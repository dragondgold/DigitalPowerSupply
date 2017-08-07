#include <xc.h>

// Primary address error trap function declarations
// Use if INTCON2 ALTIVT=1
void __attribute__((interrupt,no_auto_psv)) _OscillatorFail()
{
        INTCON1bits.OSCFAIL = 0;
        int trap = 1;
        while (trap);
}

void __attribute__((interrupt,no_auto_psv)) _AddressError()
{
        INTCON1bits.ADDRERR = 0;
        int trap = 1;
        while (trap);
}
void __attribute__((interrupt,no_auto_psv)) _StackError()
{
        INTCON1bits.STKERR = 0;
        int trap = 1;
        while (trap);
}

void __attribute__((interrupt,no_auto_psv)) _MathError()
{
        INTCON1bits.MATHERR = 0;
        int trap = 1;
        while (trap);
}

// Alternate address error trap function declarations
// Use if INTCON2 ALTIVT=0
void __attribute__((interrupt,no_auto_psv)) _AltOscillatorFail()
{
        INTCON1bits.OSCFAIL = 0;
        int trap = 1;
        while (trap);
}

void __attribute__((interrupt,no_auto_psv)) _AltAddressError()
{
        INTCON1bits.ADDRERR = 0;
        int trap = 1;
        while (trap);
}

void __attribute__((interrupt,no_auto_psv)) _AltStackError()
{
        INTCON1bits.STKERR = 0;
        int trap = 1;
        while (trap);
}

void __attribute__((interrupt,no_auto_psv)) _AltMathError()
{
        INTCON1bits.MATHERR = 0;
        int trap = 1;
        while (trap);
}

/*
// This executes when an interrupt occurs for an interrupt source with an
// improperly defined or undefined interrupt handling routine.
void __attribute__((interrupt,no_auto_psv)) _DefaultInterrupt() {
        int trap = 1;
        //while (trap);
}*/
