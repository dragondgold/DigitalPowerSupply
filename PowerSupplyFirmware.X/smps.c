#include "smps.h"
#include "definitions.h"
#include <xc.h>
#include <libpic30.h>
#include <dsp.h>
#include <pps.h>
#include <stdint.h>
#include <math.h>
#include "pid.h"
#include "ds1307.h"

// Tablas de corrección del ADC
#define ADC_VOLTAGE_ERROR_TABLE_SIZE 22
static const int16_t adcVoltageErrorTable[][2] = { 
    { 0, 0 },       // { ADC Value, Cuanto sumar al valor del ADC obtenido }
    { 36/2, 3 },      // 1V
    { 70/2, 3 },      // 2V
    { 104/2, 3 },     // 3V
    { 138/2, 2 },     // 4V
    { 172/2, 2 },     // 5V
    { 205/2, 2 },     // Y asi sigue...
    { 239/2, 2 },
    { 273/2, 2 },
    { 306/2, 2 },
    { 340/2, 2 },
    { 372/2, 2 },
    { 406/2, 2 },
    { 440/2, 2 }, 
    { 473/2, 1 },
    { 508/2, 1 },
    { 539/2, 1 },
    { 573/2, -1},
    { 606/2, 0},
    { 641/2, -1},
    { 674/2, 0},
    { 1023/2, -1}
};

#define ADC_CURRENT_ERROR_TABLE_SIZE 22
static const int16_t adcCurrentErrorTable[][2] = { 
    { 0, 0 },       // { ADC Value, Cuanto sumar al valor del ADC obtenido }
    { 43/2, 30 },      
    { 79/2, 17 },      
    { 116/2, 27 },     
    { 155/2, 36 },     
    { 191/2, 26 },     
    { 228/2, 40 },     
    { 264/2, 59 },
    { 297/2, 63 },
    { 340/2, 73 },
    { 376/2, 69 },
    { 416/2, 69 },
    { 455/2, 73 },
    { 493/2, 72 }, 
    { 527/2, 69 },
    { 566/2, 68 },
    { 605/2, 68 },
    { 543/2, 63 },
    { 681/2, 62 },
    { 719/2, 60 },
    { 757/2, 54 },
    { 1023/2, 0}
};

// Estado de los buck
PowerSupplyStatus buck1Status;
PowerSupplyStatus buck2Status;
PowerSupplyStatus aux5VStatus;
PowerSupplyStatus aux3V3Status;

/**
 * Interrupción por fin de conversión del par 0. La interrupcion se genera
 *  cuando se convirtieron ambos canales del par.
 */
void __attribute__((__interrupt__, no_auto_psv)) _ADCP0Interrupt(void) {
    register int16_t output asm("w0");
    
    // Verificamos que no halla un exceso de corriente
    #ifdef ENABLE_CURRENT_PROTECTION
    if(B1_CURRENT_ADC_BUFFER > B1_MAX_CURRENT){
        buck1Emergency();
        buck1Status.overCurrent = 1;
        IFS6bits.ADCP0IF = 0;
        return;
    }
    else if(B1_CURRENT_ADC_BUFFER > buck1Status.currentLimit) {
        buck1Emergency();
        buck1Status.currentLimitFired = 1;
        IFS6bits.ADCP0IF = 0;
        return;
    }
    if(B1_CURRENT_ADC_BUFFER + buck2Status.current > MAX_BUCKS_SHARED_CURRENT) {
        buck1Emergency();
        buck2Emergency();
        buck1Status.overCurrent = 1;
        buck2Status.overCurrent = 1;
        IFS6bits.ADCP0IF = 0;
        return;
    }
    #endif
    buck1Status.current = B1_CURRENT_ADC_BUFFER;
    
    buck1Status.PID.measuredOutput = B1_VOLTAGE_ADC_BUFFER;
    buck1Status.outputVoltage = B1_VOLTAGE_ADC_BUFFER;
    if(buck1Status.enablePID) {
        output = myPI(&buck1Status.PID);

        if(output > B1_MAX_DUTY_CYCLE){
            output = B1_MAX_DUTY_CYCLE;
        }
        else if(output < B1_MIN_DUTY_CYCLE) {
            output = B1_MIN_DUTY_CYCLE;
        }
        setBuck1Duty(output);
    }

    IFS6bits.ADCP0IF = 0;       // Flag par 0
}

/**
 * Interrupción por fin de conversión del par 1. La interrupcion se genera
 *  cuando se convirtieron ambos canales del par.
 */
void __attribute__((__interrupt__, no_auto_psv)) _ADCP1Interrupt(void) {
    register int16_t output asm("w0");
    
    // Verificamos que no halla un exceso de corriente
    #ifdef ENABLE_CURRENT_PROTECTION
    if(B2_CURRENT_ADC_BUFFER > B2_MAX_CURRENT){
        buck2Emergency();
        buck2Status.overCurrent = 1;
        IFS6bits.ADCP1IF = 0;
        return;
    }
    else if(B2_CURRENT_ADC_BUFFER > buck2Status.currentLimit) {
        buck2Emergency();
        buck2Status.currentLimitFired = 1;
        IFS6bits.ADCP1IF = 0;
        return;
    }
    if(B2_CURRENT_ADC_BUFFER + buck1Status.current > MAX_BUCKS_SHARED_CURRENT) {
        buck1Emergency();
        buck2Emergency();
        buck1Status.overCurrent = 1;
        buck2Status.overCurrent = 1;
        IFS6bits.ADCP1IF = 0;
        return;
    }
    #endif
    buck2Status.current = B2_CURRENT_ADC_BUFFER;
    
    buck2Status.PID.measuredOutput = B2_VOLTAGE_ADC_BUFFER;
    buck2Status.outputVoltage = B2_VOLTAGE_ADC_BUFFER;
    if(buck2Status.enablePID) {
        output = myPI(&buck2Status.PID);

        if(output > B2_MAX_DUTY_CYCLE){
            output = B2_MAX_DUTY_CYCLE;
        }
        else if(output < B2_MIN_DUTY_CYCLE) {
            output = B2_MIN_DUTY_CYCLE;
        }
        setBuck2Duty(output);
    }
    
    IFS6bits.ADCP1IF = 0;       // Flag par 1
}

/**
 * Interrupción por fin de conversión del par 2. La interrupcion se genera
 *  cuando se convirtieron ambos canales del par.
 */
void __attribute__((__interrupt__, no_auto_psv)) _ADCP2Interrupt(void) {
    if(CURRENT_5V_ADC_BUFFER > MAX_CURRENT_5V){
        auxEmergency();
        aux5VStatus.overCurrent = 1;
        return;
    }
    else if(CURRENT_5V_ADC_BUFFER > aux5VStatus.currentLimit) {
        auxEmergency();
        aux5VStatus.currentLimitFired = 1;
        IFS7bits.ADCP2IF = 0;
        return;
    }
    
    if(CURRENT_3V3_ADC_BUFFER > MAX_CURRENT_3V3){
        auxEmergency();
        aux3V3Status.overCurrent = 1;
        return;
    }
    else if(CURRENT_3V3_ADC_BUFFER > aux3V3Status.currentLimit) {
        auxEmergency();
        aux3V3Status.currentLimitFired = 1;
        IFS7bits.ADCP2IF = 0;
        return;
    }
    
    aux3V3Status.current = CURRENT_3V3_ADC_BUFFER;
    aux5VStatus.current = CURRENT_5V_ADC_BUFFER;

    IFS7bits.ADCP2IF = 0;       // Flag par 2
}

/**
 * Interrupción por fin de conversión del par 3. La interrupcion se genera
 *  cuando se convirtieron ambos canales del par.
 */
void __attribute__((__interrupt__, no_auto_psv)) _ADCP3Interrupt(void) {
    aux5VStatus.outputVoltage = VOLTAGE_5V_ADC_BUFFER;
    aux3V3Status.outputVoltage = VOLTAGE_3V3_ADC_BUFFER;    
    
    IFS7bits.ADCP3IF = 0;       // Flag par 3
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
    
    // Estados de ambos Buck
    buck1Status.PID.integralTerm = 0;
    buck1Status.PID.kp = B1Kp;
    buck1Status.PID.ki = B1Ki;
    buck1Status.PID.n = 2;
    buck1Status.PID.measuredOutput = 0;
    buck1Status.PID.setpoint = _loadSetpoint(B1_SETPOINT_RAM_POS_L, B1_SETPOINT_RAM_POS_H);
    buck1Status.duty = B1_INITIAL_DUTY_CYCLE;
    buck1Status.deadTime = B1_DEAD_TIME;
    buck1Status.current = 0;
    buck1Status.currentLimit = B1_MAX_CURRENT;
    buck1Status.enablePID = B1_DEFAULT_PID_ENABLE;
    buck1Status.outputVoltage = 0;
    
    buck2Status.PID.integralTerm = 0;
    buck2Status.PID.kp = B2Kp;
    buck2Status.PID.ki = B2Ki;
    buck2Status.PID.n = 2;
    buck2Status.PID.measuredOutput = 0;
    buck2Status.PID.setpoint = _loadSetpoint(B2_SETPOINT_RAM_POS_L, B2_SETPOINT_RAM_POS_H);
    buck2Status.duty = B2_INITIAL_DUTY_CYCLE;
    buck2Status.deadTime = B2_DEAD_TIME;
    buck2Status.current = 0;
    buck2Status.currentLimit = B2_MAX_CURRENT;
    buck2Status.enablePID = B2_DEFAULT_PID_ENABLE;
    buck2Status.outputVoltage = 0;
    
    aux5VStatus.current = 0;
    aux5VStatus.currentLimit = MAX_CURRENT_5V;
    aux5VStatus.PID.measuredOutput = 0;
    
    aux3V3Status.current = 0;
    aux3V3Status.currentLimit = MAX_CURRENT_3V3;
    aux3V3Status.PID.measuredOutput = 0;

    // Configuracion general del modulo PWM
    PTCONbits.PTEN = 0;         // Modulo PWM apagado
    PTCONbits.PTSIDL = 0;       // PWM continua en modo IDLE
    PTCONbits.SEIEN = 0;        // Interrupcion por evento especial desactivada
    PTCONbits.EIPU = 0;         // El periodo del PWM se actualiza al final el periodo actual
    PTCONbits.SYNCPOL = 0;      // El estado activo es 1 (no invertido)
    PTCONbits.SYNCOEN = 0;      // Salida de sincronismo apagado
    PTCONbits.SYNCEN = 0;       // Entrada de sincronismo apagada
    PTCON2bits.PCLKDIV = 0;     // Divisor del PWM en 1:1
    
    // Override
    IOCON1bits.OVRDAT = 0;
    IOCON2bits.OVRDAT = 0;
    IOCON1bits.OVRENH = 1; IOCON1bits.OVRENL = 1;
    IOCON2bits.OVRENH = 1; IOCON2bits.OVRENL = 1;
    
    _initADC();
    _initBuck1();
    _initBuck2();
    
    // Enciendo PWM el modulo PWM
    PTCONbits.PTEN = 1;
    IOCON1bits.OVRENH = 0; IOCON1bits.OVRENL = 0;
    IOCON2bits.OVRENH = 0; IOCON2bits.OVRENL = 0;
    
    __delay_us(100);            // Esperamos algunos ciclos para que
                                //  se estabilize el PWM
    
    // Habilitamos las salidas
    IOCON1bits.PENH = 1;
    IOCON1bits.PENL = 1;
    IOCON2bits.PENH = 1;
    IOCON2bits.PENL = 1;
    setBuck2Voltage(0);
    
    auxEnable();
    ON_OFF_5V_TRIS = 0;
    PWR_GOOD_LAT = 1;
}

void smpsTasks(void) {
    // Guardamos los setpoint en la memoria RAM del RTC para poder cargarlos
    //  en el nuevo encendido
    if(buck1Status.dirtySetpoint) {
        _saveSetpoint(buck1Status.PID.setpoint, B1_SETPOINT_RAM_POS_L, B1_SETPOINT_RAM_POS_H);
        buck1Status.dirtySetpoint = 0;
    }
    else if(buck2Status.dirtySetpoint) {
        _saveSetpoint(buck1Status.PID.setpoint, B2_SETPOINT_RAM_POS_L, B2_SETPOINT_RAM_POS_H);
        buck1Status.dirtySetpoint = 0;
    }
}

void buck1Enable() {
    buck1Status.PID.integralTerm = 0;
    buck1Status.overCurrent = 0;
    buck1Status.currentLimitFired = 0;
    buck1Status.enablePID = 1;
    buck1Status.enabled = 1;
    FAULT_LAT = 0;
    
    IOCON1bits.OVRENH = 0; 
    IOCON1bits.OVRENL = 0;
}

void buck2Enable() {
    buck2Status.PID.integralTerm = 0;
    buck2Status.overCurrent = 0;
    buck2Status.currentLimitFired = 0;
    buck2Status.enablePID = 1;
    buck2Status.enabled = 1;
    FAULT_LAT = 0;
    
    IOCON2bits.OVRENH = 0; 
    IOCON2bits.OVRENL = 0;
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

void buck1Disable(void) {
    IOCON1bits.OVRENH = 1; 
    IOCON1bits.OVRENL = 1;
    
    buck1Status.enablePID = 0;
    buck1Status.enabled = 0;
}

void buck2Disable(void) {
    IOCON2bits.OVRENH = 1; 
    IOCON2bits.OVRENL = 1;
    
    buck2Status.enablePID = 0;
    buck2Status.enabled = 0;
}

void auxDisable(void) {
    ON_OFF_5V_LAT = 1;
    
    aux3V3Status.enabled = 0;
    aux5VStatus.enabled = 0;
}

/**
 * Setea la tensión de salida del Buck 1
 * @param voltage tensión de salida en milivolts
 */
void setBuck1Voltage(uint16_t voltage) {
    // Convertimos la tensión de salida en milivolts deaseada al valor
    //  correspondiente del ADC que debería ser leído aplicando el factor
    //  de corrección
    float v = (float)voltage * (float)B1_V_FEEDBACK_FACTOR;
    buck1Status.PID.setpoint = getMatchedVoltageADCValue((uint16_t)(v*((float)ADC_COUNTS/(ADC_VREF*1000.0))));
    buck1Status.dirtySetpoint = 1;
}

/**
 * Setea la tensión de salida del Buck 2
 * @param voltage tensión de salida en milivolts
 */
void setBuck2Voltage(uint16_t voltage) {
    // Convertimos la tensión de salida en milivolts deaseada al valor
    //  correspondiente del ADC que debería ser leído aplicando el factor
    //  de corrección
    float v = (float)voltage * (float)B2_V_FEEDBACK_FACTOR;
    buck2Status.PID.setpoint = getMatchedVoltageADCValue((uint16_t)(v*((float)ADC_COUNTS/(ADC_VREF*1000.0)))) - 4;
    Nop();
    buck2Status.dirtySetpoint = 1;
}

/**
 * Setea el límite de corriente para el Buck 1
 * @param currentLimit límite de corriente en miliamperes
 */
void setBuck1CurrentLimit(uint16_t currentLimit) {
    // Conversión de corriente de salida a tensión leída en el ADC
    float v = (float)B1_I_FEEDBACK_FACTOR * (float)currentLimit;
    buck1Status.currentLimit = (uint16_t)(v*((float)ADC_COUNTS/(ADC_VREF*1000.0)));
    
    if(buck1Status.currentLimit > B1_MAX_CURRENT) {
        buck1Status.currentLimit = B1_MAX_CURRENT;
    }
}

/**
 * Setea el límite de corriente para el Buck 1
 * @param currentLimit límite de corriente en miliamperes
 */
void setBuck2CurrentLimit(uint16_t currentLimit) {
    // Conversión de corriente de salida a tensión leída en el ADC
    float v = (float)B2_I_FEEDBACK_FACTOR * (float)currentLimit;
    buck2Status.currentLimit = (uint16_t)(v*((float)ADC_COUNTS/(ADC_VREF*1000.0)));
    
    if(buck2Status.currentLimit > B2_MAX_CURRENT) {
        buck2Status.currentLimit = B2_MAX_CURRENT;
    }
}

/**
 * Setea el límite de corriente para la salida de 5V
 * @param currentLimit límite de corriente en miliamperes
 */
void set5VCurrentLimit(uint16_t currentLimit) {
    // Conversión de corriente de salida a tensión leída en el ADC
    float v = (float)AUX_5V_I_FEEDBACK_FACTOR * (float)currentLimit;
    aux5VStatus.currentLimit = (uint16_t)(v*((float)ADC_COUNTS/(ADC_VREF*1000.0)));
    
    if(aux5VStatus.currentLimit > MAX_CURRENT_5V) {
        aux5VStatus.currentLimit = MAX_CURRENT_5V;
    }
}

/**
 * Setea el límite de corriente para la salida de 3.3V
 * @param currentLimit límite de corriente en miliamperes
 */
void set3V3CurrentLimit(uint16_t currentLimit) {
    // Conversión de corriente de salida a tensión leída en el ADC
    float v = (float)AUX_3V3_I_FEEDBACK_FACTOR * (float)currentLimit;
    aux3V3Status.currentLimit = (uint16_t)(v*((float)ADC_COUNTS/(ADC_VREF*1000.0)));
    
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
    
    // Calculamos la correccion que debe ser aplicada (interpolando de ser
    //  necesario)
    if(table[index][0] == adcValue) {
        return adcValue + table[index][1];
    }
    // Interpolamos el error
    else {
        // Interpolacion
        double escala = (double)(table[index][1] - table[index-1][1]) / 
            (double)(table[index][0] - table[index-1][0]);
        newADCValue = ((double)table[index-1][1]) + escala * (double)(adcValue - table[index-1][0]);
        
        // Redondeo
        newADCValue = floor(newADCValue + 0.5);
        
        return adcValue + ((uint16_t)newADCValue);
    }
}

void _initBuck1(void) {
    B1_PWMH_TRIS = B1_PWML_TRIS = 1;

    PWMCON1bits.FLTIEN = 0;     // Fault Interrupt deshabilitada
    PWMCON1bits.CLIEN = 1;      // Current-limit Interrupt habilitada
    PWMCON1bits.TRGIEN = 0;     // No generar interrupción en trigger
    PWMCON1bits.ITB = 1;        // Independent Time Base. En Push-Pull controlamos
                                //  el periodo con los registros PHASEx/SPHASEx
    PWMCON1bits.MDCS = 0;       // Los registros PDC1/SDC1 determinan el duty cycle
                                //  del PWM1, no usamos un duty cycle global (MDC)
    PWMCON1bits.DTC = 0;        // Dead-time positivo
    PWMCON1bits.CAM = 0;        // Center-Aligned deshabilitado
    PWMCON1bits.XPRES = 0;      // Los pines externos por sobre-corriente deshabilitados
    PWMCON1bits.IUE = 0;        // Los cambios en el duty son sincronizados con la base
                                //  de tiempo del PWM (no son inmediatos)
    
    TRIG1 = 1500;               // Trigger principal
    STRIG1 = 3000;              // Trigger secundario
    TRGCON1bits.DTM = 0;        // Generamos triggers principales y secundarios
    TRGCON1bits.TRGSTRT = 1;    // Esperamos 1 ciclo de PWM al iniciar el modulo antes
                                //  de comenzar a contar los ciclos para el trigger de
                                //  este modo el muestreo es intercalado con el buck 2
    TRGCON1bits.TRGDIV = 1;     // Cada 1 periodo del PWM se genera un evento de
                                //  trigger

    IOCON1bits.PENH = 0;        // Pin PWMH deshabilitado
    IOCON1bits.PENL = 0;        // Pin PWML deshabilitado
    IOCON1bits.POLH = 0;        // PWMH activo alto
    IOCON1bits.POLL = 0;        // PWML activo alto
    IOCON1bits.PMOD = 0;        // PMW1 en modo Complementario (Para Buck Sincronico)
    IOCON1bits.SWAP = 0;        // PWMH y PWML a sus respectivas salidas y no al reves
    IOCON1bits.OSYNC = 0;       // Override es asincrono

    /*
     * Período del PWM viene dado por:
     *  ( (ACLK * 8 * PWM_PERIOD)/(PMW_INPUT_PREESCALER) ) - 8
     *
     * Para una frecuencia de 100KHz con preescaler 1:1 y el ACLK en 117.92MHz
     */
    PHASE1 = B1_PHASE1;

    /*
     * El duty cycle viene dado por:
     *  (ACLK * 8 * PWM_DUTY)/(PMW_INPUT_PREESCALER)
     *
     * La resolución del duty es:
     *  log2( (ACLK * 8 * PWM_PERIOD)/(PMW_INPUT_PREESCALER) ) = 13 bits
     *
     */
    PDC1 = SDC1 = buck1Status.duty;

    /*
     * El Dead-time viene dado por:
     *  (ACLK * 8 * DEAD_TIME)/(PMW_INPUT_PREESCALER)
     */
    DTR1 = buck1Status.deadTime;                 // Dead-time para PWMH
    ALTDTR1 = buck1Status.deadTime;              // Dead-time para PWML
}

void _initBuck2(void) {
    B2_PWMH_TRIS = B2_PWML_TRIS = 1;

    PWMCON2bits.FLTIEN = 0;     // Fault Interrupt deshabilitada
    PWMCON2bits.CLIEN = 1;      // Current-limit Interrupt habilitada
    PWMCON2bits.TRGIEN = 0;     // No generar interrupción en trigger
    PWMCON2bits.ITB = 1;        // Independent Time Base. En Push-Pull controlamos
                                //  el periodo con los registros PHASEx/SPHASEx
    PWMCON2bits.MDCS = 0;       // Los registros PDC1/SDC1 determinan el duty cycle
                                //  del PWM1, no usamos un duty cycle global (MDC)
    PWMCON2bits.DTC = 0;        // Dead-time positivo
    PWMCON2bits.CAM = 0;        // Center-Aligned deshabilitado
    PWMCON2bits.XPRES = 0;      // Los pines externos por sobre-corriente deshabilitados
    PWMCON2bits.IUE = 0;        // Los cambios en el duty son sincronizados con la base
                                //  de tiempo del PWM (no son inmediatos)
    
    TRIG2 = 1500;               // Trigger principal
    STRIG2 = 3000;              // Trigger secundario
    TRGCON2bits.DTM = 0;        // Generamos triggers principales y secundarios
    TRGCON2bits.TRGSTRT = 0;    // Esperamos 0 ciclos de PWM al iniciar el modulo antes
                                //  de comenzar a contar los ciclos para el trigger
    TRGCON2bits.TRGDIV = 1;     // Cada periodo del PWM se genera un evento de
                                //  trigger

    IOCON2bits.PENH = 0;        // Pin PWMH deshabilitado
    IOCON2bits.PENL = 0;        // Pin PWML deshabilitado
    IOCON2bits.POLH = 0;        // PWMH activo alto
    IOCON2bits.POLL = 0;        // PWML activo alto
    IOCON2bits.PMOD = 0;        // PMW1 en modo Complementario (Para Buck Sincronico)
    IOCON2bits.SWAP = 0;        // PWMH y PWML a sus respectivas salidas y no al reves
    IOCON2bits.OSYNC = 0;       // Override es asincrono

    /*
     * Período del PWM viene dado por:
     *  ( (ACLK * 8 * PWM_PERIOD)/(PMW_INPUT_PREESCALER) ) - 8
     *
     * Para una frecuencia de 100KHz con preescaler 1:1 y el ACLK en 117.92MHz
     */
    PHASE2 = B2_PHASE1;

    /*
     * El duty cycle viene dado por:
     *  (ACLK * 8 * PWM_DUTY)/(PMW_INPUT_PREESCALER)
     *
     * La resolución del duty es:
     *  log2( (ACLK * 8 * PWM_PERIOD)/(PMW_INPUT_PREESCALER) ) = 13 bits
     *
     */
    PDC2 = SDC2 = buck2Status.duty;

    /*
     * El Dead-time viene dado por:
     *  (ACLK * 8 * DEAD_TIME)/(PMW_INPUT_PREESCALER)
     */
    DTR2 = buck2Status.deadTime;                 // Dead-time para PWMH
    ALTDTR2 = buck2Status.deadTime;              // Dead-time para PWML
}

void _initADC(void) {
    ADCONbits.ADON = 0;
    ADCONbits.ADSIDL = 0;       // Modulo continua operando en modo IDLE
    ADCONbits.SLOWCLK = 1;      // El clock proviene de ACLK (Clock auxiliar)
    ADCONbits.FORM = 0;         // Dato de salida en formato integer (no fractional)
    ADCONbits.EIE = 0;          // La interrupción se genera cuando se convirtieron los dos canales del par
    ADCONbits.ORDER = 0;        // Primero se convierte la entrada par y luego la impar
    ADCONbits.SEQSAMP = 0;      // Ambos canales del pair se muestrean al mismo tiempo (el dedicado y el shared)
    ADCONbits.ASYNCSAMP = 0;
    ADCONbits.ADCS = 5;         // Divide el clock proveniente de ACLK en 6. El ACKL
                                //  es de 117.92 MHz por lo que el Tad = 41.66ns

    // Par 3 del ADC (Tensiones auxiliares)
    ADCPC1bits.IRQEN3 = 1;          // Interrupción cuando el par AN6 y AN7 termina su conversión
    ADCPC1bits.TRGSRC3 = 0b01111;   // Trigger secundario de PWM 2
    
    // Par 2 del ADC (Corrientes auxiliares)
    ADCPC1bits.IRQEN2 = 1;          // Interrupción cuando el par AN4 y AN5 termina su conversión
    ADCPC1bits.TRGSRC2 = 0b01110;   // Trigger secundario de PWM 1

    // Par 1 del ADC (Corriente y tension Buck 2)
    ADCPC0bits.IRQEN1 = 1;          // Interrupción cuando el par AN2 y AN3 termina su conversión
    ADCPC0bits.TRGSRC1 = 0b00101;   // Trigger principal de PWM 2

    // Par 0 del ADC (Corriente y tension Buck 1)
    ADCPC0bits.IRQEN0 = 0;          // Interrupción cuando el par AN0 y AN1 termina su conversión
    ADCPC0bits.TRGSRC0 = 0b00100;   // Trigger principal de PWM 1

    // Configuramos que pines son digitales (1) y cuales analogos (0)
    ADPCFG = 0xFFFF;
    ADPCFGbits.PCFG0 = 0;       // Corriente Buck 1
    ADPCFGbits.PCFG1 = 0;       // Tension Buck 1
    ADPCFGbits.PCFG2 = 0;       // Corriente Buck 2
    ADPCFGbits.PCFG3 = 0;       // Tension Buck 2
    ADPCFGbits.PCFG4 = 0;       // Corriente 5V
    ADPCFGbits.PCFG5 = 0;       // Corriente 3.3V
    ADPCFGbits.PCFG6 = 0;       // Tension 5V
    ADPCFGbits.PCFG7 = 0;       // Tension 3.3V

    IPC27bits.ADCP0IP = 7;      // Par 0 prioridad maxima (Buck 1)
    IPC27bits.ADCP1IP = 6;      // Par 1 (Buck 2)
    IPC28bits.ADCP2IP = 5;      // Par 2 (Corrientes auxiliares)
    IPC28bits.ADCP3IP = 4;      // Par 3 (Tension auxiliares)

    IFS7bits.ADCP3IF = 0;       // Borramos los flags de los pares
    IFS7bits.ADCP2IF = 0;       
    IFS6bits.ADCP1IF = 0;
    IFS6bits.ADCP0IF = 0;

    IEC6bits.ADCP0IE = 1;       // Habilitamos la interrupcion de los pares
    IEC6bits.ADCP1IE = 1;
    IEC7bits.ADCP2IE = 1;
    IEC7bits.ADCP3IE = 1;
    
    ADCONbits.ADON = 1;         // Encendemos el ADC
    __delay_us(10);             // El ADC necesita un tiempo de 10us para estabilización
                                //  cuando se enciende
}

void _saveSetpoint(uint16_t setpoint, uint8_t lowPos, uint8_t highPos) {
    writeGpRAM((uint8_t)(setpoint & 0x00FF), lowPos);
    writeGpRAM((uint8_t)((setpoint & 0xFF00) >> 8), highPos);
}

uint16_t _loadSetpoint(uint8_t lowPos, uint8_t highPos) {
    uint16_t setpoint;
    uint8_t data;
    
    if(readGpRAM(highPos, &data) > 0){
        setpoint = data;
        
        setpoint = setpoint << 8;
        if(readGpRAM(lowPos, &data) > 0){
            setpoint += data;
            
            if(setpoint > 1023) setpoint = 1023;
            return setpoint;
        }
    }
    return 0;
}
