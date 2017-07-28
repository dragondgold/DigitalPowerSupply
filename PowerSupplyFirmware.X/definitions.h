/* 
 * File:   definitions.h
 * Author: Andres Torti
 *
 * Created on April 20, 2015, 11:09 AM
 */

#ifndef DEFINITIONS_H
#define	DEFINITIONS_H

#include <xc.h>

#define FCY                         70000000
#define SYSTEM_VERSION              "2.0.0"

#define BUCK_CURRENT_ADC_BUFFER     ADFL0DAT
#define BUCK_VOLTAGE_ADC_BUFFER     ADFL1DAT

#define CURRENT_5V_ADC_BUFFER       ADCBUF2
#define VOLTAGE_5V_ADC_BUFFER       ADCBUF3

#define CURRENT_3V3_ADC_BUFFER      ADCBUF7
#define VOLTAGE_3V3_ADC_BUFFER      ADCBUF18

// Generales
#define ADC_VREF                    3.30

// Parámetros Buck 1
#define BUCK_DEFAULT_PID_ENABLE     0
#define BUCK_PHASE                  3136    // 300kHz
#define BUCK_MAX_DUTY_CYCLE         2988    // 95%
#define BUCK_INITIAL_DUTY_CYCLE     (BUCK_MAX_DUTY_CYCLE/2)
#define BUCK_MIN_DUTY_CYCLE         60      // 2%
#define BUCK_DEAD_TIME              157     // 5%
#define BUCK_MAX_CURRENT            500
#define BUCK_KP                     300
#define BUCK_KI                     4070
#define BUCK_PWMH_TRIS              TRISAbits.TRISA4
#define BUCK_PWML_TRIS              TRISAbits.TRISA3
#define BUCK_PWMH_LAT               LATBbits.LATB4
#define BUCK_PWML_LAT               LATBbits.LATB3
#define BUCK_V_FEEDBACK_FACTOR      0.1092
#define BUCK_I_FEEDBACK_FACTOR      1.044   // mV/mA
#define BUCK_ADC_COUNTS             16384   // 14 bits oversampling
// Este factor permite convertir la multiplicacion de tension*corriente en valores de ADC al valor
//  correspondiente en Watts. La ventaja de esto es que en la interrupcion podemos multiplicar los
//  valores enteros del ADC rapidamente y para obtener el valor de potencia en Watts multiplicar por
//  este facor posteriormente:
//
// W = (ADC_V * ADC_I) * ((Vref/ADC_COUNT)^2 / (FACTOR_V * FACTOR_I))
#define BUCK_W_FACTOR               ((ADC_VREF*ADC_VREF)/((double)BUCK_ADC_COUNTS*(double)BUCK_ADC_COUNTS)) / (BUCK_V_FEEDBACK_FACTOR * BUCK_I_FEEDBACK_FACTOR)
#define POWER_SAMPLE_INTERVAL       6e-6

// Parametros lineas auxiliares
#define MAX_CURRENT_5V              1020
#define MAX_CURRENT_3V3             1020
#define ON_OFF_5V_TRIS              TRISBbits.TRISB15  
#define ON_OFF_5V_LAT               LATBbits.LATB15
#define AUX_5V_V_FEEDBACK_FACTOR    0.641
#define AUX_3V3_V_FEEDBACK_FACTOR   0.909
#define AUX_5V_I_FEEDBACK_FACTOR    3.133   // mV/mA
#define AUX_3V3_I_FEEDBACK_FACTOR   6.666   // mV/mA
#define AUX_ADC_COUNTS              4096

// Modulo UART
#define RX_TRIS                 TRISBbits.TRISB12
#define TX_TRIS                 TRISBbits.TRISB11
#define RX_DEBUG_TRIS           TRISBbits.TRISB14
#define TX_DEBUG_TRIS           TRISBbits.TRISB13

// Salidas de estado
#define FAULT_TRIS              TRISBbits.TRISB5
#define FAULT_LAT               LATBbits.LATB5
#define PWR_GOOD_TRIS           TRISBbits.TRISB15
#define PWR_GOOD_LAT            LATBbits.LATB15

// Entradas analogicas
#define BUCK_CURRENT_TRIS       TRISAbits.TRISA0
#define BUCK_VOLTAGE_TRIS       TRISAbits.TRISA1
#define AUX_5V_CURRENT_TRIS     TRISAbits.TRISA2
#define AUX_5V_VOLTAGE_TRIS     TRISBbits.TRISB0
#define AUX_3V_CURRENT_TRIS     TRISBbits.TRISB2
#define AUX_3V_VOLTAGE_TRIS     TRISBbits.TRISB3

#endif	/* DEFINITIONS_H */

