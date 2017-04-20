#include "CommandParser.h"
#include "smps.h"
#include "definitions.h"
#include "ds1307.h"
#include <xc.h>
#include <uart.h>
#include <libpic30.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

static char buffer[50] = {0};

// External variables
extern PowerSupplyStatus buck1Status;
extern PowerSupplyStatus buck2Status;
extern PowerSupplyStatus aux5VStatus;
extern PowerSupplyStatus aux3V3Status;

extern RTCTime rtcTime;
static float v;
static uint16_t status, a, b, c, d, e;

/**
 * Convierte string a float, una version simplificada de atof() en la libreria
 *  estandard. En nuestro caso solo se utiliza para la conversion de string
 *  simples como "12.24"
 * @param s string a convertir
 * @return float resultado de la conversion
 */
float _myAtof(char* s){
  float rez = 0, fact = 1;
  int point_seen;
  
  if (*s == '-'){
    s++;
    fact = -1;
  }
  
  for (point_seen = 0; *s; s++){
    if (*s == '.'){
      point_seen = 1; 
      continue;
    };
    int d = *s - '0';
    if (d >= 0 && d <= 9){
      if (point_seen) fact /= 10.0f;
      rez = rez * 10.0f + (float)d;
    }
  }
  return rez * fact;
}

/**
 * Reemplazo basico de la funcion standard ltoa() para un menor uso
 *  de memoria. Convierte un entero de hasta 32-bits a string
 * @param Value Valor a convertir
 * @param Buffer Punto al array donde almacenar el string
 */
void _ltoa(int32_t Value, char* Buffer) {
	int16_t i;
	int32_t Digit;
	int32_t Divisor;
	uint16_t Printed = 0;

	if(Value) {
		for(i = 0, Divisor = 1000000000; i < 10; i++) {
			Digit = Value/Divisor;
			if(Digit || Printed) {
				*Buffer++ = '0' + Digit;
				Value -= Digit*Divisor;
				Printed = 1;
			}
			Divisor /= 10;
		}
	}
	else {
		*Buffer++ = '0';
	}

	*Buffer = '\0';
}

/**
 * Convierte un numero en coma flotante a string
 * @param out Donde almacenar el string convertido
 * @param fVal Valor a convertir
 * @param decimalPlaces Cantidad de decimales en el string de salida.
 *  Como máximo convierte hasta 3 decimales.
 */
void _floatToString(char *out, float fVal, uint16_t decimalPlaces) {    
    char temp[7];   // 6 decimales y NULL
    float i;
    float f = modff(fVal, &i);
    int32_t integral, fractional;
    uint32_t factor;
    uint16_t zeros, n;
    
    if(decimalPlaces > 3) decimalPlaces = 3;
    if(f < 0.0001) f = 0.0001;
    
    integral = (int32_t)i;
    factor = mPow(10, decimalPlaces);
    fractional = (uint32_t)(f*(double)factor);
    
    if((uint16_t)(f*1000) == 0) zeros = 3;
    else if((uint16_t)(f*100) == 0) zeros = 2;
    else if((uint16_t)(f*10) == 0) zeros = 1;
    else zeros = 0;
    
    if(zeros > decimalPlaces) zeros = decimalPlaces;
    
    _ltoa(integral, out);
    if(decimalPlaces > 0) {
        strcat(out, ".");
        for(n = 0; n < zeros; ++n) {
            strcat(out, "0");
        } 
        
        if(decimalPlaces > zeros) {
            _ltoa(fractional, temp);
            strcat(out, temp);
        }
    }
}

/**
 * Funcion pow() simplificada
 * @param val Numero el cual elevar el exponente
 * @param exponent Exponente
 * @return val elevado a exponent
 */
int32_t mPow(int16_t val, uint16_t exponent) {
    uint16_t n;
    int32_t result = 1;
    
    if(exponent == 0) return 1;
    if(exponent == 1) return val;
    
    for(n = 0; n < exponent; ++n) {
        result = result*val; 
    }
    
    return result;
}

void initCommandParser(void) {
    U1MODEbits.IREN = 0;                // IrDA deshabilitado
    U1MODEbits.RTSMD = 1;               // Modo Simplex
    U1MODEbits.UEN = 0b00;              // Solo usamos pines RX y TX
    U1MODEbits.LPBACK = 0;              // Loopback desactivado
    U1MODEbits.ABAUD = 0;               // Autobaud desactivado
    U1MODEbits.URXINV = 0;              // No invertimos pin RX
    U1MODEbits.PDSEL = 0b00;            // 8-bit, sin paridad
    U1MODEbits.STSEL = 0;               // 1 bit de stop
    U1MODEbits.BRGH = 1;
    U1BRG = 86;                         // 115200 baudios
    
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

void _decodeCommand(uint16_t command){
    switch (command){
        case SET_BUCK1_VOLTAGE:
            WriteUART1(ACK);
            status = _readStringWithTimeout(TIMEOUT);
            if(status == 0) {
                double v = _myAtof(buffer);
                setBuck1Voltage((uint16_t)(v*1000.0));
            }
            break;
            
        case GET_BUCK1_VOLTAGE:
            WriteUART1(ACK);     
            v = ((float)getMatchedVoltageADCValue(buck1Status.outputVoltage))*(ADC_VREF/(float)ADC_COUNTS);
            _floatToString(buffer, v/B1_V_FEEDBACK_FACTOR, 2);
            putsUART1((unsigned int *)buffer);
            WriteUART1('\0');
            break;
            
        case GET_BUCK1_CURRENT:
            WriteUART1(ACK);
            v = ((float)getMatchedCurrentADCValue(buck1Status.current))*(ADC_VREF/(float)ADC_COUNTS);
            v = v / (float)B1_I_FEEDBACK_FACTOR;
            _floatToString(buffer, v, 2);
            putsUART1((unsigned int *)buffer);
            WriteUART1('\0');
            break;
        
        case SET_BUCK2_VOLTAGE:
            Nop();
            WriteUART1(ACK);
            status = _readStringWithTimeout(TIMEOUT);
            if(status == 0) {
                Nop();
                double v = _myAtof(buffer);
                setBuck2Voltage((uint16_t)(v*1000.0));
            }
            break;

        case GET_BUCK2_VOLTAGE:
            WriteUART1(ACK);     
            v = ((float)getMatchedVoltageADCValue(buck2Status.outputVoltage))*(ADC_VREF/(float)ADC_COUNTS);
            _floatToString(buffer, v/B2_V_FEEDBACK_FACTOR, 2);
            putsUART1((unsigned int *)buffer);
            WriteUART1('\0');
            break;

        case GET_BUCK2_CURRENT:
            WriteUART1(ACK);
            v = ((float)getMatchedCurrentADCValue(buck2Status.current))*(ADC_VREF/(float)ADC_COUNTS);
            v = v / (float)B2_I_FEEDBACK_FACTOR;
            _floatToString(buffer, v, 2);
            putsUART1((unsigned int *)buffer);
            WriteUART1('\0');
            break;
            
        case DISABLE_B1_PID:
            WriteUART1(ACK);
            buck1Status.enablePID = 0;
            break;
            
        case ENABLE_B1_PID:
            WriteUART1(ACK);
            buck1Status.PID.integralTerm = 0;
            buck1Status.enablePID = 1;
            break;

        case DISABLE_B2_PID:
            WriteUART1(ACK);
            buck2Status.enablePID = 0;
            break;

        case ENABLE_B2_PID:
            WriteUART1(ACK);
            buck2Status.PID.integralTerm = 0;
            buck2Status.enablePID = 1;
            break;
            
        case SET_BUCK1_KP:
            WriteUART1(ACK);
            buck1Status.enablePID = 0;
            status = _readStringWithTimeout(TIMEOUT);
            if(status == 0) {
                double kp = _myAtof(buffer);
                buck1Status.PID.kp = kp;
            }
            buck1Status.PID.integralTerm = 0;
            buck1Status.enablePID = 1;
            break;
            
        case SET_BUCK1_KI:
            WriteUART1(ACK);
            buck1Status.enablePID = 0;
            status = _readStringWithTimeout(TIMEOUT);
            if(status == 0) {
                double ki = _myAtof(buffer);
                buck1Status.PID.ki = ki;
            }
            buck1Status.PID.integralTerm = 0;
            buck1Status.enablePID = 1;
            break;
            
        case SET_BUCK2_KP:
            WriteUART1(ACK);
            buck2Status.enablePID = 0;
            status = _readStringWithTimeout(TIMEOUT);
            if(status == 0) {
                double kp = _myAtof(buffer);
                buck2Status.PID.kp = kp;
            }
            buck2Status.PID.integralTerm = 0;
            buck2Status.enablePID = 1;
            break;
        
        case SET_BUCK2_KI:
            WriteUART1(ACK);
            buck2Status.enablePID = 0;
            status = _readStringWithTimeout(TIMEOUT);
            if(status == 0) {
                double ki = _myAtof(buffer);
                buck2Status.PID.ki = ki;
            }
            buck2Status.PID.integralTerm = 0;
            buck2Status.enablePID = 1;
            break;
            
        case GET_5V_VOLTAGE:
            Nop();
            WriteUART1(ACK);
            v = ((float)aux5VStatus.outputVoltage)*(ADC_VREF/(float)ADC_COUNTS);
            _floatToString(buffer, v/AUX_5V_V_FEEDBACK_FACTOR, 2);
            putsUART1((unsigned int *)buffer);
            WriteUART1('\0');
            break;
            
        case GET_5V_CURRENT:
            WriteUART1(ACK);
            v = ((float)getMatchedCurrentADCValue(aux5VStatus.current))*((ADC_VREF*1000.0)/(float)ADC_COUNTS);
            a = (uint16_t)(v / (float)AUX_5V_I_FEEDBACK_FACTOR);
            Nop();
            // No mostramos mas de 3 digitos
            if(a > 999) a = 999;
            _ltoa(a, buffer);
            WriteUART1('.');
            if(a < 100) WriteUART1('0');
            if(a < 10) WriteUART1('0');
            putsUART1((unsigned int *)buffer);
            WriteUART1('\0');
            break;
            
        case GET_3V3_VOLTAGE:
            WriteUART1(ACK);
            v = ((float)aux3V3Status.outputVoltage)*(ADC_VREF/(float)ADC_COUNTS);
            _floatToString(buffer, v/AUX_3V3_V_FEEDBACK_FACTOR, 2);
            putsUART1((unsigned int *)buffer);
            WriteUART1('\0');
            break;
            
        case GET_3V3_CURRENT:
            WriteUART1(ACK);
            v = ((float)getMatchedCurrentADCValue(aux3V3Status.current))*((ADC_VREF*1000.0)/(float)ADC_COUNTS);
            a = (uint16_t)(v / (float)AUX_3V3_I_FEEDBACK_FACTOR);
            // No mostramos mas de 3 digitos
            if(a > 999) a = 999;
            _ltoa(a, buffer);
            WriteUART1('.');
            if(a < 100) WriteUART1('0');
            if(a < 10) WriteUART1('0');
            putsUART1((unsigned int *)buffer);
            WriteUART1('\0');
            break;

        case SHUTDOWN:
            WriteUART1(ACK);
            buck1Disable();
            buck2Disable();
            auxDisable();
            break;
            
        case GET_RTC_TIME:
            WriteUART1(ACK);
            formatTime(buffer, 0);
            putsUART1((unsigned int *)buffer);
            WriteUART1('\0');
            break;
            
        case GET_RTC_DATE:
            WriteUART1(ACK);
            WriteUART1(rtcTime.date);
            WriteUART1(rtcTime.month);
            WriteUART1(rtcTime.year);
            break;
            
        case SET_RTC_TIME:
            WriteUART1(ACK);
            
            a = _waitAndReceive();  // Hours
            b = _waitAndReceive();  // Minutes
            c = _waitAndReceive();  // Seconds
            d = _waitAndReceive();  // Is PM?
            e = _waitAndReceive();  // Is 24 hours
            writeRTCTime(a, b, c, d, e);
            break;
            
        case SET_RTC_DATE:
            WriteUART1(ACK);
            a = _waitAndReceive();  // Week day
            b = _waitAndReceive();  // Date
            c = _waitAndReceive();  // Month
            d = _waitAndReceive();  // Year
            writeRTCDate(a, b, c, d);
            break;
          
        case SET_BUCK1_CURRENT_LIMIT:
            WriteUART1(ACK);
            status = _readStringWithTimeout(TIMEOUT);
            if(status == 0) {
                double i = _myAtof(buffer);
                setBuck1CurrentLimit((uint16_t)(i*1000.0));
            }
            break;
            
        case SET_BUCK2_CURRENT_LIMIT:
            WriteUART1(ACK);
            status = _readStringWithTimeout(TIMEOUT);
            if(status == 0) {
                double i = _myAtof(buffer);
                setBuck2CurrentLimit((uint16_t)(i*1000.0));
            }
            break;
            
        case SET_5V_CURRENT_LIMIT:
            WriteUART1(ACK);
            status = _readStringWithTimeout(TIMEOUT);
            if(status == 0) {
                double i = _myAtof(buffer);
                set5VCurrentLimit((uint16_t)(i*1000.0));
            }
            break;
            
        case SET_3V3_CURRENT_LIMIT:
            WriteUART1(ACK);
            status = _readStringWithTimeout(TIMEOUT);
            if(status == 0) {
                double i = _myAtof(buffer);
                set3V3CurrentLimit((uint16_t)(i*1000.0));
            }
            break;
            
        case ENABLE_BUCK1:
            WriteUART1(ACK);
            buck1Enable();
            break;
            
        case ENABLE_BUCK2:
            WriteUART1(ACK);
            buck2Enable();
            break;
            
        case ENABLE_AUX:
            WriteUART1(ACK);
            auxEnable();
            break;
            
        case DISABLE_BUCK1:
            WriteUART1(ACK);
            buck1Disable();
            break;
            
        case DISABLE_BUCK2:
            WriteUART1(ACK);
            buck2Disable();
            break;
            
        case DISABLE_AUX:
            WriteUART1(ACK);
            auxDisable();
            break;
            
        case GET_STATUS:
            WriteUART1(ACK);
            buffer[0] = 0;
            if(buck1Status.overCurrent || buck2Status.overCurrent ||
               aux5VStatus.overCurrent || aux3V3Status.overCurrent) {
                a = 1;
            }
            else if(buck1Status.currentLimitFired || buck2Status.currentLimitFired ||
                    aux5VStatus.currentLimitFired || aux3V3Status.currentLimitFired) {
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
                if(buck1Status.overCurrent){
                    strcat(buffer, "B1,");
                }
                if(buck2Status.overCurrent) {
                    strcat(buffer, "B2,");
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
                if(buck1Status.currentLimitFired){
                    strcat(buffer, "B1,");
                }
                if(buck2Status.currentLimitFired) {
                    strcat(buffer, "B2,");
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
            
        case GET_B1_SETPOINT:
            WriteUART1(ACK);
            v = ((float)buck1Status.PID.setpoint)*(ADC_VREF/(float)ADC_COUNTS);
            _floatToString(buffer, v/B1_V_FEEDBACK_FACTOR, 2);
            putsUART1((unsigned int *)buffer);
            WriteUART1('\0');
            break;
            
        case GET_B2_SETPOINT:
            WriteUART1(ACK);
            v = ((float)buck2Status.PID.setpoint)*(ADC_VREF/(float)ADC_COUNTS);
            _floatToString(buffer, v/B2_V_FEEDBACK_FACTOR, 2);
            putsUART1((unsigned int *)buffer);
            WriteUART1('\0');
            break;
                    
        default:
            WriteUART1(NACK);
            break;
    }
}
