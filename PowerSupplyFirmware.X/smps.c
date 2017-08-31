#include "smps.h"
#include "definitions.h"
#include <xc.h>
#include <libpic30.h>
#include <pps.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <uart.h>
#include "pid.h"

volatile static int16_t abcCoefficient[3]__attribute__((space(xmemory)));
volatile static int16_t errorHistory[3]__attribute__((space(ymemory)));

// Estados de las salidas
volatile PowerSupplyStatus buckStatus;
volatile PowerSupplyStatus aux5VStatus;
volatile PowerSupplyStatus aux3V3Status;

void __attribute__((__interrupt__, no_auto_psv)) _AD1Interrupt(void) {
    IFS0bits.ADCIF = 0;
}

/**
 * Interrupción del canal AN0 -> Buck current
 */
void __attribute__((__interrupt__, no_auto_psv)) _ADCAN0Interrupt(void) {   
    buckStatus.current = BUCK_CURRENT_ADC_BUFFER;
    
    if(buckStatus.currentCurrentSample < INSTANTANEUS_CURRENT_SAMPLES) {
        buckStatus.currentSum += buckStatus.current;
        buckStatus.currentCurrentSample++;
    }
    
    IFS6bits.ADCAN0IF = 0;
}

/**
 * Interrupción del canal AN1 -> Buck voltage
 */
void __attribute__((__interrupt__, no_auto_psv)) _ADCAN1Interrupt(void) {
    LATBbits.LATB9 = 1;
    buckStatus.PID.measuredOutput = BUCK_VOLTAGE_ADC_BUFFER;
    buckStatus.outputVoltage = buckStatus.PID.measuredOutput;
    
    if(buckStatus.enablePID) {
        mPID(&buckStatus.PID);
        setBuckDuty(buckStatus.PID.controlOutput);
    }
    
    // Guardamos los valores de tensión para luego calcular un promedio
    if(buckStatus.currentVoltageSample < INSTANTANEUS_VOLTAGE_SAMPLES) {
        buckStatus.voltageSum += buckStatus.outputVoltage;
        buckStatus.currentVoltageSample++;
    }
    
    setChannelsStep2();
    LATBbits.LATB9 = 0;
    IFS6bits.ADCAN1IF = 0;
}

/**
 * Interrupción del canal AN2 -> 5V current
 */
void __attribute__((__interrupt__, no_auto_psv)) _ADCAN2Interrupt(void) {   
    aux5VStatus.current = CURRENT_5V_ADC_BUFFER;
    
    if(aux5VStatus.currentCurrentSample < INSTANTANEUS_CURRENT_SAMPLES) {
        aux5VStatus.currentSum += aux5VStatus.current;
        aux5VStatus.currentCurrentSample++;
    }
    
    IFS7bits.ADCAN2IF = 0;
}

/**
 * Interrupción del canal AN3 -> 5V voltage. Menor prioridad que AN2 por lo que
 *  esta se ejecuta luego de AN2
 */
void __attribute__((__interrupt__, no_auto_psv)) _ADCAN3Interrupt(void) {
    aux5VStatus.outputVoltage = VOLTAGE_5V_ADC_BUFFER;
    
    // Guardamos los valores de tensión para luego calcular un promedio
    if(aux5VStatus.currentVoltageSample < INSTANTANEUS_VOLTAGE_SAMPLES) {
        aux5VStatus.voltageSum += aux5VStatus.outputVoltage;
        aux5VStatus.currentVoltageSample++;
    }
    
    IFS7bits.ADCAN3IF = 0;
}

/**
 * Interrupción del canal AN7 -> 3.3V current
 */
void __attribute__((__interrupt__, no_auto_psv)) _ADCAN7Interrupt(void) {    
    aux3V3Status.current = CURRENT_3V3_ADC_BUFFER;
    
    if(aux3V3Status.currentCurrentSample < INSTANTANEUS_CURRENT_SAMPLES) {
        aux3V3Status.currentSum += aux3V3Status.current;
        aux3V3Status.currentCurrentSample++;
    }
    
    IFS7bits.ADCAN7IF = 0;
}

/**
 * Interrupción del canal AN18 -> 3.3V voltage.
 * Como este canal tiene menor prioridad que AN7, cuando llegamos la interrupción
 *  de AN7 ya se procesó por lo que volvemos al step 1
 */
void __attribute__((__interrupt__, no_auto_psv)) _ADCAN18Interrupt(void) {
    aux3V3Status.outputVoltage = VOLTAGE_3V3_ADC_BUFFER;
    
    // Guardamos los valores de tensión para luego calcular un promedio
    if(aux3V3Status.currentVoltageSample < INSTANTANEUS_VOLTAGE_SAMPLES) {
        aux3V3Status.voltageSum += aux3V3Status.outputVoltage;
        aux3V3Status.currentVoltageSample++;
    }
    
    setChannelsStep1();
    IFS10bits.ADCAN18IF = 0;
}

/**
 * Interrupción PWM1 para interrupción FAUL
 */
void __attribute__((__interrupt__, no_auto_psv)) _PWM1Interrupt(void) {
    // Sobre corriente en el buck
    if(PWMCON1bits.FLTSTAT) {
        buckStatus.currentLimitFired = 1;
        FAULT_LAT = 0;
        IFS1bits.AC1IF = 0;
    }
    
    IFS5bits.PWM1IF = 0;
}

/**
 * Sobre corriente de la línea de 5V
 */
void __attribute__((__interrupt__, no_auto_psv)) _CMP2Interrupt(void) {    
    auxEmergency();
    aux5VStatus.currentLimitFired = 1;
    IFS6bits.AC2IF = 0;
}

/**
 * Sobre corriente de la línea de 3.3V
 */
void __attribute__((__interrupt__, no_auto_psv)) _CMP3Interrupt(void) {    
    auxEmergency();
    aux3V3Status.currentLimitFired = 1;
    IFS6bits.AC3IF = 0;
}

/**
 * Inicializa los controladores PID, estructuras del estado de las salidas,
 *  módulo PWM y ADC
 */
void smpsInit(void){   
    // Salidas de estado
    FAULT_TRIS = 0;
    FAULT_LAT = 1;
    PWR_GOOD_TRIS = 0;
    PWR_GOOD_LAT = 0;
    
    // Inicialización del PID
    buckStatus.PID.abcCoefficients = abcCoefficient;
    buckStatus.PID.errorHistory = errorHistory;
    buckStatus.PID.abcCoefficients[0] = BUCK_PIC_A_COEFFICIENT;
    buckStatus.PID.abcCoefficients[1] = BUCK_PIC_B_COEFFICIENT;
    buckStatus.PID.abcCoefficients[2] = BUCK_PIC_C_COEFFICIENT;
    buckStatus.PID.coeffFactor = 4; // 2^4 = 16
    buckStatus.PID.upperLimit = BUCK_MAX_DUTY_CYCLE;
    buckStatus.PID.lowerLimit = BUCK_MIN_DUTY_CYCLE;
    
    mPIDInit(&buckStatus.PID);
    
    // Inicialización de las estructuras de cada salida
    buckStatus.duty = BUCK_INITIAL_DUTY_CYCLE;
    buckStatus.phaseReg = BUCK_PHASE;
    buckStatus.pTrigger = 350;
    buckStatus.deadTime = BUCK_DEAD_TIME;
    buckStatus.current = 0;
    buckStatus.currentLimit = BUCK_MAX_CURRENT;
    buckStatus.enablePID = BUCK_DEFAULT_PID_ENABLE;
    buckStatus.outputVoltage = 0;
    buckStatus.averagePower = 0;
    buckStatus.currentVoltageSample = 0;
    buckStatus.currentCurrentSample = 0;
    buckStatus.averageVoltage = 65535;
    
    aux5VStatus.current = 0;
    aux5VStatus.currentLimit = MAX_CURRENT_5V;
    aux5VStatus.PID.measuredOutput = 0;
    aux5VStatus.outputVoltage = 0;
    aux5VStatus.averagePower = 0;
    aux5VStatus.currentVoltageSample = 0;
    aux5VStatus.currentCurrentSample = 0;
    aux5VStatus.averageVoltage = 0;
    
    aux3V3Status.current = 0;
    aux3V3Status.currentLimit = MAX_CURRENT_3V3;
    aux3V3Status.PID.measuredOutput = 0;
    aux3V3Status.outputVoltage = 0;
    aux3V3Status.averagePower = 0;
    aux3V3Status.currentVoltageSample = 0;
    aux3V3Status.currentCurrentSample = 0;
    aux3V3Status.averageVoltage = 0;
    
    // Configuración general del modulo PWM
    PTCONbits.PTEN = 0;         // Modulo PWM apagado
    PTCONbits.PTSIDL = 0;       // PWM continua en modo IDLE
    PTCONbits.SEIEN = 0;        // Interrupción por evento especial desactivada
    PTCONbits.EIPU = 0;         // El periodo del PWM se actualiza al final del periodo actual
    PTCONbits.SYNCPOL = 0;      // El estado activo es 1 (no invertido)
    PTCONbits.SYNCOEN = 0;      // Salida de sincronismo apagado
    PTCONbits.SYNCEN = 0;       // Entrada de sincronismo apagada
    PTCON2bits.PCLKDIV = 0;     // Divisor del PWM en 1:1
    
    STCONbits.SEIEN = 0;
    STCONbits.EIPU = 0;
    STCONbits.SYNCPOL = 0;
    STCONbits.SYNCOEN = 0;
    STCONbits.SYNCEN = 0;
    STCON2bits.PCLKDIV = 0;
    
    // Chop
    CHOPbits.CHPCLKEN = 0;      // Chop deshabilitado
    
    // Override
    IOCON1bits.OVRDAT = 0;
    IOCON1bits.OVRENH = 1;
    IOCON1bits.OVRENL = 1;
    
    TRISBbits.TRISB9 = 0;
    LATBbits.LATB9 = 0;
    setBuckVoltage(6000);
    
    _initADC();
    _initComparators();
    _initBuckPWM(&buckStatus);
    
    // PWM2 y PWM3 controlados por GPIO y no por el PWM (valor por defecto)
    IOCON2bits.PENH = IOCON2bits.PENL = 0;
    IOCON3bits.PENH = IOCON3bits.PENL = 0;
    
    // Enciendo PWM el modulo PWM
    PTCONbits.PTEN = 1;
    IOCON1bits.OVRENH = 0;
    IOCON1bits.OVRENL = 0;
    
    __delay_us(100);            // Esperamos algunos ciclos para que
                                //  se estabilice el PWM
    
    // Habilitamos las salidas
    IOCON1bits.PENH = 1;
    IOCON1bits.PENL = 1;
    
    auxEnable();
    
    ON_OFF_5V_TRIS = 0;
    ON_OFF_5V_LAT = 0;
    PWR_GOOD_LAT = 1; 
}

void smpsTasks(void) {
    uint32_t val;
    
    // Cálculo de la potencia media del buck
    if(buckStatus.currentVoltageSample >= INSTANTANEUS_VOLTAGE_SAMPLES && buckStatus.currentVoltageSample >= INSTANTANEUS_CURRENT_SAMPLES) {      
        val = buckStatus.voltageSum / INSTANTANEUS_VOLTAGE_SAMPLES;
        buckStatus.averageVoltage = val;
        buckStatus.voltageSum = 0;
        
        val = buckStatus.currentSum / INSTANTANEUS_CURRENT_SAMPLES;
        buckStatus.averageCurrent = val;
        buckStatus.currentSum = 0;
        
        buckStatus.averagePower = (uint32_t)buckStatus.averageCurrent * (uint32_t)buckStatus.averageVoltage;
        
        buckStatus.currentCurrentSample = 0;
        buckStatus.currentVoltageSample = 0;
    }
    
    // Cálculo de la potencia media de la línea de 5V
    if(aux5VStatus.currentVoltageSample >= INSTANTANEUS_VOLTAGE_SAMPLES && aux5VStatus.currentVoltageSample >= INSTANTANEUS_CURRENT_SAMPLES) {       
        val = aux5VStatus.voltageSum / INSTANTANEUS_VOLTAGE_SAMPLES;
        aux5VStatus.averageVoltage = val;
        aux5VStatus.voltageSum = 0;
        
        val = aux5VStatus.currentSum / INSTANTANEUS_CURRENT_SAMPLES;
        aux5VStatus.averageCurrent = val;
        aux5VStatus.currentSum = 0;
        
        aux5VStatus.averagePower = (uint32_t)aux5VStatus.averageCurrent * (uint32_t)aux5VStatus.averageVoltage;
        
        aux5VStatus.currentCurrentSample = 0;
        aux5VStatus.currentVoltageSample = 0;
    }
    
    // Cálculo de la potencia media de la línea de 3.3V
    if(aux3V3Status.currentVoltageSample >= INSTANTANEUS_VOLTAGE_SAMPLES && aux3V3Status.currentVoltageSample >= INSTANTANEUS_CURRENT_SAMPLES) {
        val = aux3V3Status.voltageSum / INSTANTANEUS_VOLTAGE_SAMPLES;
        aux3V3Status.averageVoltage = val;
        aux3V3Status.voltageSum = 0;
        
        val = aux3V3Status.currentSum / INSTANTANEUS_CURRENT_SAMPLES;
        aux3V3Status.averageCurrent = val;
        aux3V3Status.currentSum = 0;
        
        aux3V3Status.averagePower = (uint32_t)aux3V3Status.averageCurrent * (uint32_t)aux3V3Status.averageVoltage;
        
        aux3V3Status.currentCurrentSample = 0;
        aux3V3Status.currentVoltageSample = 0;
    }
}

void buckEnable() {
    uint16_t prevFaultMode = 0;
    
    mPIDInit(&buckStatus.PID);
    buckStatus.currentLimitFired = 0;
    buckStatus.enablePID = 1;
    buckStatus.enabled = 1;
    FAULT_LAT = 1;
    
    // Procedimiento para borrar condición FAULT
    IOCON1bits.OVRENH = 1;      // Activamos override (salidas a 0)
    IOCON1bits.OVRENL = 1;
    prevFaultMode = FCLCON1bits.FLTMOD;
    FCLCON1bits.FLTMOD = 0b11;  // Desactivamos el modo fault
    __delay_us(5);              // Delay de al menos un ciclo de PWM
    FCLCON1bits.FLTMOD = prevFaultMode;
    PWMCON1bits.FLTIEN = 0;     // Borramos el bit de status de fault
    PWMCON1bits.FLTIEN = 1;
    IOCON1bits.OVRENH = 0;      // Desactivamos el override
    IOCON1bits.OVRENL = 0;
}

void auxEnable() {    
    aux5VStatus.currentLimitFired = 0;
    aux3V3Status.currentLimitFired = 0;
    
    aux3V3Status.enabled = 1;
    aux5VStatus.enabled = 1;
    
    ON_OFF_5V_LAT = 0;
    FAULT_LAT = 1;
}

void buckDisable(void) {
    IOCON1bits.OVRENH = 1; 
    IOCON1bits.OVRENL = 1;
    
    buckStatus.enablePID = 0;
    buckStatus.enabled = 0;
}

void auxDisable(void) {
    ON_OFF_5V_LAT = 1;
    
    aux3V3Status.enabled = 0;
    aux5VStatus.enabled = 0;
}

/**
 * Configura la tensión de salida del Buck 1
 * @param voltage tensión de salida en milivolts
 */
void setBuckVoltage(uint16_t voltage) {
    // Convertimos la tensión de salida en milivolts deseada al valor
    //  correspondiente del ADC que debería ser leído aplicando el factor
    //  de corrección
    if(voltage < BUCK_MIN_VOLTAGE) {
        voltage = BUCK_MIN_VOLTAGE;
    }
    else if(voltage > BUCK_MAX_VOLTAGE) {
        voltage = BUCK_MAX_VOLTAGE;
    }
    
    float v = (float)voltage * (float)BUCK_V_FEEDBACK_FACTOR;
    buckStatus.PID.controlReference = getInverseMatchedADCValue((uint16_t)(v*((float)BUCK_VOLTAGE_ADC_COUNTS/(ADC_VREF*1000.0))),
            BUCK_ADC_VOLTAGE_OFFSET, BUCK_ADC_VOLTAGE_GAIN);
}

/**
 * Configura el límite de corriente para el Buck 1
 * @param currentLimit límite de corriente en miliamperes
 */
void setBuckCurrentLimit(uint16_t currentLimit) {
    // Conversión de corriente de salida a tensión en la entrada del comparador
    //  en milivolts
    float v = (float)BUCK_I_FEEDBACK_FACTOR * (float)currentLimit;
    uint16_t ref = (uint16_t)(v * ((float)4096/(ADC_VREF*1000.0)));
    ref = getInverseMatchedADCValue(ref, BUCK_ADC_CURRENT_OFFSET, BUCK_ADC_CURRENT_GAIN);
    
    if(ref > BUCK_MAX_CURRENT) {
        ref = BUCK_MAX_CURRENT;
    }
    
    // Calculamos el valor para el DAC del comparador
    CMP1DACbits.CMREF = ref;
    buckStatus.currentLimit = ref;
}

/**
 * Configura el límite de corriente para la salida de 5V
 * @param currentLimit límite de corriente en miliamperes
 */
void set5VCurrentLimit(uint16_t currentLimit) {
    // Conversión de corriente de salida a tensión en la entrada del comparador
    //  en milivolts
    float v = (float)AUX_5V_I_FEEDBACK_FACTOR * (float)currentLimit;
    uint16_t ref = (uint16_t)(v * ((float)4096/(ADC_VREF*1000.0)));
    
    if(ref > MAX_CURRENT_5V) {
        ref = MAX_CURRENT_5V;
    }
    
    // Calculamos el valor para el DAC del comparador
    CMP2DACbits.CMREF = ref;
    aux5VStatus.currentLimit = ref;
}

/**
 * Configura el límite de corriente para la salida de 3.3V
 * @param currentLimit límite de corriente en miliamperes
 */
void set3V3CurrentLimit(uint16_t currentLimit) {
    // Conversión de corriente de salida a tensión en la entrada del comparador
    //  en milivolts
    float v = (float)AUX_3V3_I_FEEDBACK_FACTOR * (float)currentLimit;
    uint16_t ref = (uint16_t)(v * ((float)4096/(ADC_VREF*1000.0)));
    
    if(ref > MAX_CURRENT_3V3) {
        ref = MAX_CURRENT_3V3;
    }
    
    // Calculamos el valor para el DAC del comparador
    CMP3DACbits.CMREF = ref;
    aux3V3Status.currentLimit = ref;
}

/**
 * Calcula el valor del ADC real con corrección aplicada a partir de un valor de
 *  ADC leído desde el módulo. Se aplica la siguiente ecuación:
 * 
 *  ADC Corregido = (adcValue * gain) + offset
 * 
 * En donde la ganancia "gain" y el offset se calculan de la siguiente manera
 *  considerando que se toman 2 muestras de tensiones en el ADC en cada uno de
 *  los extremos del rango de tensión de trabajo donde el punto 2 es el de
 *  mayor valor:
 * 
 *      gain = (Ideal2 - Ideal1) / (Leido2 - Leido1)
 * 
 *      offset = Ideal1 - (gain * Leido1)
 * 
 * @param adcValue valor del ADC a corregir
 * @param offset offset del ADC a corregir
 * @param gain ganancia del ADC a corregir
 * @return valor del ADC corregido
 */
uint16_t getMatchedADCValue(uint16_t adcValue, float offset, float gain) {
    return (uint16_t)((float)adcValue * gain + offset);
}

/**
 * Calcula el valor que debería leer el ADC a partir del valor ideal que debería
 *  leer si fuera un ADC perfecto sin errores. Utiliza la ecuación:
 * 
 *      Valor ADC para leer = (Valor_ADC_Ideal - offset) / gain
 * 
 * Para más información leer los comentarios de la función getMatchedADCValue()
 * 
 * @param adcValue valor del ADC a corregir
 * @param offset offset del ADC a corregir
 * @param gain ganancia del ADC a corregir
 * @return valor del ADC corregido
 */
uint16_t getInverseMatchedADCValue(uint16_t adcValue, float offset, float gain) {
    return (uint16_t)(((float)adcValue - offset) / gain);
}

void _initBuckPWM(PowerSupplyStatus *data) {
    BUCK_PWMH_TRIS = BUCK_PWML_TRIS = 1;
    
    // Si se usa un debugger y se detiene en un breakpoint las salidas de PWM
    //  toman el estado de los registros LAT, por lo que las deshabilitamos
    //  para evitar cortocircuitos en el buck
    BUCK_PWMH_LAT = BUCK_PWML_LAT = 0;

    PWMCON1bits.FLTIEN = 1;     // Fault Interrupt habilitada
    PWMCON1bits.CLIEN = 0;      // Current-limit Interrupt deshabilitada
    PWMCON1bits.TRGIEN = 0;     // No generar interrupción en trigger
    PWMCON1bits.ITB = 1;        // Independent Time Base. En Complementary controlamos
                                //  el periodo con los registros PHASEx/SPHASEx
    PWMCON1bits.MDCS = 0;       // Los registros PDC1/SDC1 determinan el duty cycle
                                //  del PWM1, no usamos un duty cycle global (MDC)
    PWMCON1bits.DTC = 0;        // Dead-time positivo
    PWMCON1bits.MTBS = 0;       // Usamos base de tiempo primaria como fuente clock (no secundaria)
    PWMCON1bits.CAM = 0;        // Edge-Aligned PWM
    PWMCON1bits.XPRES = 0;      // Los pines externos por sobre-corriente deshabilitados
    PWMCON1bits.IUE = 0;        // Los cambios en el duty son sincronizados con la base
                                //  de tiempo del PWM (no son inmediatos)
    
    FCLCON1 = 0;
    FCLCON1bits.IFLTMOD = 0;        // FLTDAT<1:0> mapea los estados de los pines PWMH y PWML
    FCLCON1bits.CLMOD = 0;          // Current limit deshabilitado
    FCLCON1bits.FLTSRC = 0b01101;   // Comparador 1 va a la entrada de FAULT
    FCLCON1bits.FLTPOL = 0;         // FAULT activo alto
    FCLCON1bits.FLTMOD = 0b00;      // FAULT activado y modo latch
    
    TRIG1 = data->pTrigger;     // Trigger principal
    STRIG1 = data->pTrigger;    // Trigger secundario
    TRGCON1bits.DTM = 0;        // Generamos triggers principales y secundarios (no combinados)
    TRGCON1bits.TRGSTRT = 0;    // Esperamos 0 ciclos de PWM al iniciar el modulo antes
                                //  de comenzar a contar los ciclos para el trigger
    TRGCON1bits.TRGDIV = 0;     // Cada 1 periodo del PWM se genera un evento de
                                //  trigger

    IOCON1bits.PENH = 0;        // Pin PWMH deshabilitado
    IOCON1bits.PENL = 0;        // Pin PWML deshabilitado
    IOCON1bits.POLH = 0;        // PWMH activo alto
    IOCON1bits.POLL = 0;        // PWML activo alto
    IOCON1bits.PMOD = 0;        // PMW1 en modo Complementario (para Buck sincrónico)
    IOCON1bits.SWAP = 1;        // PWMH y PWML a sus respectivas salidas y no al revés
    IOCON1bits.OSYNC = 0;       // Override es asíncrono
    IOCON1bits.FLTDAT = 0b00;   // PWMH y PWML a 0 en condicion de FAULT
    
    AUXCON1bits.HRPDIS = 0;     // Alta resolución de periodo
    AUXCON1bits.HRDDIS = 0;     // Alta resolución de duty
    AUXCON1bits.BLANKSEL = 0;   // Blanking deshabilitado
    AUXCON1bits.CHOPHEN = 0;    // Chop deshabilitado en PWMH
    AUXCON1bits.CHOPLEN = 0;    // Chop deshabilitado en PWML
    
    IFS5bits.PWM1IF = 0;        // Interrupción
    IPC23bits.PWM1IP = 7;
    IEC5bits.PWM1IE = 1;

    /*
     * Período del PWM viene dado por:
     *  ( (ACLK * 8 * PWM_PERIOD)/(PMW_INPUT_PREESCALER) ) - 8
     *
     * Para una frecuencia de 300KHz con preescaler 1:1 y el ACLK en 117.92MHz
     */
    PHASE1 = data->phaseReg;

    /*
     * El duty cycle viene dado por:
     *  (ACLK * 8 * PWM_DUTY)/(PMW_INPUT_PREESCALER)
     *
     * La resolución del duty es:
     *  log2( (ACLK * 8 * PWM_PERIOD)/(PMW_INPUT_PREESCALER) ) = 11.6 bits
     *
     */
    PDC1 = SDC1 = data->duty;

    /*
     * El Dead-time viene dado por:
     *  (ACLK * 8 * DEAD_TIME)/(PMW_INPUT_PREESCALER)
     */
    DTR1 = data->deadTime;      // Dead-time para PWMH
    ALTDTR1 = data->deadTime;   // Dead-time para PWML
}

void _initADC(void) {
    /**
     * Es importante que todos los cores del ADC estén configurados exactamente
     * igual, su fuente de trigger sea la misma e inicien la conversión al mismo
     * tiempo para poder hacer sampling simultáneo de otro modo el resultado de
     * la conversión será erróneo debido a un error en el modulo. Copia del errata:
     * 
     *  When using multiple ADC cores, if one of the
     *  ADC cores completes conversion while other
     *  ADC cores are still converting, the data in the
     *  ADC cores which are converting may be
     *  randomly corrupted.
     *  
     *  Work around 1: When using multiple ADC cores,
     *  the ADC triggers are to be sufficiently staggered
     *  in time to ensure that the end of conversion of
     *  one or more cores doesn't occur during the
     *  conversion process of other cores.
     * 
     *  Work around 2: For simultaneous conversion
     *  requirements, make sure the following
     *  conditions are met:
     *      1. All the ADC cores for simultaneous
     *         conversion should have the same
     *         configurations.
     *      2. Avoid shared ADC core conversion with
     *         any of the dedicated ADC cores. They can
     *         be sequential.
     *      3. The trigger to initiate ADC conversion
     *         should be from the same source and at the
     *         same time
     */
    ADCON1Lbits.ADON = 0;
    
    // Todos los pines como IO digital
    ANSELA = 0x0000;
    ANSELB = 0x0000;
    
    BUCK_CURRENT_TRIS = 1;
    BUCK_VOLTAGE_TRIS = 1;
    AUX_5V_CURRENT_TRIS = 1;
    AUX_5V_VOLTAGE_TRIS = 1;
    AUX_3V_CURRENT_TRIS = 1;
    AUX_3V_VOLTAGE_TRIS = 1;
    
    ANSELAbits.ANSA0 = 1;       // RA0 analógica
    ANSELAbits.ANSA1 = 1;       // RA1 analógica
    ANSELAbits.ANSA2 = 1;       // RA2 analógica
    ANSELBbits.ANSB0 = 1;       // RB0 analógica
    ANSELBbits.ANSB2 = 1;       // RB2 analógica
    ANSELBbits.ANSB3 = 1;       // RB3 analógica
    
    ADCON1Lbits.ADSIDL = 0;     // Modulo continua operando en modo IDLE
    ADCON1Hbits.FORM = 0;       // Dato de salida en formato integer (no fractional)
    ADCON1Hbits.SHRRES = 0b11;  // Resolución de 12 bits
    
    ADCON2Lbits.REFCIE = 0;     // Interrupción para band-gap y reference voltage ready desactivada
    ADCON2Lbits.REFERCIE = 0;   // Interrupción para band-gap y reference voltage error desactivada
    ADCON2Lbits.EIEN = 0;       // Early interrupt desactivada
    
    ADCON3Lbits.SUSPEND = 0;
    ADCON3Lbits.SUSPCIE = 0;
    ADCON3Lbits.SWLCTRG = 0;    // Level-sensitive trigger desactivado
    ADCON3Hbits.CLKSEL = 0b11;  // Clock auxiliar (117.92MHz) como fuente
    ADCON3Hbits.CLKDIV = 0;     // Divisor 1:1 para el clock del modulo ADC
    ADCON3Hbits.SHREN = 1;      // Shared ADC core habilitado
    ADCON3Hbits.C0EN = 1;       // ADC Core 0 habilitado
    ADCON3Hbits.C1EN = 1;       // ADC Core 1 habilitado
    ADCON3Hbits.C2EN = 1;       // ADC Core 2 habilitado
    ADCON3Hbits.C3EN = 1;       // ADC Core 3 habilitado
    
    // Habilitamos la espera del tiempo de muestreo (requerido por oversampling)
    ADCON4Lbits.SAMC0EN = 1;    // Luego del trigger se espera el tiempo de muestreo antes de iniciar la conversión en core 0
    ADCON4Lbits.SAMC1EN = 1;    // Luego del trigger se espera el tiempo de muestreo antes de iniciar la conversión en core 1
    ADCON4Lbits.SAMC2EN = 1;    // Luego del trigger se espera el tiempo de muestreo antes de iniciar la conversión en core 2
    ADCON4Lbits.SAMC3EN = 1;    // Luego del trigger se espera el tiempo de muestreo antes de iniciar la conversión en core 3
    
    // Todos los cores tienen un divisor 1:2 por lo que el clock en cada
    //  core es de 117.92MHz / 2 = 58.96 Mhz = Tad
    //
    // Conversion time  = 8 * Tcoresrc + (Resolution + 2.5) * Tadcore
    //                  = 8 * (1/117.92Mhz) + (12 + 2.5) * (1/58.96Mhz)
    //                  = 314 ns + 33.92ns (sampling) = 2.87 MSPS
    // Core 0
    ADCORE0Lbits.SAMC = 0;      // 2 Tad sample time
    ADCORE0Hbits.RES = 0b11;    // 12 bit de resolución
    ADCORE0Hbits.ADCS = 0;      // Divisor 1:2 de clock
    // Core 1
    ADCORE1Lbits.SAMC = 0;      // 2 Tad sample time
    ADCORE1Hbits.RES = 0b11;    // 12 bit de resolución
    ADCORE1Hbits.ADCS = 0;      // Divisor 1:2 de clock
    // Core 2
    ADCORE2Lbits.SAMC = 0;      // 2 Tad sample time
    ADCORE2Hbits.RES = 0b11;    // 12 bit de resolución
    ADCORE2Hbits.ADCS = 0;      // Divisor 1:2 de clock
    // Core 3
    ADCORE3Lbits.SAMC = 0;      // 2 Tad sample time
    ADCORE3Hbits.RES = 0b11;    // 12 bit de resolución
    ADCORE3Hbits.ADCS = 0;      // Divisor 1:2 de clock
    // Shared Core
    ADCON2Hbits.SHRSAMC = 0b0000000000; // Shared ADC 2 Tad sample time
    ADCON2Lbits.SHRADCS = 0b0000000;    // Divisor 1:2 de clock
    
    ADCON5Hbits.WARMTIME = 15;  // Máximo tiempo de encendido para los cores
    ADCON5Hbits.SHRCIE = 0;     // Interrupción de Shared ADC Core ready desactivada
    ADCON5Hbits.C0C1E = 0;      // Interrupción de ADC Core 0 ready desactivada
    ADCON5Hbits.C1C1E = 0;      // Interrupción de ADC Core 1 ready desactivada
    ADCON5Hbits.C2C1E = 0;      // Interrupción de ADC Core 2 ready desactivada
    ADCON5Hbits.C3C1E = 0;      // Interrupción de ADC Core 3 ready desactivada
    
    ADMOD0L = 0;                // Canales en modo single-ended y formato unsigned
    ADMOD1L = 0;                // Canales en modo single-ended y formato unsigned
    ADIEL = 0xFFFF;             // Interrupciones habilitadas para todos los canales
    ADIEH = 0xFFFF;             // Interrupciones habilitadas para todos los canales
    
    // Triggers
    ADTRIG0Lbits.TRGSRC0 = 0b00101;     // AN0 PWM Generator 1 primary trigger (Buck current)
    ADTRIG0Lbits.TRGSRC1 = 0b00101;     // AN1 PWM Generator 1 primary trigger (Buck voltage)
    ADTRIG0Hbits.TRGSRC2 = 0b00101;     // AN2 PWM Generator 1 primary trigger (5V current)
    ADTRIG0Hbits.TRGSRC3 = 0b00101;     // AN3 PWM Generator 1 primary trigger (5V voltage)
    ADTRIG1Hbits.TRGSRC7 = 0b00101;     // AN7 PWM Generator 1 primary trigger (3.3V current)
    ADTRIG4Hbits.TRGSRC18 = 0b00101;    // AN18 PWM Generator 1 primary trigger (3.3V voltage)
    
    // Encendido de ADC
    ADCON1Lbits.ADON = 1;
    
    ADCON5Lbits.SHRPWR = 1;     // Shared core encendido
    ADCON5Lbits.C0PWR = 1;      // Dedicated core 0 encendido
    ADCON5Lbits.C1PWR = 1;      // Dedicated core 1 encendido
    ADCON5Lbits.C2PWR = 1;      // Dedicated core 2 encendido
    ADCON5Lbits.C3PWR = 1;      // Dedicated core 3 encendido
    
    // Esperamos que todos los cores se enciendan
    while(!ADCON5Lbits.C0RDY || !ADCON5Lbits.C1RDY || !ADCON5Lbits.C2RDY 
            || !ADCON5Lbits.C3RDY || !ADCON5Lbits.SHRRDY);
    
    // Calibración para single-ended
    ADCAL0Lbits.CAL0EN = 1;     // Habilitamos modo calibración
    ADCAL0Lbits.CAL1EN = 1;
    ADCAL0Hbits.CAL2EN = 1;
    ADCAL0Hbits.CAL3EN = 1;
    ADCAL1Hbits.CSHREN = 1;
    ADCAL0Lbits.CAL0DIFF = 0;   // Modo single-ended
    ADCAL0Lbits.CAL1DIFF = 0;
    ADCAL0Hbits.CAL2DIFF = 0;
    ADCAL0Hbits.CAL3DIFF = 0;
    ADCAL1Hbits.CSHRDIFF = 0;
    ADCAL0Lbits.CAL0RUN = 1;    // Inicio de calibración
    ADCAL0Lbits.CAL1RUN = 1;
    ADCAL0Hbits.CAL2RUN = 1;
    ADCAL0Hbits.CAL3RUN = 1;
    ADCAL1Hbits.CSHRRUN = 1;
    
    // Esperamos a que la calibración finalice
    while(!ADCAL0Lbits.CAL0RDY || !ADCAL0Lbits.CAL1RDY || !ADCAL0Hbits.CAL2RDY
            || !ADCAL0Hbits.CAL3RDY || !ADCAL1Hbits.CSHRRDY);
    
    ADCAL0Lbits.CAL0EN = 0;         // Deshabilitamos modo calibración
    ADCAL0Lbits.CAL1EN = 0;
    ADCAL0Hbits.CAL2EN = 0;
    ADCAL0Hbits.CAL3EN = 0;
    ADCAL1Hbits.CSHREN = 0;
    
    // Configuración de oversampling
    ADFL0CONbits.FLEN = 0;
    ADFL0CONbits.MODE = 0b00;       // Modo oversampling
    ADFL0CONbits.OVRSAM = 0b000;    // 4x oversampling (1 bit adicional)
    ADFL0CONbits.FLCHSEL = 1;       // AN1 para el filtro digital 0
    ADFL0CONbits.IE = 1;            // Interrupción habilitada
    ADFL0CONbits.FLEN = 0;
    
    // Interrupciones filtros
    IFS11bits.ADFLTR0IF = 0;         // Borramos flag de interrupción
    IPC44bits.ADFLTR0IP = 6;         // Prioridad 6 para filter 0 (current buck)
    IEC11bits.ADFLTR0IE = 1;         // Habilitamos interrupción de los filtros
    
    // Interrupciones de canales
    // AN0 (buck current)
    IFS6bits.ADCAN0IF = 0;
    IEC6bits.ADCAN0IE = 1;
    IPC27bits.ADCAN0IP = 6;
    
    // AN1 (buck voltage)
    IFS6bits.ADCAN1IF = 0;
    IEC6bits.ADCAN1IE = 1;
    IPC27bits.ADCAN1IP = 5;         // Este canal trabaja a través del filtro únicamente
    
    // AN2 (5v current)
    IFS7bits.ADCAN2IF = 0;
    IEC7bits.ADCAN2IE = 1;
    IPC28bits.ADCAN2IP = 4;
    
    // AN3 (5v voltage)
    IFS7bits.ADCAN3IF = 0;
    IEC7bits.ADCAN3IE = 1;
    IPC28bits.ADCAN3IP = 3;
    
    // AN7 (3.3v current)
    IFS7bits.ADCAN7IF = 0;
    IEC7bits.ADCAN7IE = 1;
    IPC29bits.ADCAN7IP = 6;
    
    // AN18 (3.3v voltage)
    IFS10bits.ADCAN18IF = 0;
    IEC10bits.ADCAN18IE = 1;
    IPC40bits.ADCAN18IP = 5;
}

void _initComparators(void) {
    // Comparador 1 - Corriente buck
    CMP1CONbits.CMPON = 0;      
    CMP1CONbits.CMPSIDL = 0;
    CMP1CONbits.HYSSEL = 0b01;      // Histéresis de 15mV = 15mA
    CMP1CONbits.FLTREN = 1;         // Filtro digital habilitado
    CMP1CONbits.FCLKSEL = 1;        // Filtro digital clock desde PWM
    CMP1CONbits.DACOE = 0;          // Salida del DAC desactivada
    CMP1CONbits.ALTINP = 0;         // No usamos entradas alternativas
    CMP1CONbits.INSEL = 0b00;       // CMP1A de entrada positiva
    CMP1CONbits.EXTREF = 0;         // Avdd de referencia para el DAC
    CMP1CONbits.HYSPOL = 1;         // Histéresis se aplica al falling edge del comparador
    CMP1CONbits.CMPPOL = 0;         // Salida no invertida
    CMP1CONbits.RANGE = 1;
    
    // Comparador 2 - Corriente 5V
    CMP2CONbits.CMPON = 0;      
    CMP2CONbits.CMPSIDL = 0;
    CMP2CONbits.HYSSEL = 0b00;      // Sin Histéresis
    CMP2CONbits.FLTREN = 1;         // Filtro digital habilitado
    CMP2CONbits.FCLKSEL = 1;        // Filtro digital clock desde PWM
    CMP2CONbits.DACOE = 0;          // Salida del DAC desactivada
    CMP2CONbits.ALTINP = 0;         // No usamos entradas alternativas
    CMP2CONbits.INSEL = 0b00;       // CMP2A de entrada positiva
    CMP2CONbits.EXTREF = 0;         // Avdd de referencia para el DAC
    CMP2CONbits.HYSPOL = 1;         // Histéresis se aplica al falling edge del comparador
    CMP2CONbits.CMPPOL = 0;         // Salida no invertida
    CMP2CONbits.RANGE = 1;
    
    // Comparador 3 - Corriente 3.3V
    CMP3CONbits.CMPON = 0;      
    CMP3CONbits.CMPSIDL = 0;
    CMP3CONbits.HYSSEL = 0b00;      // Sin Histéresis
    CMP3CONbits.FLTREN = 1;         // Filtro digital habilitado
    CMP3CONbits.FCLKSEL = 1;        // Filtro digital clock desde PWM
    CMP3CONbits.DACOE = 0;          // Salida del DAC desactivada
    CMP3CONbits.ALTINP = 0;         // No usamos entradas alternativas
    CMP3CONbits.INSEL = 0b11;       // CMP3D de entrada positiva
    CMP3CONbits.EXTREF = 0;         // Avdd de referencia para el DAC
    CMP3CONbits.HYSPOL = 1;         // Histéresis se aplica al falling edge del comparador
    CMP3CONbits.CMPPOL = 0;         // Salida no invertida
    CMP3CONbits.RANGE = 1;
    
    // Interrupciones
    IEC1bits.AC1IE = 0;             // Este comparador va a la entrada FAULT del modulo PWM
                                    //  y se genera una interrupción por FAULT
    IFS1bits.AC1IF = 0;
    
    IFS6bits.AC2IF = 0;
    IPC25bits.AC2IP = 7;            // Prioridad máxima
    IEC6bits.AC2IE = 1;
    
    IFS6bits.AC3IF = 0;
    IPC26bits.AC3IP = 7;            // Prioridad máxima
    IEC6bits.AC3IE = 1;
    
    // Corriente máxima de límite
    CMP1DACbits.CMREF = BUCK_MAX_CURRENT;
    CMP2DACbits.CMREF = MAX_CURRENT_5V;
    CMP3DACbits.CMREF = MAX_CURRENT_3V3;
    
    // Encendido de los comparadores
    CMP1CONbits.CMPON = 1;
    CMP2CONbits.CMPON = 1;
    CMP3CONbits.CMPON = 1;
}
