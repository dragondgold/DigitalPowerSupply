#ifndef DS1307_H
#define	DS1307_H

#include <xc.h>
#include <stdint.h>

#define RTC_ADDR        0b11010000
#define I2C_READ        1
#define I2C_WRITE       0
#define DS1307_TIMEOUT  10000

#define bitClear(x, n)  ((x) & ~(1 << (n)))
#define bitSet(x, n)    ((x) | (1 << (n)))
#define bitTest(x, n)   ((x) & (1 << (n)))

#define DS1307_WAIT_IDLE_RETURN()   if(_IdleI2C1Timeout(DS1307_TIMEOUT) < 0) return -1

typedef struct {
    uint8_t isMode24;       // Modo 24 o 12 horas
    uint8_t isPM;           // Es PM o AM en modo de 12 horas?
    
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    
    uint8_t weekDay;
    uint8_t date;
    uint8_t month;
    uint8_t year;
} RTCTime;

uint8_t _bcdToBin(uint8_t bcd);
uint8_t _binToBCD(uint8_t bin);
void _decodeHours(uint8_t data, RTCTime *timeData);
int16_t _IdleI2C1Timeout(uint16_t timeout);

void setupI2C(void);
int16_t initRTC(void);
void rtcTasks(void);

int16_t readRTCDate(RTCTime *data);
int16_t readRTCTime(RTCTime *data);
int16_t writeRTCDate(uint8_t weekDay, uint8_t date, uint8_t month, uint8_t year);
int16_t writeRTCTime(uint8_t hours, uint8_t minutes, uint8_t seconds, uint8_t isPM, uint8_t is24);

int16_t writeGpRAM(uint8_t data, uint8_t position);
int16_t readGpRAM(uint8_t position, uint8_t *p);

void formatTime(char *buffer, uint8_t includeSeconds);

#endif

