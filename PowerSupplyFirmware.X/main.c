/*
 * File:   main.c
 * Author: Andres Torti
 *
 * Created on April 19, 2015, 8:15 PM
 */

// FSEC
#pragma config BWRP = OFF               // Boot Segment Write-Protect bit (Boot Segment may be written)
#pragma config BSS = DISABLED           // Boot Segment Code-Protect Level bits (No Protection (other than BWRP))
#pragma config BSEN = OFF               // Boot Segment Control bit (No Boot Segment)
#pragma config GWRP = OFF               // General Segment Write-Protect bit (General Segment may be written)
#pragma config GSS = DISABLED           // General Segment Code-Protect Level bits (No Protection (other than GWRP))
#pragma config CWRP = OFF               // Configuration Segment Write-Protect bit (Configuration Segment may be written)
#pragma config CSS = DISABLED           // Configuration Segment Code-Protect Level bits (No Protection (other than CWRP))
#pragma config AIVTDIS = OFF            // Alternate Interrupt Vector Table bit (Disabled AIVT)

// FBSLIM
#pragma config BSLIM = 0x1FFF           // Boot Segment Flash Page Address Limit bits (Boot Segment Flash page address  limit)

// FSIGN

// FOSCSEL (Select Internal FRC at POR)
#pragma config FNOSC = FRC              // Oscillator Source Selection (Fast RC Oscillator)
#pragma config IESO = OFF               // Two-speed Oscillator Start-up Disabled

// FOSC
#pragma config POSCMD = NONE            // Primary Oscillator Mode Select bits (Primary Oscillator disabled)
#pragma config OSCIOFNC = ON            // OSC2 Pin is I/O
#pragma config IOL1WAY = OFF            // Peripheral pin select configuration bit (Allow multiple reconfigurations)
#pragma config FCKSM = CSECME           // Clock Switching Mode bits (Both Clock switching and Fail-safe Clock Monitor are enabled)
#pragma config PLLKEN = ON              // PLL Lock Enable Bit (Clock switch will wait for the PLL lock signal)

// FWDT
#pragma config WDTPOST = PS32768        // Watchdog Timer Postscaler bits (1:32,768)
#pragma config WDTPRE = PR128           // Watchdog Timer Prescaler bit (1:128)
#pragma config WDTEN = OFF              // Watchdog Timer Enable bits (WDT and SWDTEN disabled)
#pragma config WINDIS = OFF             // Watchdog Timer Window Enable bit (Watchdog Timer in Non-Window mode)
#pragma config WDTWIN = WIN25           // Watchdog Timer Window Select bits (WDT Window is 25% of WDT period)

// FICD
#pragma config ICS = PGD1               // ICD Communication Channel Select bits (Communicate on PGEC1 and PGED1)
#pragma config JTAGEN = OFF             // JTAG Enable bit (JTAG is disabled)
#pragma config BTSWP = OFF              // BOOTSWP Instruction Enable/Disable bit (BOOTSWP instruction is disabled)

// FDEVOPT
#pragma config PWMLOCK = OFF            // PWMx Lock Enable bit (PWM registers may be written without key sequence)
#pragma config ALTI2C1 = OFF            // Alternate I2C1 Pin bit (I2C1 mapped to SDA1/SCL1 pins)
#pragma config ALTI2C2 = OFF            // Alternate I2C2 Pin bit (I2C2 mapped to SDA2/SCL2 pins)
#pragma config DBCC = OFF               // DACx Output Cross Connection bit (No Cross Connection between DAC outputs)

// FALTREG
#pragma config CTXT1 = OFF              // Specifies Interrupt Priority Level (IPL) Associated to Alternate Working Register 1 bits (Not Assigned)
#pragma config CTXT2 = OFF              // Specifies Interrupt Priority Level (IPL) Associated to Alternate Working Register 2 bits (Not Assigned)

// FBTSEQ
#pragma config BSEQ = 0xFFF             // Relative value defining which partition will be active after device Reset; the partition containing a lower boot number will be active (Boot Sequence Number bits)
#pragma config IBSEQ = 0xFFF            // The one's complement of BSEQ; must be calculated by the user and written during device programming. (Inverse Boot Sequence Number bits)

#include "definitions.h"
#include <xc.h>
#include <stdlib.h>
#include <libpic30.h>
#include <pps.h>
#include <uart.h>
#include "command-parser.h"
#include "smps.h"
#include <stdint.h>
#include "pid.h"
#include "debug-uart.h"

int main(int argc, char** argv) {   
    // FRCat 7.48 MHz (+1.5% de la frecuencia central, 4*0.375%)
    //OSCTUNbits.TUN = 4;

    /* Configuramos el oscilador para 70 MIPS
     * La frecuencia nominal del Fast RC (FRC) es de 7.37 MHz
     * FOSC = Fin * M/(N1 * N2), FCY = FOSC/2
     * FOSC = (7.37 * 76)/(2 * 2) = 140 MHz de FOSC, FCY = 70 MHz */
    PLLFBD = 74;                        // M = PLLFBD + 2 = 76
    CLKDIVbits.PLLPOST = 0;             // N2 = 2
    CLKDIVbits.PLLPRE = 0;              // N1 = 2
    
    // Iniciamos el cambio de oscilador a FRC con PLL (NOSC=0b001)
    __builtin_write_OSCCONH(0x01);
    __builtin_write_OSCCONL(OSCCON | 0x01);
    while(OSCCONbits.COSC != 0);            // Esperamos que ocurra el cambio de oscilador
    while(OSCCONbits.LOCK != 1);            // Esperamos el lock del PLL

    /* Configuramos al FRC con PLL para funcionar tambien como REFCLK
     * La frecuencia nominal del REFCLK es de 120 MHz
     * El postcaler del clock auxiliar se configura como 1:1
     *  (APSTSCLR<2:0> = 111) para la correcta operación del módulo PWM
     * ((FRC * 16) / APSTSCLR) = (7.37 * 16) / 1 = 117.92 MHz */
    ACLKCONbits.FRCSEL = 1;             // Usamos el oscilador FRC como fuente del REFCLK
    ACLKCONbits.SELACLK = 1;            // La fuente de clock esta dada por el oscilador auxiliar
    ACLKCONbits.APSTSCLR = 7;           // Divisor del clock auxiliar por 1
    ACLKCONbits.ENAPLL = 1;             // PLL auxiliar habilitado (multiplicamos x16)
    while(ACLKCONbits.APLLCK != 1);     // Esperamos el lock del PLL Auxiliar

    INTCON1bits.NSTDIS = 0;             // Nesting de interrupciones habilitado

    PPSUnLock;
    // Control UART
    RPINR18bits.U1RXR = 44;             // U1RX a RP44
    RPOR5bits.RP43R = 0b000001;         // U1TX en RP43
    
    // Debug UART
    RPINR19bits.U2RXR = 44;             // U2RX a RP46
    RPOR6bits.RP45R = 0b000011;         // U2TX en RP45
    PPSLock;

    smpsInit();
    initCommandParser();
    initDebugUART();
    
    //writeDebugUART("System ready!");
    //writeDebugUART("Version: " SYSTEM_VERSION);
    
    FAULT_TRIS = 0;
    LATBbits.LATB5 = 1;
    LATBbits.LATB5 = 0;
    LATBbits.LATB5 = 1;
    LATBbits.LATB5 = 0;
    LATBbits.LATB5 = 1;
    while(1){
        //LATBbits.LATB5 = !LATBbits.LATB5;
        //commandParserTasks();
        //smpsTasks();
    }

    return (EXIT_SUCCESS);
}
