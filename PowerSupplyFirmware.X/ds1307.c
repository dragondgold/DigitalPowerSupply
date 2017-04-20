#include "ds1307.h"
#include "definitions.h"
#include "CommandParser.h"
#include <i2c.h>
#include <xc.h>
#include <libpic30.h>
#include <string.h>

static volatile uint16_t counter;
static volatile uint16_t dirtyRTC;
RTCTime rtcTime;
uint16_t rtcReady = 0;

void __attribute__((__interrupt__, no_auto_psv)) _T1Interrupt(void) {
    PR1 = 31250;
    
    // Cada 600ms actualizamos el RTC
    if(++counter == 3) {
        counter = 0;
        dirtyRTC = 1;
    }
    
    IFS0bits.T1IF = 0;
}

uint8_t _bcdToBin(uint8_t bcd) {    
    int r, t;
    t = bcd & 0x0F;
    r = t;
    bcd = 0xF0 & bcd;
    t = bcd >> 4;
    t = 0x0F & t;
    r = t*10 + r;
    return r;
}

uint8_t _binToBCD(uint8_t bin) {
    return (bin/10*16) + (bin%10);
}

void _decodeHours(uint8_t data, RTCTime *timeData) {
    uint8_t lower = data & 0x0F;
    uint8_t higher;
    
    // 12 hour mode
    if(bitTest(data, 6)) {
        timeData->isMode24 = 0;
        timeData->isPM = bitTest(data, 5);
        if(bitTest(data, 4)) {
            timeData->hours = lower + 10;
        }
        else {
            timeData->hours = lower;
        }
    }
    // 24 hour mode
    else {
        timeData->isMode24 = 1;
        higher = (data & 0x30) >> 4;
        timeData->hours = lower + higher*10;
    }
}

/**
 * Basado en la funcion IdleI2C1() de la libreria de perifericos pero
 *  agregando un timeout de espera
 * @param timeout tiempo de espera siendo aproximadamente de [timeout]*10us
 * @return 
 */
int16_t _IdleI2C1Timeout(uint16_t timeout) {
    while(I2C1CONbits.SEN || I2C1CONbits.PEN || I2C1CONbits.RCEN || 
			I2C1CONbits.RSEN || I2C1CONbits.ACKEN || I2C1STATbits.TRSTAT) {
        __delay_us(10);
        --timeout;
        if(timeout == 0){
            return -1;
        }
    }
    return 0;
}

/**
 * Inicializa el RTC y lee la fecha y hora actual
 * @return 0 si la inicializacion se completo. -1 si hubo algun problema.
 */
int16_t initRTC(void) {
    uint8_t data;
    rtcReady = 0;
    dirtyRTC = 0;
    counter = 0;
    
    PR1 = 31250;            // Interrupcion cada 200ms
    IPC0bits.T1IP = 1;      // Prioridad 1
    IFS0bits.T1IF = 0;
    IEC0bits.T1IE = 1;
    
    T1CONbits.TCKPS = 0b11;
    T1CONbits.TCS = 0;
    T1CONbits.TON = 1;
    
    // Leemos hora y fecha actual
    if(readRTCTime(&rtcTime) < 0) return -2;
    if(readRTCDate(&rtcTime) < 0) return -3;
    
    // Habilitamos el oscilador en caso de que no esté habilitado.
    // Direccionamos el apuntador de memoria del RTC a la primera
    //  posicion
    StartI2C1();
    DS1307_WAIT_IDLE_RETURN();
    data = MasterWriteI2C1(RTC_ADDR | I2C_WRITE);
    if(data < 0) {
        StopI2C1();
        DS1307_WAIT_IDLE_RETURN();
        return -1;
    }
    DS1307_WAIT_IDLE_RETURN();
    MasterWriteI2C1(0);
    DS1307_WAIT_IDLE_RETURN();
    
    // Entramos a modo lectura ahora y leemos el registro
    //  de segundos
    RestartI2C1();
    DS1307_WAIT_IDLE_RETURN();
    data = MasterWriteI2C1(RTC_ADDR | I2C_READ);
    if(data < 0) {
        StopI2C1();
        DS1307_WAIT_IDLE_RETURN();
        return -1;
    }
    DS1307_WAIT_IDLE_RETURN();
    data = MasterReadI2C1();
    NotAckI2C1();
    DS1307_WAIT_IDLE_RETURN();
    StopI2C1();
    DS1307_WAIT_IDLE_RETURN();
    __delay_us(5);
    
    // Bit CH en 1 = oscilador deshabilitado
    if(bitTest(data, 7)) {
        // Habilitamos el oscilador
        data = bitClear(data, 7);
        
        StartI2C1();
        DS1307_WAIT_IDLE_RETURN();
        data = MasterWriteI2C1(RTC_ADDR | I2C_WRITE);
        if(data < 0) {
            StopI2C1();
            DS1307_WAIT_IDLE_RETURN();
            return -1;
        }
        DS1307_WAIT_IDLE_RETURN();
        MasterWriteI2C1(0);
        DS1307_WAIT_IDLE_RETURN();
        MasterWriteI2C1(data);
        DS1307_WAIT_IDLE_RETURN();
        StopI2C1();
        DS1307_WAIT_IDLE_RETURN();
        __delay_us(5);
    }
    
    rtcReady = 1;
    return 0;
}

void rtcTasks(void) {
    if(dirtyRTC) {
        dirtyRTC = 0;
    }
}

void setupI2C(void) {
    I2C1CONbits.STREN = 1;      // Clock stretching
    I2C1CONbits.DISSLW = 0;
    I2C1CONbits.I2CEN = 1;
    I2C1BRG = 393;  // 100 KHz
}

/**
 * Lee la hora actual del RTC y lo guarda en la estructura de tiempo
 *  pasada como puntero
 * @param data estructura donde almacenar la hora leida
 * @return 0 si la lectura fue exitosa, -1 de otro modo.
 */
int16_t readRTCTime(RTCTime *data) {
    uint8_t d;
    if(!rtcReady) return -1;
    
    // Direccionamos el apuntador de memoria del RTC a la primera
    //  posicion
    StartI2C1();
    DS1307_WAIT_IDLE_RETURN();
    d = MasterWriteI2C1(RTC_ADDR | I2C_WRITE);
    if(d < 0) {
        StopI2C1();
        DS1307_WAIT_IDLE_RETURN();
        return -1;
    }
    DS1307_WAIT_IDLE_RETURN();
    MasterWriteI2C1(0);
    DS1307_WAIT_IDLE_RETURN();

    // Entramos a modo lectura ahora
    RestartI2C1();
    DS1307_WAIT_IDLE_RETURN();
    d = MasterWriteI2C1(RTC_ADDR | I2C_READ);
    if(d < 0) {
        StopI2C1();
        DS1307_WAIT_IDLE_RETURN();
        return -1;
    }
    DS1307_WAIT_IDLE_RETURN();
    
    // Leemos segundos
    d = MasterReadI2C1();
    data->seconds = _bcdToBin(bitClear(d, 7));
    AckI2C1();
    DS1307_WAIT_IDLE_RETURN();
   
    // Leemos minutos
    d = MasterReadI2C1();
    data->minutes = _bcdToBin(d);
    AckI2C1();
    DS1307_WAIT_IDLE_RETURN();
    
    // Leemos horas finalmente
    d = MasterReadI2C1();
    _decodeHours(d, data);
    NotAckI2C1();
    DS1307_WAIT_IDLE_RETURN();
    StopI2C1();
    DS1307_WAIT_IDLE_RETURN();
    __delay_us(5);
    
    return 0;
}

/**
 * Lee la fecha actual del RTC y lo guarda en la estructura de tiempo
 *  pasada como puntero
 * @param data estructura donde almacenar la fecha leida
 * @return 0 si la lectura fue exitosa, -1 de otro modo.
 */
int16_t readRTCDate(RTCTime *data) {
    uint8_t d;
    if(!rtcReady) return -1;
    
    // Direccionamos el apuntador de memoria del RTC a donde se
    //  almacena el dia de la semana
    StartI2C1();
    DS1307_WAIT_IDLE_RETURN();
    d = MasterWriteI2C1(RTC_ADDR | I2C_WRITE);
    if(d < 0) {
        StopI2C1();
        DS1307_WAIT_IDLE_RETURN();
        return -1;
    }
    DS1307_WAIT_IDLE_RETURN();
    MasterWriteI2C1(3);
    DS1307_WAIT_IDLE_RETURN();
    
    // Entramos a modo lectura ahora
    RestartI2C1();
    DS1307_WAIT_IDLE_RETURN();
    d = MasterWriteI2C1(RTC_ADDR | I2C_READ);
    if(d < 0) {
        StopI2C1();
        DS1307_WAIT_IDLE_RETURN();
        return -1;
    }
    DS1307_WAIT_IDLE_RETURN();
    
    // Leemos dia de la semana
    data->weekDay = MasterReadI2C1();
    AckI2C1();
    DS1307_WAIT_IDLE_RETURN();
   
    // Leemos fecha
    d = MasterReadI2C1();
    data->date = _bcdToBin(d);
    AckI2C1();
    DS1307_WAIT_IDLE_RETURN();
    
    // Leemos mes
    d = MasterReadI2C1();
    data->month = _bcdToBin(d);
    AckI2C1();
    DS1307_WAIT_IDLE_RETURN();
    
    // Leemos año finalmente
    d = MasterReadI2C1();
    data->year = _bcdToBin(d);
    NotAckI2C1();
    DS1307_WAIT_IDLE_RETURN();
    StopI2C1();
    DS1307_WAIT_IDLE_RETURN();
    __delay_us(5);
    
    return 0;
}

/**
 * Configura una fecha en el RTC
 * @param weekDay dia de la semana (0 a 7)
 * @param date fecha (1 a 31)
 * @param month mes (1 a 12)
 * @param year año (0 a 99)
 * @return 0 si la escritura fue exitosa, -1 de otro modo
 */
int16_t writeRTCDate(uint8_t weekDay, uint8_t date, uint8_t month, uint8_t year) {
    uint8_t d;
    if(!rtcReady) return -1;
    
    if(weekDay > 7) weekDay = 7;
    else if(weekDay < 1) weekDay = 1;
    if(date > 31) date = 31;
    else if(date < 1) date = 1;
    if(month > 12) month = 12;
    else if(month < 1) month = 1;
    if(year > 99) year = 99;
    
    StartI2C1();
    DS1307_WAIT_IDLE_RETURN();
    d = MasterWriteI2C1(RTC_ADDR | I2C_WRITE);
    if(d < 0) {
        StopI2C1();
        DS1307_WAIT_IDLE_RETURN();
        return -1;
    }
    DS1307_WAIT_IDLE_RETURN();
    MasterWriteI2C1(3);
    DS1307_WAIT_IDLE_RETURN();
    
    // Enviamos
    MasterWriteI2C1(_binToBCD(weekDay));
    DS1307_WAIT_IDLE_RETURN();
    MasterWriteI2C1(_binToBCD(date));
    DS1307_WAIT_IDLE_RETURN();
    MasterWriteI2C1(_binToBCD(month));
    DS1307_WAIT_IDLE_RETURN();
    MasterWriteI2C1(_binToBCD(year));
    DS1307_WAIT_IDLE_RETURN();
    StopI2C1();
    DS1307_WAIT_IDLE_RETURN();
    __delay_us(5);
    
    rtcTime.weekDay = weekDay;
    rtcTime.date = date;
    rtcTime.month = month;
    rtcTime.year = year;
    
    return 0;
}

/**
 * Configura la hora en el RTC
 * @param hours hora (0-12 o 0-23)
 * @param minutes minutos (0 a 59)
 * @param seconds segundos (0 a 59)
 * @param isPM determina si la hora es AM o PM en el caso de que el modo no sea 24 horas, de
 *          otra forma es ignorado
 * @param is24 determina si es modo 24 horas
 * @return 0 si la escritura fue exitosa, -1 de otro modo
 */
int16_t writeRTCTime(uint8_t hours, uint8_t minutes, uint8_t seconds, uint8_t isPM, uint8_t is24) {
    uint8_t data;
    if(!rtcReady) return -1;
    
    if(is24 && hours > 23) hours = 23;
    else if(!is24 && hours > 12) hours = 12;
    if(minutes > 59) minutes = 59;
    if(seconds > 59) seconds = 59;
    
    // Creamos el registro de horas
    data = _binToBCD(hours);
    if(is24) data = bitClear(data, 6);
    else data = bitSet(data, 6);
    if(!is24 && isPM) data = bitSet(data, 5);
    else if(!is24) data = bitClear(data, 5);
    
    StartI2C1();
    DS1307_WAIT_IDLE_RETURN();
    data = MasterWriteI2C1(RTC_ADDR | I2C_WRITE);
    if(data < 0) {
        StopI2C1();
        DS1307_WAIT_IDLE_RETURN();
        return -1;
    }
    DS1307_WAIT_IDLE_RETURN();
    MasterWriteI2C1(0);
    DS1307_WAIT_IDLE_RETURN();
    
    // Enviamos
    MasterWriteI2C1(_binToBCD(seconds));
    DS1307_WAIT_IDLE_RETURN();
    MasterWriteI2C1(_binToBCD(minutes));
    DS1307_WAIT_IDLE_RETURN();
    MasterWriteI2C1(data);
    DS1307_WAIT_IDLE_RETURN();
    StopI2C1();
    DS1307_WAIT_IDLE_RETURN();
    __delay_us(5);
    
    rtcTime.isMode24 = is24;
    rtcTime.isPM = isPM;
    rtcTime.hours = hours;
    rtcTime.minutes = minutes;
    rtcTime.seconds = seconds;
    
    return 0;
}

/**
 * Guarda un byte en la memoria RAM de proposito general del RTC
 * @param data byte a almacenar
 * @param position posicion en la RAM, de 0 a 55
 * @return 0 si la escritura fue exitosa, -1 de otro modo
 */
int16_t writeGpRAM(uint8_t data, uint8_t position) {
    uint8_t d;
    if(!rtcReady) return -1;
    if(position > 55) position = 55;
    
    // Direccionamos el apuntador de memoria del RTC a la direccion
    //  de memoria de proposito general
    StartI2C1();
    DS1307_WAIT_IDLE_RETURN();
    d = MasterWriteI2C1(RTC_ADDR | I2C_WRITE);
    if(d < 0) {
        StopI2C1();
        DS1307_WAIT_IDLE_RETURN();
        return -1;
    }
    DS1307_WAIT_IDLE_RETURN();
    MasterWriteI2C1(0x08 + position);
    DS1307_WAIT_IDLE_RETURN();
    MasterWriteI2C1(data);
    DS1307_WAIT_IDLE_RETURN();
    StopI2C1();
    DS1307_WAIT_IDLE_RETURN();
    __delay_us(5);
    
    return 0;
}

/**
 * Lee un byte de la memoria RAM de proposito general del RTC
 * @param position posicion de donde leer el dato, de 0 a 55
 * @param p puntero donde almacenar el dato leido
 * @return 0 si la lectura fue exitosa, -1 de otro modo
 */
int16_t readGpRAM(uint8_t position, uint8_t *p) {
    uint8_t data;
    if(!rtcReady) return -1;
    if(position > 55) position = 55;
    
    // Direccionamos el apuntador de memoria del RTC a la direccion
    //  de memoria de proposito general
    StartI2C1();
    DS1307_WAIT_IDLE_RETURN();
    data = MasterWriteI2C1(RTC_ADDR | I2C_WRITE);
    if(data < 0) {
        StopI2C1();
        DS1307_WAIT_IDLE_RETURN();
        return -1;
    }
    DS1307_WAIT_IDLE_RETURN();
    MasterWriteI2C1(0x08 + position);
    DS1307_WAIT_IDLE_RETURN();
    
    // Entramos a modo lectura ahora
    RestartI2C1();
    DS1307_WAIT_IDLE_RETURN();
    data = MasterWriteI2C1(RTC_ADDR | I2C_READ);
    if(data < 0) {
        StopI2C1();
        DS1307_WAIT_IDLE_RETURN();
        return -1;
    }
    DS1307_WAIT_IDLE_RETURN();
    
    data = MasterReadI2C1();
    NotAckI2C1();
    DS1307_WAIT_IDLE_RETURN();
    StopI2C1();
    DS1307_WAIT_IDLE_RETURN();
    __delay_us(5);
    
    *p = data;
    return 0;
}

/**
 * Convierte la hora a string con el siguiente formato hh:mm:ss
 * @param buffer donde almacenar el string
 * @param includeSeconds si deben incluirse o no los segundos
 */
void formatTime(char *buffer, uint8_t includeSeconds) {
    // Horas
    if(rtcTime.hours < 10) {
        buffer[0] = '0';
    }
    _ltoa(rtcTime.hours, buffer+1);
    buffer[2] = ':';
    
    // Minutos
    if(rtcTime.minutes < 10) {
        buffer[3] = '0';
        _ltoa(rtcTime.minutes, buffer+4);
    }
    else {
        _ltoa(rtcTime.minutes, buffer+3);
    }
    
    // Segundos
    if(includeSeconds) {
        buffer[5] = ':';
        if(rtcTime.seconds < 10) {
            buffer[6] = '0';
            _ltoa(rtcTime.seconds, buffer+7);
        }
        else {
            _ltoa(rtcTime.seconds, buffer+6);
        }
    }
}