/* 
 * File:   smps.h
 * Author: Andres Torti
 *
 * Created on April 21, 2015, 11:51 PM
 */

#ifndef SMPS_H
#define	SMPS_H

#include <stdint.h>
#include "definitions.h"
#include "pid.h"

#define INSTANTANEUS_VOLTAGE_SAMPLES        2048
#define INSTANTANEUS_CURRENT_SAMPLES        INSTANTANEUS_VOLTAGE_SAMPLES

#define setBuckDuty(duty)       PDC1 = SDC1 = duty
#define getBuckDuty()           PDC1

#define buckEmergency()         IOCON1bits.OVRENH = 1; IOCON1bits.OVRENL = 1; FAULT_LAT = 0;
#define auxEmergency()          ON_OFF_5V_LAT = 1; FAULT_LAT = 0;

#define setChannelsStep1()      setBuckChannels()
#define setChannelsStep2()      set3V3Channels()

// Buck current, core 0 -> AN0
// Buck voltage, core 1 -> AN1
#define setBuckChannels()       ADCON4Hbits.C0CHS = 0b00;           /* AN0 to Core 0 */ \
                                ADCON4Hbits.C1CHS = 0b00;           /* AN1 to Core 1 */ \
                                ADTRIG1Hbits.TRGSRC7 = 0;           /* AN7 trigger deshabilitado */ \
                                ADTRIG4Hbits.TRGSRC18 = 0;          /* AN18 trigger deshabilitado */ \
                                ADTRIG0Lbits.TRGSRC0 = 0b00101;     /* AN0 PWM Generator 1 primary trigger */ \
                                ADTRIG0Lbits.TRGSRC1 = 0b00101;     /* AN1 PWM Generator 1 primary trigger */

// 5V current, core 2 -> AN2
// 5V voltage, core 3 -> AN3
#define set5VChannels()         ADCON4Hbits.C2CHS = 0b00;           /* AN2 to Core 2 */ \
                                ADCON4Hbits.C3CHS = 0b00;           /* AN3 to Core 3 */ \
                                ADTRIG0Hbits.TRGSRC2 = 0b00101;     /* AN2 PWM Generator 1 primary trigger */ \
                                ADTRIG0Hbits.TRGSRC3 = 0b00101;     /* AN3 PWM Generator 1 primary trigger */

// 3V3 current, core 0 -> AN7
// 3V3 voltage, core 1 -> AN18
#define set3V3Channels()        ADCON4Hbits.C0CHS = 0b01;           /* AN7 to Core 0 */ \
                                ADCON4Hbits.C1CHS = 0b01;           /* AN18 to Core 1 */  \
                                ADTRIG0Lbits.TRGSRC0 = 0;           /* AN0 trigger deshabilitado */ \
                                ADTRIG0Lbits.TRGSRC1 = 0;           /* AN1 trigger deshabilitado */ \
                                ADTRIG1Hbits.TRGSRC7 = 0b00101;     /* AN7 PWM Generator 1 primary trigger */ \
                                ADTRIG4Hbits.TRGSRC18 = 0b00101;    /* AN18 PWM Generator 1 primary trigger */

typedef struct {
    uint16_t    duty;               // Duty cycle
    uint16_t    deadTime;           // Dead time actual
    uint16_t    phaseReg;           // Valor del registro PHASE para la frecuencia
    uint16_t    pTrigger;           // Trigger primario
    
    uint16_t    currentLimit;       // Corriente limite en valor del ADC
    uint16_t    current;            // Corriente actual en valor del ADC
    uint16_t    outputVoltage;      // Tensión de salida en valor del ADC
    uint32_t    averagePower;       // Potencia media en valor del ADC

    _PIDData    PID;                    // Controlador PID
    
    uint16_t    enablePID:1;            // Indica si el PID está habilitado
    uint16_t    currentLimitFired:1;    // Indica si se excedió el limite de corriente
    uint16_t    enabled:1;              // Indica si la salida está habilitada o no
    
    // Para calculo de tensión media  
    volatile uint32_t voltageSum;
    volatile uint16_t currentVoltageSample;
    volatile uint16_t averageVoltage;
    
    volatile uint32_t currentSum;
    volatile uint16_t currentCurrentSample;
    volatile uint16_t averageCurrent;
} PowerSupplyStatus;

void smpsInit(void);
void smpsTasks(void);

void buckEnable(void);
void auxEnable(void);

void buckDisable(void);
void auxDisable(void);

void setBuckVoltage(uint16_t voltage);

void setBuckCurrentLimit(uint16_t currentLimit);
void set5VCurrentLimit(uint16_t currentLimit);
void set3V3CurrentLimit(uint16_t currentLimit);

uint16_t getMatchedADCValue(uint16_t adcValue, float offset, float gain);
uint16_t getInverseMatchedADCValue(uint16_t adcValue, float offset, float gain);

void _initBuckPWM(PowerSupplyStatus *data);
void _initADC(void);
void _initComparators(void);

#endif	/* SMPS_H */
