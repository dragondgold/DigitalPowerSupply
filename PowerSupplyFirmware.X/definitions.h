/* 
 * File:   definitions.h
 * Author: Andres Torti
 *
 * Created on April 20, 2015, 11:09 AM
 */

#ifndef DEFINITIONS_H
#define	DEFINITIONS_H

#include <xc.h>

#define FCY                         40000000

// Par 0 (Buck 1)
#define B1_CURRENT_ADC_BUFFER       (ADCBUF0 >> 1)     // S&H Dedicado
#define B1_VOLTAGE_ADC_BUFFER       (ADCBUF1 >> 1)

// Par 1 (Buck 2)
#define B2_CURRENT_ADC_BUFFER       (ADCBUF2 >> 1)     // S&H Dedicado
#define B2_VOLTAGE_ADC_BUFFER       (ADCBUF3 >> 1)

// Par 2 (Corrientes auxiliares)
#define CURRENT_5V_ADC_BUFFER       (ADCBUF4 >> 1)     // S&H Dedicado
#define CURRENT_3V3_ADC_BUFFER      (ADCBUF5 >> 1)

// Par 3 (Tensiones auxiliares)
#define VOLTAGE_5V_ADC_BUFFER       (ADCBUF6 >> 1)
#define VOLTAGE_3V3_ADC_BUFFER      (ADCBUF7 >> 1)

// Generales
#define MAX_BUCKS_SHARED_CURRENT    1020
#define ADC_VREF                    3.24
#define ADC_COUNTS                  511

// Parámetros Buck 1
#define B1_DEFAULT_PID_ENABLE   0
#define B1_PHASE1               6280    // 150KHz
#define B1_INITIAL_DUTY_CYCLE   200     // 3%
#define B1_MAX_DUTY_CYCLE       5960    // 95%
#define B1_MIN_DUTY_CYCLE       100     // 1.6%
#define B1_DEAD_TIME            236     // 250ns - 3.7%
#define B1_MAX_CURRENT          500
#define B1Kp                    300
#define B1Ki                    4070
#define B1_PWMH_TRIS            TRISAbits.TRISA4
#define B1_PWML_TRIS            TRISAbits.TRISA3
#define B1_PWMH_LAT             LATAbits.LATA4
#define B1_PWML_LAT             LATAbits.LATA3
#define B1_V_FEEDBACK_FACTOR    0.1092
#define B1_I_FEEDBACK_FACTOR    1.044   // mV/mA
#define B1_SETPOINT_RAM_POS_L   0
#define B1_SETPOINT_RAM_POS_H   1

// Parámetros Buck 2
#define B2_DEFAULT_PID_ENABLE   1
#define B2_PHASE1               6280    // 150KHz
#define B2_INITIAL_DUTY_CYCLE   200     // 3%
#define B2_MAX_DUTY_CYCLE       5960    // 95%
#define B2_MIN_DUTY_CYCLE       100     // 1.6%
#define B2_DEAD_TIME            236     // 250ns - 3.7%
#define B2_MAX_CURRENT          500    
#define B2Kp                    300
#define B2Ki                    4070
#define B2_PWMH_TRIS            TRISBbits.TRISB13
#define B2_PWML_TRIS            TRISBbits.TRISB14
#define B2_PWMH_LAT             LATBbits.LATB13
#define B2_PWML_LAT             LATBbits.LATB14
#define B2_V_FEEDBACK_FACTOR    0.1092
#define B2_I_FEEDBACK_FACTOR    1.044   // mV/mA
#define B2_SETPOINT_RAM_POS_L   2
#define B2_SETPOINT_RAM_POS_H   3

// Parametros lineas auxiliares
#define MAX_CURRENT_5V              1020
#define MAX_CURRENT_3V3             1020
#define ON_OFF_5V_TRIS              TRISBbits.TRISB15  
#define ON_OFF_5V_LAT               LATBbits.LATB15
#define AUX_5V_V_FEEDBACK_FACTOR    0.641
#define AUX_3V3_V_FEEDBACK_FACTOR   0.909
#define AUX_5V_I_FEEDBACK_FACTOR    3.133   // mV/mA
#define AUX_3V3_I_FEEDBACK_FACTOR   6.666   // mV/mA

// Modulo UART
#define RX_TRIS                 TRISBbits.TRISB12
#define TX_TRIS                 TRISBbits.TRISB11

// Salidas de estado
#define FAULT_TRIS              TRISBbits.TRISB5
#define FAULT_LAT               LATBbits.LATB5
#define PWR_GOOD_TRIS           TRISBbits.TRISB8
#define PWR_GOOD_LAT            LATBbits.LATB8

#endif	/* DEFINITIONS_H */

