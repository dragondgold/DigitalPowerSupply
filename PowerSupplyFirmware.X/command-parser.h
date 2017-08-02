#ifndef COMMANDPARSER_H
#define	COMMANDPARSER_H

#include <stdint.h>
#include "smps.h"

// ~75ms
#define TIMER_VALUE                     45000

#define TIMEOUT                         10000
#define ACK                             0x06
#define ACK_ERROR                       0x01
#define NACK                            0xFF

#define SET_BUCK_VOLTAGE                1
#define GET_BUCK_VOLTAGE                2
#define GET_BUCK_CURRENT                3
#define GET_BUCK_POWER                  4

#define SET_BUCK_KP                     5
#define SET_BUCK_KI                     7

#define DISABLE_BUCK_PID                8
#define ENABLE_BUCK_PID                 9

#define GET_5V_VOLTAGE                  10
#define GET_3V3_VOLTAGE                 11 
#define GET_5V_CURRENT                  12
#define GET_3V3_CURRENT                 13

#define SET_BUCK_CURRENT_LIMIT          14
#define SET_5V_CURRENT_LIMIT            15
#define SET_3V3_CURRENT_LIMIT           16

#define ENABLE_BUCK                     17
#define ENABLE_AUX                      18
#define DISABLE_BUCK                    19
#define DISABLE_AUX                     20

#define GET_STATUS                      21
#define GET_BUCK_SETPOINT               22
#define GET_ALL_STRING                  23
#define GET_BUCK_KP                     25
#define GET_BUCK_KI                     26

#define SHUTDOWN                        24

void commandParserTasks(void);
void initCommandParser(void);

float getBuckVoltage();
float getBuckCurrent();
float getBuckPower();
float getBuckCurrentLimit();

float get5VVoltage();
float get5VCurrent();
float get5VPower();
float get5VCurrentLimit();

float get3V3Voltage();
float get3V3Current();
float get3V3Power();
float get3V3CurrentLimit();

void _constructDataString(char *buffer);

int16_t _readStringWithTimeout(uint16_t timeout);
uint16_t _waitAndReceive(void);
void _decodeCommand(uint16_t command);

#endif	/* COMMANDPARSER_H */

