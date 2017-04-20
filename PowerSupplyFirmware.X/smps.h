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

#define ENABLE_CURRENT_PROTECTION

#define setBuck1Duty(duty)      PDC1 = SDC1 = duty
#define getBuck1Duty()          PDC1

#define setBuck2Duty(duty)      PDC2 = SDC2 = duty
#define getBuck2Duty()          PDC2

#define buck1Emergency()        IOCON1bits.OVRENH = 1; IOCON1bits.OVRENL = 1; FAULT_LAT = 1;

#define buck2Emergency()        IOCON2bits.OVRENH = 1; IOCON2bits.OVRENL = 1; FAULT_LAT = 1;

#define auxEmergency()          ON_OFF_5V_LAT = 1; FAULT_LAT = 1;

typedef struct {
    uint16_t    duty;               // Duty cycle
    uint16_t    deadTime;           // Dead time actual
    
    uint16_t    currentLimit;       // Corriente limite en valor del ADC
    uint16_t    current;            // Corriente actual en valor del ADC
    uint16_t    outputVoltage;      // Tension de salida en valor del ADC

    myPIData    PID;                    // Controlador PID
    
    uint16_t    enablePID:1;            // Indica si el PID está habilitado
    uint16_t    overCurrent:1;          // Indica si hay un exceso de corriente actualmente
    uint16_t    currentLimitFired:1;    // Indica si se excedio el limite de corriente
    uint16_t    enabled:1;              // Indica si la salida está habilitada o no
    uint16_t    dirtySetpoint:1;        // Indica si debe guardarse el setpoint actual en la
                                        //  memoria para ser recordado
} PowerSupplyStatus;

void smpsInit(void);
void smpsTasks(void);

void buck1Enable(void);
void buck2Enable(void);
void auxEnable(void);

void buck1Disable(void);
void buck2Disable(void);
void auxDisable(void);

void setBuck1Voltage(uint16_t voltage);
void setBuck2Voltage(uint16_t voltage);

void setBuck1CurrentLimit(uint16_t currentLimit);
void setBuck2CurrentLimit(uint16_t currentLimit);
void set5VCurrentLimit(uint16_t currentLimit);
void set3V3CurrentLimit(uint16_t currentLimit);

uint16_t getMatchedVoltageADCValue(uint16_t adcValue);
uint16_t getMatchedCurrentADCValue(uint16_t adcValue);

void _initBuck1(void);
void _initBuck2(void);
void _initADC(void);
void _saveSetpoint(uint16_t setpoint, uint8_t lowPos, uint8_t highPos);
uint16_t _loadSetpoint(uint8_t lowPos, uint8_t highPos);
uint16_t _getMatchedADCValue(uint16_t adcValue, const int16_t table[][2], uint16_t size);

#endif	/* SMPS_H */
