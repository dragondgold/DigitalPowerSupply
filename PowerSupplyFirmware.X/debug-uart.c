#include "xc.h"
#include "debug-uart.h"
#include "definitions.h"
#include <uart.h>

void initDebugUART(void) {
    RX_DEBUG_TRIS = 1;
    TX_DEBUG_TRIS = 0;
    
    U2MODEbits.UARTEN = 0;
    U2MODEbits.IREN = 0;                // IrDA deshabilitado
    U2MODEbits.RTSMD = 1;               // Modo Simplex
    U2MODEbits.UEN = 0b00;              // Solo usamos pines RX y TX
    U2MODEbits.LPBACK = 0;              // Loopback desactivado
    U2MODEbits.ABAUD = 0;               // Autobaud desactivado
    U2MODEbits.URXINV = 0;              // No invertimos pin RX
    U2MODEbits.PDSEL = 0b00;            // 8-bit, sin paridad
    U2MODEbits.STSEL = 0;               // 1 bit de stop
    U2MODEbits.BRGH = 0;
    U2BRG = 37;                         // 115200 baudios a 70 MIPS
    
    U2STAbits.URXISEL = 0;
    U2STAbits.ADDEN = 0;
    U2STAbits.UTXINV = 0;               // No invertimos pin TX
    U2STAbits.UTXBRK = 0;               // Sync Break desactivado
    U2STAbits.URXISEL = 0b00;           // Interrupción al recibir solo un caracter
    U2STAbits.UTXEN = 0;
    
    U2MODEbits.UARTEN = 1;              // UART habilitado
    U2STAbits.UTXEN = 1;                // TX habilitado (debe habilitarse después
                                        // de haber habilitado el modulo)
}

void writeDebugUART(char *str) {
    putsUART2((unsigned int *)str);
    putcUART2('\n');
}
