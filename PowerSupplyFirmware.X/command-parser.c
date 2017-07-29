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
static uint16_t status, a, b, c;

void initCommandParser(void) {
    RX_TRIS = 1;
    TX_TRIS = 0;
    
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
    U1STAbits.UTXINV = 0;               // No invertimos pin TX
    U1STAbits.UTXBRK = 0;               // Sync Break desactivado
    U1STAbits.URXISEL = 0b00;           // Interrupcion al recibir solo un caracter
    U1STAbits.UTXEN = 1;
    U1MODEbits.UARTEN = 1;              // UART habilitado
    U1STAbits.UTXEN = 1;                // TX habilitado (debe habilitarse despues
                                        // de haber habilitado el modulo)
}

void commandParserTasks(void) {
    if(U1STAbits.OERR) U1STAbits.OERR = 0;
    if(DataRdyUART1()) {
        _decodeCommand(_waitAndReceive());
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
 * Corriente de la línea de 5V en miliamperes
 * @return Corriente de la línea de 5V en miliamperes
 */
uint16_t get5VCurrent() {
    return (uint16_t)((((float)getMatchedCurrentADCValue(aux5VStatus.current))*((ADC_VREF*1000.0)/(float)AUX_ADC_COUNTS)) / (float)AUX_5V_I_FEEDBACK_FACTOR);
}

/**
 * Potencia de la línea de 5V en Watts
 * @return Potencia de la línea de 5V en Watts 
 */
float get5VPower() {
    return (float)aux5VStatus.averagePower * (float)AUX_5V_W_FACTOR;
}

/**
 * Corriente límite de la línea de 5V en miliamperes
 * @return Corriente límite de la línea de 5V en miliamperes
 */
uint16_t get5VCurrentLimit() {
    return (uint16_t)((((float)aux5VStatus.currentLimit*(ADC_VREF / (float)AUX_ADC_COUNTS)) / (float)AUX_5V_I_FEEDBACK_FACTOR)*1000.0);
}

/**
 * Tensión de la línea de 3.3V en volts
 * @return Tensión de la línea de 3.3V en volts
 */
float get3V3Voltage() {
    return (((float)aux3V3Status.outputVoltage)*(ADC_VREF/(float)AUX_ADC_COUNTS)) / (float)AUX_3V3_V_FEEDBACK_FACTOR;
}

/**
 * Corriente de la línea de 3.3V en miliamperes
 * @return Corriente de la línea de 3.3V en miliamperes
 */
uint16_t get3V3Current() {
    return (uint16_t)((((float)getMatchedCurrentADCValue(aux3V3Status.current))*((ADC_VREF*1000.0)/(float)AUX_ADC_COUNTS)) / (float)AUX_3V3_I_FEEDBACK_FACTOR);
}

/**
 * Potencia de la línea de 3.3V en Watts
 * @return Potencia de la línea de 3.3V en Watts 
 */
float get3V3Power() {
    return (float)aux3V3Status.averagePower * (float)AUX_3V3_W_FACTOR;
}

/**
 * Corriente límite de la línea de 3.3V en miliamperes
 * @return Corriente límite de la línea de 3.3V en miliamperes
 */
uint16_t get3V3CurrentLimit() {
    return (uint16_t)((((float)aux3V3Status.currentLimit*(ADC_VREF / (float)AUX_ADC_COUNTS)) / (float)AUX_3V3_I_FEEDBACK_FACTOR)*1000.0);
}

void _constructDataString(char *buffer) {
    sprintf(buffer, "%.2f;", getBuckVoltage());
    sprintf(buffer, "%.2f;", getBuckCurrent());
    sprintf(buffer, "%.2f;", getBuckPower());
    sprintf(buffer, "%.2f;", getBuckCurrentLimit());
    
    sprintf(buffer, "%.2f;", get5VVoltage());
    sprintf(buffer, "%03d;", get5VCurrent());
    sprintf(buffer, "%.2f;", get5VPower());
    sprintf(buffer, "%03d;", get5VCurrentLimit());
    
    sprintf(buffer, "%.2f;", get3V3Voltage());
    sprintf(buffer, "%03d;", get3V3Current());
    sprintf(buffer, "%.2f;", get3V3Power());
    sprintf(buffer, "%03d", get3V3CurrentLimit());
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
            sprintf(buffer, "%.2f", buckStatus.averagePower);
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
            buffer[0] = 0;
            if(buckStatus.overCurrent || aux5VStatus.overCurrent
                    || aux3V3Status.overCurrent) {
                a = 1;
            }
            else if(buckStatus.currentLimitFired || aux5VStatus.currentLimitFired
                    || aux3V3Status.currentLimitFired) {
                a = 2;
            }
            else {
                a = 0;
            }
            
            // Ningun error encontrado
            if(a == 0) {
                putsUART1((unsigned int *)"Sin error");
                WriteUART1('\0');
                return;
            }
            // Damos prioridad si hay cortocircuito
            else if(a == 1) {
                putsUART1((unsigned int *)"Cortocircuito en\n");
                if(buckStatus.overCurrent){
                    strcat(buffer, "B1,");
                }
                if(aux5VStatus.overCurrent) {
                    strcat(buffer, "5V,");
                }
                if(aux3V3Status.overCurrent) {
                    strcat(buffer, "3.3V,");
                }
                // Centramos la segunda linea agregando espacio
                c = (16 - strlen(buffer))/2;
                for(b = 17, c = b + c; b < c; ++b) {
                    buffer[b] = ' ';
                }
                buffer[++b] = '\0';
                
                // Borramos la ultima coma
                buffer[strlen(buffer)-1] = '\0';
            }
            // Sobrecorriente
            else if(a == 2) {
                putsUART1((unsigned int *)"Corriente superada\nen ");
                if(buckStatus.currentLimitFired){
                    strcat(buffer, "B1,");
                }
                if(aux5VStatus.currentLimitFired) {
                    strcat(buffer, "5V,");
                }
                if(aux3V3Status.currentLimitFired) {
                    strcat(buffer, "3.3V,");
                }
                // Centramos la segunda linea agregando espacio
                c = (16 - strlen(buffer))/2;
                for(b = 17, c = b + c; b < c; ++b) {
                    buffer[b] = ' ';
                }
                buffer[++b] = '\0';
                
                // Borramos la ultima coma
                buffer[strlen(buffer)-1] = '\0';
            }
            putsUART1((unsigned int *)buffer);
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
                    
        default:
            WriteUART1(NACK);
            break;
    }
}