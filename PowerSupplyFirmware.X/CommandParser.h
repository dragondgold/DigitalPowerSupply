#ifndef COMMANDPARSER_H
#define	COMMANDPARSER_H

#include <stdint.h>
#include "smps.h"

#define TIMEOUT                         10000
#define ACK                             6
#define NACK                            255

#define SET_BUCK1_VOLTAGE               1
#define GET_BUCK1_VOLTAGE               2
#define GET_BUCK1_CURRENT               3

#define SET_BUCK2_VOLTAGE               4
#define GET_BUCK2_VOLTAGE               5
#define GET_BUCK2_CURRENT               7

#define SET_BUCK1_KP                    8
#define SET_BUCK1_KI                    9

#define SET_BUCK2_KP                    10
#define SET_BUCK2_KI                    11

#define DISABLE_B1_PID                  12
#define ENABLE_B1_PID                   13

#define DISABLE_B2_PID                  14
#define ENABLE_B2_PID                   15

#define GET_5V_VOLTAGE                  16
#define GET_3V3_VOLTAGE                 17 
#define GET_5V_CURRENT                  18
#define GET_3V3_CURRENT                 19

#define GET_RTC_TIME                    20
#define GET_RTC_DATE                    21

#define SET_BUCK1_CURRENT_LIMIT         22
#define SET_BUCK2_CURRENT_LIMIT         23
#define SET_5V_CURRENT_LIMIT            24
#define SET_3V3_CURRENT_LIMIT           25

#define ENABLE_BUCK1                    26
#define ENABLE_BUCK2                    27
#define ENABLE_AUX                      28
#define DISABLE_BUCK1                   29
#define DISABLE_BUCK2                   30
#define DISABLE_AUX                     31

#define GET_STATUS                      33
#define GET_B1_SETPOINT                 34
#define GET_B2_SETPOINT                 35

#define SET_RTC_TIME                    37
#define SET_RTC_DATE                    38

#define SHUTDOWN                        32

void commandParserTasks(void);
void initCommandParser(void);

int16_t _readStringWithTimeout(uint16_t timeout);
uint16_t _waitAndReceive(void);
void _decodeCommand(uint16_t command);

// Implementaciones simples de conversion de float a string y viceversa para
//  evitar utilizar sprintf() y atof() de la librería estandard debido a su
//  gran consumo de memoria y en este dsPIC solo tenemos 16KB
float _myAtof(char* s);
void _ltoa(int32_t Value, char *Buffer);
void _floatToString(char *out, float fVal, uint16_t decimalPlaces);
int32_t mPow(int16_t val, uint16_t exponent);

#endif	/* COMMANDPARSER_H */

