#include "smps.h"
#include "definitions.h"
#include <xc.h>
#include <libpic30.h>
#include <dsp.h>
#include <pps.h>
#include <stdint.h>
#include <math.h>
#include "pid.h"

// Buffer para calculo de potencia media
#define INSTANTANEUS_POWER_BUFFER_SAMPLES 32
static volatile uint16_t currentPowerSample = 0;
static volatile uint32_t powerValues[INSTANTANEUS_POWER_BUFFER_SAMPLES] = {0};

// Tablas de corrección del ADC
#define ADC_VOLTAGE_ERROR_TABLE_SIZE 22
static const int16_t adcVoltageErrorTable[][2] = { 
    { 0, 0 },         // { ADC Value, Cuanto sumar al valor del ADC obtenido }
    { 36/2, 0 },      // 1V
    { 70/2, 0 },      // 2V
    { 104/2, 0 },     // 3V
    { 138/2, 0 },     // 4V
    { 172/2, 0 },     // 5V
    { 205/2, 0 },     // Y asi sigue...
    { 239/2, 0 },
    { 273/2, 0 },
    { 306/2, 0 },
    { 340/2, 0 },
    { 372/2, 0 },
    { 406/2, 0 },
    { 440/2, 0 }, 
    { 473/2, 0 },
    { 508/2, 0 },
    { 539/2, 0 },
    { 573/2, 0 },
    { 606/2, 0 },
    { 641/2, 0 },
    { 674/2, 0 },
    { 1023/2, 0 }
};

#define ADC_CURRENT_ERROR_TABLE_SIZE 22
static const int16_t adcCurrentErrorTable[][2] = { 
    { 0, 0 },         // { ADC Value, Cuanto sumar al valor del ADC obtenido }
    { 43/2, 0 },      
    { 79/2, 0 },      
    { 116/2, 0 },     
    { 155/2, 0 },     
    { 191/2, 0 },     
    { 228/2, 0 },     
    { 264/2, 0 },
    { 297/2, 0 },
    { 340/2, 0 },
    { 376/2, 0 },
    { 416/2, 0 },
    { 455/2, 0 },
    { 493/2, 0 }, 
    { 527/2, 0 },
    { 566/2, 0 },
    { 605/2, 0 },
    { 543/2, 0 },
    { 681/2, 0 },
    { 719/2, 0 },
    { 757/2, 0 },
    { 1023/2, 0}
};

// Estados de las salidas
PowerSupplyStatus buckStatus;
PowerSupplyStatus aux5VStatus;
PowerSupplyStatus aux3V3Status;

/**
 * Interrupción del filtro digital 0. Corresponde a AN0 -> Buck current.
 * El tiempo total de obtención es el tiempo de muestreo + tiempo de conversión.
 * 
 * Total time = Conversion time + Sampling time = 314ns + 34ns = 350ns
 * 
 * El filtro tiene un oversampling de 16x por lo que deberia ejecutarse cada:
 * 
 *  350ns * 16 = 5.6us
 */
void __attribute__((__interrupt__, no_auto_psv)) _ADFLTR0Interrupt(void) {
    LATBbits.LATB5 = !LATBbits.LATB5;
    
    // Verificamos que no haya exceso de corriente
    #ifdef ENABLE_CURRENT_PROTECTION
    if(BUCK_CURRENT_ADC_BUFFER > BUCK_MAX_CURRENT){
        buckEmergency();
        buckStatus.overCurrent = 1;
        IFS11bits.ADFLTR0IF = 0;
        return;
    }
    else if(BUCK_CURRENT_ADC_BUFFER > buckStatus.currentLimit) {
        buckEmergency();
        buckStatus.currentLimitFired = 1;
        IFS11bits.ADFLTR0IF = 0;
        return;
    }
    #endif
    buckStatus.current = BUCK_CURRENT_ADC_BUFFER;
    IFS11bits.ADFLTR0IF = 0;
}

/**
 * Interrupción del filtro digital 1. Corresponde a AN1 -> Buck voltage.
 * 
 * Este es un filtro de oversampling, al mismo tiempo los core 2 y 3 convierten
 *  los valores de tensión y corriente de la línea de 5V por lo que para cuando
 *  ambos filtros terminaron, las conversiones de 5V ya están finalizadas ya que
 *  es un solo muestreo.
 * El filtro digital 1 es el de menor prioridad de interrupción y ya que ambos
 *  filtros estan asociados a entradas con un trigger común, si llegamos acá el
 *  filtro digital 0 ya finalizó por lo que podemos pasar al step 2 y liberar los
 *  cores 0 y 1 para leer el canal de 3.3V
 */
void __attribute__((__interrupt__, no_auto_psv)) _ADFLTR1Interrupt(void) {
    register int16_t output asm("w0");
    
    buckStatus.PID.measuredOutput = BUCK_VOLTAGE_ADC_BUFFER;
    buckStatus.outputVoltage = BUCK_VOLTAGE_ADC_BUFFER;
    if(buckStatus.enablePID) {
        output = myPI(&buckStatus.PID);

        if(output > BUCK_MAX_DUTY_CYCLE){
            output = BUCK_MAX_DUTY_CYCLE;
        }
        else if(output < BUCK_MIN_DUTY_CYCLE) {
            output = BUCK_MIN_DUTY_CYCLE;
        }
        setBuckDuty(output);
    }

    setChannelsStep2();
    IFS11bits.ADFLTR1IF = 0;
}

/**
 * interrupción del canal AN2 -> 5V current
 */
void __attribute__((__interrupt__, no_auto_psv)) _ADCAN2Interrupt(void) {
    if(CURRENT_5V_ADC_BUFFER > MAX_CURRENT_5V){
        auxEmergency();
        aux5VStatus.overCurrent = 1;
        IFS7bits.ADCAN2IF = 0;
        return;
    }
    else if(CURRENT_5V_ADC_BUFFER > aux5VStatus.currentLimit) {
        auxEmergency();
        aux5VStatus.currentLimitFired = 1;
        IFS7bits.ADCAN2IF = 0;
        return;
    }
    
    aux5VStatus.current = CURRENT_5V_ADC_BUFFER;
    IFS7bits.ADCAN2IF = 0;
}

/**
 * Interrupción del canal AN3 -> 5V voltage
 */
void __attribute__((__interrupt__, no_auto_psv)) _ADCAN3Interrupt(void) {
    aux5VStatus.outputVoltage = VOLTAGE_5V_ADC_BUFFER;
    IFS7bits.ADCAN3IF = 0;
}

/**
 * Interrupción del canal AN7 -> 3.3V current
 */
void __attribute__((__interrupt__, no_auto_psv)) _ADCAN7Interrupt(void) {
    if(CURRENT_3V3_ADC_BUFFER > MAX_CURRENT_3V3){
        auxEmergency();
        aux3V3Status.overCurrent = 1;
        IFS7bits.ADCAN7IF = 0;
        return;
    }
    else if(CURRENT_3V3_ADC_BUFFER > aux3V3Status.currentLimit) {
        auxEmergency();
        aux3V3Status.currentLimitFired = 1;
        IFS7bits.ADCAN7IF = 0;
        return;
    }
    
    aux3V3Status.current = CURRENT_3V3_ADC_BUFFER;
    IFS7bits.ADCAN7IF = 0;
}

/**
 * Interrupción del canal AN18 -> 3.3V voltage.
 * Como este canal tiene menor prioridad que AN7, cuando llegamos la interrupción
 *  de AN7 ya se procesó por lo que volvemos al step 1
 */
void __attribute__((__interrupt__, no_auto_psv)) _ADCAN18Interrupt(void) {
    aux3V3Status.outputVoltage = VOLTAGE_3V3_ADC_BUFFER;
    setChannelsStep1();
    IFS10bits.ADCAN18IF = 0;
}

/**
 * Inicializa los controladores PID, estructuras del estado de las salidas,
 *  módulo PWM y ADC
 */
void smpsInit(void){   
    // Salidas de estado
    FAULT_TRIS = 0;
    FAULT_LAT = 0;
    PWR_GOOD_TRIS = 0;
    PWR_GOOD_LAT = 0;
    
    currentPowerSample = 0;
    
    // Estados de ambos Buck
    buckStatus.PID.integralTerm = 0;
    buckStatus.PID.kp = BUCK_KP;
    buckStatus.PID.ki = BUCK_KI;
    buckStatus.PID.n = 2;
    buckStatus.PID.measuredOutput = 0;
    buckStatus.PID.setpoint = 0;
    buckStatus.duty = BUCK_INITIAL_DUTY_CYCLE;
    buckStatus.phaseReg = BUCK_PHASE;
    buckStatus.pTrigger = 1568;
    buckStatus.deadTime = BUCK_DEAD_TIME;
    buckStatus.current = 0;
    buckStatus.currentLimit = BUCK_MAX_CURRENT;
    buckStatus.enablePID = BUCK_DEFAULT_PID_ENABLE;
    buckStatus.outputVoltage = 0;
    
    aux5VStatus.current = 0;
    aux5VStatus.currentLimit = MAX_CURRENT_5V;
    aux5VStatus.PID.measuredOutput = 0;
    aux5VStatus.outputVoltage = 0;
    
    aux3V3Status.current = 0;
    aux3V3Status.currentLimit = MAX_CURRENT_3V3;
    aux3V3Status.PID.measuredOutput = 0;
    aux3V3Status.outputVoltage = 0;
    
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
    IOCON2bits.OVRDAT = 0;
    IOCON1bits.OVRENH = 1;
    IOCON1bits.OVRENL = 1;
    
    _initADC();
    _initBuckPWM(&buckStatus);
    
    // Enciendo PWM el modulo PWM
    PTCONbits.PTEN = 1;
    IOCON1bits.OVRENH = 0;
    IOCON1bits.OVRENL = 0;
    
    __delay_us(100);            // Esperamos algunos ciclos para que
                                //  se estabilize el PWM
    
    // Habilitamos las salidas
    IOCON1bits.PENH = 1;
    IOCON1bits.PENL = 1;
    
    //setBuckVoltage(0); 
    auxEnable();
    
    ON_OFF_5V_TRIS = 0;
    PWR_GOOD_LAT = 1; 
}

void smpsTasks(void) {
    uint16_t n;
    double power = 0;
    
    // Cálculo de la potencia media
    if(currentPowerSample >= (INSTANTANEUS_POWER_BUFFER_SAMPLES - 1)) {
        for(n = 0; n < INSTANTANEUS_POWER_BUFFER_SAMPLES; ++n) {
            power += powerValues[n];
        }
        
        // Dividimos la sumatoria (área bajo la curva) por el intervalo de tiempo
        power /= (double)INSTANTANEUS_POWER_BUFFER_SAMPLES;
        buckStatus.averagePower = power;
        currentPowerSample = 0;
    }
}

void buckEnable() {
    buckStatus.PID.integralTerm = 0;
    buckStatus.overCurrent = 0;
    buckStatus.currentLimitFired = 0;
    buckStatus.enablePID = 1;
    buckStatus.enabled = 1;
    FAULT_LAT = 0;
    
    IOCON1bits.OVRENH = 0; 
    IOCON1bits.OVRENL = 0;
}

void auxEnable() {    
    aux5VStatus.overCurrent = 0;
    aux3V3Status.overCurrent = 0;
    aux5VStatus.currentLimitFired = 0;
    aux3V3Status.currentLimitFired = 0;
    
    aux3V3Status.enabled = 1;
    aux5VStatus.enabled = 1;
    
    ON_OFF_5V_LAT = 0;
    FAULT_LAT = 0;
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
    float v = (float)voltage * (float)BUCK_V_FEEDBACK_FACTOR;
    buckStatus.PID.setpoint = getMatchedVoltageADCValue((uint16_t)(v*((float)BUCK_ADC_COUNTS/(ADC_VREF*1000.0))));
}

/**
 * Configura el límite de corriente para el Buck 1
 * @param currentLimit límite de corriente en miliamperes
 */
void setBuckCurrentLimit(uint16_t currentLimit) {
    // Conversión de corriente de salida a tensión leída en el ADC
    float v = (float)BUCK_I_FEEDBACK_FACTOR * (float)currentLimit;
    buckStatus.currentLimit = (uint16_t)(v*((float)BUCK_ADC_COUNTS/(ADC_VREF*1000.0)));
    
    if(buckStatus.currentLimit > BUCK_MAX_CURRENT) {
        buckStatus.currentLimit = BUCK_MAX_CURRENT;
    }
}

/**
 * Configura el límite de corriente para la salida de 5V
 * @param currentLimit límite de corriente en miliamperes
 */
void set5VCurrentLimit(uint16_t currentLimit) {
    // Conversión de corriente de salida a tensión leída en el ADC
    float v = (float)AUX_5V_I_FEEDBACK_FACTOR * (float)currentLimit;
    aux5VStatus.currentLimit = (uint16_t)(v*((float)AUX_ADC_COUNTS/(ADC_VREF*1000.0)));
    
    if(aux5VStatus.currentLimit > MAX_CURRENT_5V) {
        aux5VStatus.currentLimit = MAX_CURRENT_5V;
    }
}

/**
 * Configura el límite de corriente para la salida de 3.3V
 * @param currentLimit límite de corriente en miliamperes
 */
void set3V3CurrentLimit(uint16_t currentLimit) {
    // Conversión de corriente de salida a tensión leída en el ADC
    float v = (float)AUX_3V3_I_FEEDBACK_FACTOR * (float)currentLimit;
    aux3V3Status.currentLimit = (uint16_t)(v*((float)AUX_ADC_COUNTS/(ADC_VREF*1000.0)));
    
    if(aux3V3Status.currentLimit > MAX_CURRENT_3V3) {
        aux3V3Status.currentLimit = MAX_CURRENT_3V3;
    }
}

uint16_t getMatchedVoltageADCValue(uint16_t adcValue) {
    return _getMatchedADCValue(adcValue, adcVoltageErrorTable, ADC_VOLTAGE_ERROR_TABLE_SIZE);
}

uint16_t getMatchedCurrentADCValue(uint16_t adcValue) {
    return _getMatchedADCValue(adcValue, adcCurrentErrorTable, ADC_CURRENT_ERROR_TABLE_SIZE);
}

uint16_t _getMatchedADCValue(uint16_t adcValue, const int16_t table[][2], uint16_t size) {
    uint16_t index;
    double newADCValue;
    
    // Encontramos primero el valor de ADC mas cercano en la tabla
    for(index = 0; index < size; ++index) {
        if(table[index][0] >= adcValue) {
            break;
        }
    }
    
    // Calculamos la corrección que debe ser aplicada (interpolando de ser
    //  necesario)
    if(table[index][0] == adcValue) {
        return adcValue + table[index][1];
    }
    // Interpolamos el error
    else {
        // interpolación
        double escala = (double)(table[index][1] - table[index-1][1]) / 
            (double)(table[index][0] - table[index-1][0]);
        newADCValue = ((double)table[index-1][1]) + escala * (double)(adcValue - table[index-1][0]);
        
        // Redondeo
        newADCValue = floor(newADCValue + 0.5);
        
        return adcValue + ((uint16_t)newADCValue);
    }
}

void _initBuckPWM(PowerSupplyStatus *data) {
    BUCK_PWMH_TRIS = BUCK_PWML_TRIS = 1;
    
    // Si se usa un debugger y se detiene en un breakpoint las salidas de PWM
    //  toman el estado de los registros LAT, por lo que las deshabilitamos
    //  para evitar cortocircuitos en el buck
    BUCK_PWMH_LAT = BUCK_PWML_LAT = 0;

    PWMCON1bits.FLTIEN = 0;     // Fault Interrupt deshabilitada
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
    FCLCON1bits.FLTMOD = 3;
    
    TRIG1 = data->pTrigger;     // Trigger principal
    //STRIG1 = 3000;            // Trigger secundario
    TRGCON1bits.DTM = 0;        // Generamos triggers principales y secundarios (no combinados)
    TRGCON1bits.TRGSTRT = 0;    // Esperamos 0 ciclos de PWM al iniciar el modulo antes
                                //  de comenzar a contar los ciclos para el trigger
    TRGCON1bits.TRGDIV = 4;     // Cada 5 periodos del PWM se genera un evento de
                                //  trigger

    IOCON1bits.PENH = 0;        // Pin PWMH deshabilitado
    IOCON1bits.PENL = 0;        // Pin PWML deshabilitado
    IOCON1bits.POLH = 0;        // PWMH activo alto
    IOCON1bits.POLL = 0;        // PWML activo alto
    IOCON1bits.PMOD = 0;        // PMW1 en modo Complementario (para Buck sincrónico)
    IOCON1bits.SWAP = 1;        // PWMH y PWML a sus respectivas salidas y no al revés
    IOCON1bits.OSYNC = 0;       // Override es asíncrono
    
    AUXCON1bits.HRPDIS = 0;     // Alta resolucion de periodo
    AUXCON1bits.HRDDIS = 0;     // Alta resolucion de duty
    AUXCON1bits.BLANKSEL = 0;   // Blanking deshabilitado
    AUXCON1bits.CHOPHEN = 0;    // Chop deshabilitado en PWMH
    AUXCON1bits.CHOPLEN = 0;    // Chop deshabilitado en PWML

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
    DTR1 = ALTDTR1 = 0;
}

void _initADC(void) {
    /**
     * Es importante que todos los cores del ADC esten configurados exactamente
     * igual, su fuente de trigger sea la misma e inicien la conversión al mismo
     * tiempo para poder hacer sampling simultáneo de otro modo el resultado de
     * la conversión será erroneo debido a un error en el modulo. Copia del errata:
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
    
    ANSELAbits.ANSA0 = 1;       // RA0 analogica
    ANSELAbits.ANSA1 = 1;       // RA1 analogica
    ANSELAbits.ANSA2 = 1;       // RA2 analogica
    ANSELBbits.ANSB0 = 1;       // RB0 analogica
    ANSELBbits.ANSB2 = 1;       // RB2 analogica
    ANSELBbits.ANSB3 = 1;       // RB3 analogica
    
    ADCON1Lbits.ADSIDL = 0;     // Modulo continua operando en modo IDLE
    ADCON1Hbits.FORM = 0;       // Dato de salida en formato integer (no fractional)
    ADCON1Hbits.SHRRES = 0b11;  // Resolucion de 12 bits
    
    ADCON2Lbits.REFCIE = 0;     // Interrupcion para band-gap y reference voltage ready desactivada
    ADCON2Lbits.REFERCIE = 0;   // Interrupcion para band-gap y reference voltage error desactivada
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
    ADCON4Lbits.SAMC0EN = 1;    // Luego del trigger se espera el tiempo de muestreo antes de iniciar la conversion en core 0
    ADCON4Lbits.SAMC1EN = 1;    // Luego del trigger se espera el tiempo de muestreo antes de iniciar la conversion en core 1
    ADCON4Lbits.SAMC2EN = 1;    // Luego del trigger se espera el tiempo de muestreo antes de iniciar la conversion en core 2
    ADCON4Lbits.SAMC3EN = 1;    // Luego del trigger se espera el tiempo de muestreo antes de iniciar la conversion en core 3
    
    // Todos los cores tienen un divisor 1:2 por lo que el clock en cada
    //  core es de 117.92MHz / 2 = 58.96 Mhz = Tad
    //
    // Conversion time  = 8 * Tcoresrc + (Resolution + 2.5) * Tadcore
    //                  = 8 * (1/117.92Mhz) + (12 + 2.5) * (1/58.96Mhz)
    //                  = 314 ns = 3.18 MSPS
    // Core 0
    ADCORE0Lbits.SAMC = 0;      // 2 Tad sample time
    ADCORE0Hbits.RES = 0;       // 12 bit de resolucion
    ADCORE0Hbits.ADCS = 0;      // Divisor 1:2 de clock
    // Core 1
    ADCORE1Lbits.SAMC = 0;      // 2 Tad sample time
    ADCORE1Hbits.RES = 0;       // 12 bit de resolucion
    ADCORE1Hbits.ADCS = 0;      // Divisor 1:2 de clock
    // Core 2
    ADCORE2Lbits.SAMC = 0;      // 2 Tad sample time
    ADCORE2Hbits.RES = 0;       // 12 bit de resolucion
    ADCORE2Hbits.ADCS = 0;      // Divisor 1:2 de clock
    // Core 3
    ADCORE3Lbits.SAMC = 0;      // 2 Tad sample time
    ADCORE3Hbits.RES = 0;       // 12 bit de resolucion
    ADCORE3Hbits.ADCS = 0;      // Divisor 1:2 de clock
    // Shared Core
    ADCON2Hbits.SHRSAMC = 0b0000000000; // Shared ADC 2 Tad sample time
    ADCON2Lbits.SHRADCS = 0b0000000;    // Divisor 1:2 de clock
    
    ADCON5Hbits.WARMTIME = 15;  // Maximo tiempo de encendido para los cores
    ADCON5Hbits.SHRCIE = 1;     // Interrupcion de Shared ADC Core ready activada
    ADCON5Hbits.C0C1E = 1;      // Interrupcion de ADC Core 0 ready activada
    ADCON5Hbits.C1C1E = 1;      // Interrupcion de ADC Core 1 ready activada
    ADCON5Hbits.C2C1E = 1;      // Interrupcion de ADC Core 2 ready activada
    ADCON5Hbits.C3C1E = 1;      // Interrupcion de ADC Core 3 ready activada
    
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
    
    // Calibracion para single-ended
    ADCAL0Lbits.CAL0EN = 1;     // Habilitamos modo calibracion
    ADCAL0Lbits.CAL1EN = 1;
    ADCAL0Hbits.CAL2EN = 1;
    ADCAL0Hbits.CAL3EN = 1;
    ADCAL1Hbits.CSHREN = 1;
    ADCAL0Lbits.CAL0DIFF = 0;   // Modo single-ended
    ADCAL0Lbits.CAL1DIFF = 0;
    ADCAL0Hbits.CAL2DIFF = 0;
    ADCAL0Hbits.CAL3DIFF = 0;
    ADCAL1Hbits.CSHRDIFF = 0;
    ADCAL0Lbits.CAL0RUN = 1;    // Inicio de calibracion
    ADCAL0Lbits.CAL1RUN = 1;
    ADCAL0Hbits.CAL2RUN = 1;
    ADCAL0Hbits.CAL3RUN = 1;
    ADCAL1Hbits.CSHRRUN = 1;
    
    // Esperamos a que la calibracion finalice
    while(!ADCAL0Lbits.CAL0RDY || !ADCAL0Lbits.CAL1RDY || !ADCAL0Hbits.CAL2RDY
            || !ADCAL0Hbits.CAL3RDY || !ADCAL1Hbits.CSHRRDY);
    
    ADCAL0Lbits.CAL0EN = 0;         // Deshabilitamos modo calibracion
    ADCAL0Lbits.CAL1EN = 0;
    ADCAL0Hbits.CAL2EN = 0;
    ADCAL0Hbits.CAL3EN = 0;
    ADCAL1Hbits.CSHREN = 0;
    
    // Configuracion de oversampling
    ADFL0CONbits.FLEN = 0;
    ADFL0CONbits.MODE = 0b00;       // Modo oversampling
    ADFL0CONbits.OVRSAM = 0b001;    // 16x oversampling (2 bit adicionales)
    ADFL0CONbits.FLCHSEL = 0;       // AN0 para el filtro digital 0
    ADFL0CONbits.IE = 1;            // Interrupcion habilitada
    ADFL0CONbits.FLEN = 1;
    
    ADFL1CONbits.FLEN = 0;
    ADFL1CONbits.MODE = 0b00;       // Modo oversampling
    ADFL1CONbits.OVRSAM = 0b001;    // 16x oversampling (2 bit adicionales)
    ADFL1CONbits.FLCHSEL = 1;       // AN1 para el filtro digital 1
    ADFL1CONbits.IE = 0;            // Interrupcion habilitada
    ADFL1CONbits.FLEN = 1;
    
    // Interrupciones filtros
    IFS11bits.ADFLTR0IF = 0;         // Borramos flag de interrupcion
    IFS11bits.ADFLTR1IF = 0;
    IPC44bits.ADFLTR0IP = 7;         // Prioridad maxima 7 para filter 0 (current buck)
    IPC45bits.ADFLTR1IP = 6;         // Prioridad 6 para filter 1 (voltage buck)
    IEC11bits.ADFLTR0IE = 1;         // Habilitamos interrupcion de los filtros
    IEC11bits.ADFLTR1IE = 1;         // Habilitamos interrupcion de los filtros
    
    // Interrupciones de canales
    // AN0 (buck current)
    IFS6bits.ADCAN0IF = 0;
    IEC6bits.ADCAN0IE = 0;
    IPC27bits.ADCAN0IP = 0;         // Este canal trabaja a traves del filtro unicamente
    
    // AN1 (buck voltage)
    IFS6bits.ADCAN1IF = 0;
    IEC6bits.ADCAN1IE = 0;
    IPC27bits.ADCAN1IP = 0;         // Este canal trabaja a traves del filtro unicamente
    
    // AN2 (5v current)
    IFS7bits.ADCAN2IF = 0;
    IEC7bits.ADCAN2IE = 1;
    IPC28bits.ADCAN2IP = 5;
    
    // AN3 (5v voltage)
    IFS7bits.ADCAN3IF = 0;
    IEC7bits.ADCAN3IE = 1;
    IPC28bits.ADCAN3IP = 5;
    
    // AN7 (3.3v current)
    IFS7bits.ADCAN7IF = 0;
    IEC7bits.ADCAN7IE = 1;
    IPC29bits.ADCAN7IP = 7;
    
    // AN18 (3.3v voltage)
    IFS10bits.ADCAN18IF = 0;
    IEC10bits.ADCAN18IE = 1;
    IPC40bits.ADCAN18IP = 6;
}
