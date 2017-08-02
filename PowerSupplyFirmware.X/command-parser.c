#include "command-parser.h"
#include "smps.h"
#include "definitions.h"
#include <xc.h>
#include <uart.h>
#include <libpic30.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

static char buffer[100] = {0};

// External variables
extern PowerSupplyStatus buckStatus;
extern PowerSupplyStatus aux5VStatus;
extern PowerSupplyStatus aux3V3Status;

static float v;
static uint16_t status, a;

void initCommandParser(void) {
    RX_TRIS = 1;
    TX_TRIS = 0;
    
    U1MODEbits.UARTEN = 0;
    U1MODEbits.IREN = 0;                // IrDA deshabilitado
    U1MODEbits.RTSMD = 1;               // Modo Simplex
    U1MODEbits.UEN = 0b00;              // Solo usamos pines RX y TX
    U1MODEbits.LPBACK = 0;              // Loopback desactivado
    U1MODEbits.ABAUD = 0;               // Autobaud desactivado
    U1MODEbits.URXINV = 0;              // No invertimos pin RX
    U1MODEbits.PDSEL = 0b00;            // 8-bit, sin paridad
    U1MODEbits.STSEL = 0;               // 1 bit de stop
    U1MODEbits.BRGH = 0;
    U1BRG = 37;                         // 115200 baudios a 70 MIPS
    
    U1STAbits.URXISEL = 0;
    U1STAbits.ADDEN = 0;
    U1STAbits.UTXINV = 0;               // No invertimos pin TX
    U1STAbits.UTXBRK = 0;               // Sync Break desactivado
    U1STAbits.URXISEL = 0b00;           // Interrupción al recibir solo un caracter
    U1STAbits.UTXEN = 0;
    
    U1MODEbits.UARTEN = 1;              // UART habilitado
    U1STAbits.UTXEN = 1;                // TX habilitado (debe habilitarse después
                                        // de haber habilitado el modulo)
    
    __delay_us(10);
    
    // Configuramos el timer para realizar el envío de datos cada cierto tiempo
    T1CONbits.TON = 0;
    T1CONbits.TSIDL = 0;
    T1CONbits.TGATE = 0;
    T1CONbits.TCKPS = 0b11;             // Prescaler 1:256
    T1CONbits.TCS = 0;
    
    TMR1 = TIMER_VALUE;
    IFS0bits.T1IF = 0;
    T1CONbits.TON = 1;
}

void commandParserTasks(void) {
    if(U1STAbits.OERR) U1STAbits.OERR = 0;
    if(DataRdyUART1()) {
        _decodeCommand(_waitAndReceive());
    }
    
    // En el desborde del timer enviamos los datos
    if(IFS0bits.T1IF) {
        _constructDataString(buffer);
        WriteUART1(ACK);
        putsUART1((unsigned int *)buffer);
        WriteUART1('\0');
        
        TMR1 = TIMER_VALUE;
        IFS0bits.T1IF = 0;
    }
}

uint16_t _waitAndReceive(void) {
    if(U1STAbits.OERR) U1STAbits.OERR = 0;
    while(!DataRdyUART1());
    return ReadUART1();
}

int16_t _readStringWithTimeout(uint16_t timeout) {
    uint16_t time = timeout;
    uint16_t index = 0;
    uint16_t maxSize = sizeof(buffer);
    
    do{
        while(!DataRdyUART1()){
            __delay_us(10);
            --time;
            if(time == 0){
                return -1;
            }
        }
        buffer[index] = ReadUART1();
        
        if(index == maxSize-1){
            return -2;
        }
    } while(buffer[index++] != '\0');

    return 0;
}

/**
 * Tensión del buck en volts
 * @return Tensión del buck en volts
 */
float getBuckVoltage() {
    return (((float)getMatchedVoltageADCValue(buckStatus.outputVoltage))*(ADC_VREF/(float)BUCK_ADC_COUNTS)) / (float)BUCK_V_FEEDBACK_FACTOR;
}

/**
 * Corriente del buck en amperes
 * @return Corriente del buck en amperes
 */
float getBuckCurrent() {
    return (((float)getMatchedCurrentADCValue(buckStatus.current))*(ADC_VREF/(float)BUCK_ADC_COUNTS)) / (float)BUCK_I_FEEDBACK_FACTOR;
}

/**
 * Potencia del buck en Watts
 * @return Potencia del buck en Watts
 */
float getBuckPower() {
    return (float)buckStatus.averagePower * (float)BUCK_W_FACTOR;
}

/**
 * Corriente límite del buck en amperes
 * @return Corriente límite del buck en amperes
 */
float getBuckCurrentLimit() {
    return ((float)buckStatus.currentLimit*(ADC_VREF / (float)BUCK_ADC_COUNTS)) / (float)BUCK_I_FEEDBACK_FACTOR;
}

/**
 * Tensión de la línea de 5V en volts
 * @return Tensión de la línea de 5V en volts
 */
float get5VVoltage() {
    return (((float)aux5VStatus.outputVoltage)*(ADC_VREF/(float)AUX_ADC_COUNTS)) / (float)AUX_5V_V_FEEDBACK_FACTOR;
}

/**
 * Corriente de la línea de 5V en amperes
 * @return Corriente de la línea de 5V en amperes
 */
float get5VCurrent() {
    return (((float)getMatchedCurrentADCValue(aux5VStatus.current))*((ADC_VREF)/(float)AUX_ADC_COUNTS)) / (float)AUX_5V_I_FEEDBACK_FACTOR;
}

/**
 * Potencia de la línea de 5V en Watts
 * @return Potencia de la línea de 5V en Watts 
 */
float get5VPower() {
    return (float)aux5VStatus.averagePower * (float)AUX_5V_W_FACTOR;
}

/**
 * Corriente límite de la línea de 5V en amperes
 * @return Corriente límite de la línea de 5V en amperes
 */
float get5VCurrentLimit() {
    return ((float)aux5VStatus.currentLimit*(ADC_VREF / (float)AUX_ADC_COUNTS)) / (float)AUX_5V_I_FEEDBACK_FACTOR;
}

/**
 * Tensión de la línea de 3.3V en volts
 * @return Tensión de la línea de 3.3V en volts
 */
float get3V3Voltage() {
    return (((float)aux3V3Status.outputVoltage)*(ADC_VREF/(float)AUX_ADC_COUNTS)) / (float)AUX_3V3_V_FEEDBACK_FACTOR;
}

/**
 * Corriente de la línea de 3.3V en amperes
 * @return Corriente de la línea de 3.3V en amperes
 */
float get3V3Current() {
    return (((float)getMatchedCurrentADCValue(aux3V3Status.current))*((ADC_VREF)/(float)AUX_ADC_COUNTS)) / (float)AUX_3V3_I_FEEDBACK_FACTOR;
}

/**
 * Potencia de la línea de 3.3V en Watts
 * @return Potencia de la línea de 3.3V en Watts 
 */
float get3V3Power() {
    return (float)aux3V3Status.averagePower * (float)AUX_3V3_W_FACTOR;
}

/**
 * Corriente límite de la línea de 3.3V en amperes
 * @return Corriente límite de la línea de 3.3V en amperes
 */
float get3V3CurrentLimit() {
    return ((float)aux3V3Status.currentLimit*(ADC_VREF / (float)AUX_ADC_COUNTS)) / (float)AUX_3V3_I_FEEDBACK_FACTOR;
}

void _constructDataString(char *buffer) {
    sprintf(buffer, "%.2f;%.3f;%.2f;%.3f;%.2f;%.3f;%.2f;%.3f;%.2f;%.3f;%.2f;%.3f", 
            getBuckVoltage(), getBuckCurrent(), getBuckPower(), getBuckCurrentLimit(),
            get5VVoltage(), get5VCurrent(), get5VPower(), get5VCurrentLimit(),
            get3V3Voltage(), get3V3Current(), get3V3Power(), get3V3CurrentLimit());
}

void _decodeCommand(uint16_t command){
    switch (command){
        case SET_BUCK_VOLTAGE:
            WriteUART1(ACK);
            status = _readStringWithTimeout(TIMEOUT);
            if(status == 0) {
                double v = atof(buffer);
                setBuckVoltage((uint16_t)(v*1000.0));
            }
            break;
            
        case GET_BUCK_VOLTAGE:
            WriteUART1(ACK);     
            v = getBuckVoltage(buckStatus);
            sprintf(buffer, "%.2f", v);
            putsUART1((unsigned int *)buffer);
            WriteUART1('\0');
            break;
            
        case GET_BUCK_CURRENT:
            WriteUART1(ACK);
            v = getBuckCurrent(buckStatus);
            sprintf(buffer, "%.2f", v);
            putsUART1((unsigned int *)buffer);
            WriteUART1('\0');
            break;
            
        case GET_BUCK_POWER:
            WriteUART1(ACK);
            sprintf(buffer, "%.2f", getBuckPower());
            putsUART1((unsigned int *)buffer);
            WriteUART1('\0');
            break;
            
        case DISABLE_BUCK_PID:
            WriteUART1(ACK);
            buckStatus.enablePID = 0;
            break;
            
        case ENABLE_BUCK_PID:
            WriteUART1(ACK);
            buckStatus.PID.integralTerm = 0;
            buckStatus.enablePID = 1;
            break;
            
        case SET_BUCK_KP:
            WriteUART1(ACK);
            buckStatus.enablePID = 0;
            status = _readStringWithTimeout(TIMEOUT);
            if(status == 0) {
                double kp = atof(buffer);
                buckStatus.PID.kp = kp;
            }
            buckStatus.PID.integralTerm = 0;
            buckStatus.enablePID = 1;
            break;
            
        case SET_BUCK_KI:
            WriteUART1(ACK);
            buckStatus.enablePID = 0;
            status = _readStringWithTimeout(TIMEOUT);
            if(status == 0) {
                double ki = atof(buffer);
                buckStatus.PID.ki = ki;
            }
            buckStatus.PID.integralTerm = 0;
            buckStatus.enablePID = 1;
            break;
            
        case GET_5V_VOLTAGE:
            WriteUART1(ACK);
            v = get5VVoltage();
            sprintf(buffer, "%.2f", v);
            putsUART1((unsigned int *)buffer);
            WriteUART1('\0');
            break;
            
        case GET_5V_CURRENT:
            WriteUART1(ACK);
            a = (uint16_t)get5VCurrent();
            
            // No mostramos mas de 3 digitos
            if(a > 999) a = 999;
            sprintf(buffer, "%u", a);
            WriteUART1('.');
            if(a < 100) WriteUART1('0');
            if(a < 10) WriteUART1('0');
            putsUART1((unsigned int *)buffer);
            WriteUART1('\0');
            break;
            
        case GET_3V3_VOLTAGE:
            WriteUART1(ACK);
            v = get3V3Voltage();
            sprintf(buffer, "%.2f", v);
            putsUART1((unsigned int *)buffer);
            WriteUART1('\0');
            break;
            
        case GET_3V3_CURRENT:
            WriteUART1(ACK);
            a = (uint16_t)get3V3Current();
            
            // No mostramos mas de 3 digitos
            if(a > 999) a = 999;
            sprintf(buffer, "%u", a);
            WriteUART1('.');
            if(a < 100) WriteUART1('0');
            if(a < 10) WriteUART1('0');
            putsUART1((unsigned int *)buffer);
            WriteUART1('\0');
            break;

        case SHUTDOWN:
            WriteUART1(ACK);
            buckDisable();
            auxDisable();
            break;
          
        case SET_BUCK_CURRENT_LIMIT:
            WriteUART1(ACK);
            status = _readStringWithTimeout(TIMEOUT);
            if(status == 0) {
                double i = atof(buffer);
                setBuckCurrentLimit((uint16_t)(i*1000.0));
            }
            break;
            
        case SET_5V_CURRENT_LIMIT:
            WriteUART1(ACK);
            status = _readStringWithTimeout(TIMEOUT);
            if(status == 0) {
                double i = atof(buffer);
                set5VCurrentLimit((uint16_t)(i*1000.0));
            }
            break;
            
        case SET_3V3_CURRENT_LIMIT:
            WriteUART1(ACK);
            status = _readStringWithTimeout(TIMEOUT);
            if(status == 0) {
                double i = atof(buffer);
                set3V3CurrentLimit((uint16_t)(i*1000.0));
            }
            break;
            
        case ENABLE_BUCK:
            WriteUART1(ACK);
            buckEnable();
            break;
            
        case ENABLE_AUX:
            WriteUART1(ACK);
            auxEnable();
            break;
            
        case DISABLE_BUCK:
            WriteUART1(ACK);
            buckDisable();
            break;
            
        case DISABLE_AUX:
            WriteUART1(ACK);
            auxDisable();
            break;
            
        case GET_STATUS:
            WriteUART1(ACK);
            
            a = 0;
            a = buckStatus.currentLimitFired ? a | (1 << 0) : a;
            a = aux5VStatus.currentLimitFired ? a | (1 << 1) : a;
            a = aux3V3Status.currentLimitFired ? a | (1 << 2) : a;
            
            WriteUART1((uint8_t)a);
            WriteUART1('\0');
            break;
            
        case GET_BUCK_SETPOINT:
            WriteUART1(ACK);
            v = ((float)buckStatus.PID.setpoint)*(ADC_VREF/(float)BUCK_ADC_COUNTS);
            sprintf(buffer, "%.2f", v/BUCK_V_FEEDBACK_FACTOR);
            putsUART1((unsigned int *)buffer);
            WriteUART1('\0');
            break;
            
        case GET_ALL_STRING:
            _constructDataString(buffer);
            putsUART1((unsigned int *)buffer);
            break;
                    
        case GET_BUCK_KP:
            sprintf(buffer, "%d", buckStatus.PID.kp);
            putsUART1((unsigned int *)buffer);
            WriteUART1('\0');
            break;
            
        case GET_BUCK_KI:
            sprintf(buffer, "%d", buckStatus.PID.ki);
            putsUART1((unsigned int *)buffer);
            WriteUART1('\0');
            break;
        
        default:
            WriteUART1(NACK);
            break;
    }
}
