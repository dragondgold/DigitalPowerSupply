/*
 * File:   main.c
 * Author: Andres Torti
 *
 * Created on April 19, 2015, 8:15 PM
 */

#include "definitions.h"
#include <xc.h>
#include <stdlib.h>
#include <libpic30.h>
#include <pps.h>
#include <uart.h>
#include <p33FJ16GS402.h>
#include "CommandParser.h"
#include "smps.h"
#include <stdint.h>
#include "pid.h"
#include "ds1307.h"

// FBS
#pragma config BWRP = WRPROTECT_OFF     // Boot Segment Write Protect (Boot Segment may be written)
#pragma config BSS = NO_FLASH           // Boot Segment Program Flash Code Protection (No Boot program Flash segment)

// FGS
#pragma config GWRP = OFF               // General Code Segment Write Protect (General Segment may be written)
#pragma config GSS = OFF                // General Segment Code Protection (User program memory is not code-protected)

// FOSCSEL
#pragma config FNOSC = FRCPLL           // Oscillator Source Selection (Internal Fast RC (FRC) with PLL)
#pragma config IESO = OFF               // Internal External Switch Over Mode (Start up device with user-selected oscillator source)

// FOSC
#pragma config POSCMD = NONE            // Primary Oscillator Source (Primary oscillator disabled)
#pragma config OSCIOFNC = ON            // OSC2 Pin Function (OSC2 is general purpose digital I/O pin)
#pragma config IOL1WAY = OFF            // Peripheral Pin Select Configuration (Allow multiple reconfigurations)
#pragma config FCKSM = CSDCMD           // Clock Switching and Monitor (Clock switching and Fail-Safe Clock Monitor are disabled, Mon Disabled)

// FWDT
#pragma config WDTPOST = PS32768        // Watchdog Timer Postscaler (1:32,768)
#pragma config WDTPRE = PR128           // WDT Prescaler (1:128)
#pragma config WINDIS = OFF             // Watchdog Timer Window (Watchdog Timer in Non-Window mode)
#pragma config FWDTEN = OFF             // Watchdog Timer Enable (Watchdog timer enabled/disabled by user software)

// FPOR
#pragma config FPWRT = PWR128           // POR Timer Value (128ms)

// FICD
#pragma config ICS = PGD2               // Comm Channel Select (Communicate on PGC2/EMUC1 and PGD2s/EMUD1)
#pragma config JTAGEN = OFF             // JTAG Port Enable (JTAG is disabled)

int16_t x;
extern RTCTime rtcTime;
int main(int argc, char** argv) {   
    uint16_t rtcRetries = 0;
    
    // FRCat 7.48 MHz (+1.5% de la frecuencia central, 4*0.375%)
    //OSCTUNbits.TUN = 4;

    /* Configuramos el oscilador para 40 MIPS
     * La frecuencia nominal del Fast RC (FRC) es de 7.37 MHz
     * FOSC = Fin * M/(N1 * N2), FCY = FOSC/2
     * FOSC = 7.37 * (43)/(2 * 2) = 80 MHz de FOSC, FCY = 40 MHz */
    PLLFBD = 41;                        // M = PLLFBD + 2
    CLKDIVbits.PLLPOST = 0;             // N1 = 2
    CLKDIVbits.PLLPRE = 0;              // N2 = 2
    while(OSCCONbits.LOCK != 1);        // Esperamos el lock del PLL

    /* Configuramos al FRC w/ PLL para funcionar tambien como REFCLK
     * La frecuencia nominal del REFCLK deberia ser de 120 MHz
     * El postcaler del clock auxiliar se configura como 1:1
     *  (APSTSCLR<2:0> = 111) para la correcta operación del módulo PWM
     * ((FRC* 16) / APSTSCLR) = (7.37 * 16) / 1 = 117.92 MHz */
    ACLKCONbits.FRCSEL = 1;             // Usamos el oscilador FRC como fuente del REFCLK
    ACLKCONbits.SELACLK = 1;            // La fuente de clock esta dada por el oscilador auxiliar
    ACLKCONbits.APSTSCLR = 7;           // Divisor del clock auxiliar por 1
    ACLKCONbits.ENAPLL = 1;             // PLL auxiliar habilitado (multiplicamos x16)
    while(ACLKCONbits.APLLCK != 1);     // Esperamos el clock del PLL Auxiliar

    INTCON1bits.NSTDIS = 0;             // Nesting de interrupciones habilitado

    RX_TRIS = 1;
    TX_TRIS = 0;

    PPSUnLock;
    RPINR18bits.U1RXR = 12;             // U1RX a RP12
    RPOR5bits.RP11R = 0b000011;         // U1TX en RP11
    PPSLock;

    setupI2C();
    /*
    while((x = initRTC()) < 0 && rtcRetries < 20) {
        ++rtcRetries;
        I2C1CONbits.I2CEN = 0;
        __delay_ms(8);
        I2C1CONbits.I2CEN = 1;
        __delay_ms(2);
    }*/
    smpsInit();
    initCommandParser();
    
    while(1){
        commandParserTasks();
        smpsTasks();
        rtcTasks();
    }

    return (EXIT_SUCCESS);
}
