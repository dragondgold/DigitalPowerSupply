#ifndef PID_H
#define	PID_H

#include <xc.h> 
#include <stdint.h>

typedef struct {
    int16_t* abcCoefficients;       /* Pointer to A, B & C coefficients located in X-space */
                                    /* These coefficients are derived from */
                                    /* the PID gain values - Kp, Ki and Kd */
    int16_t* errorHistory;          /* Pointer to 3 delay-line samples located in Y-space */
                                    /* with the first sample being the most recent */
    int16_t controlOutput;          /* PID Controller Output  */
    int16_t measuredOutput;         /* Measured Output sample */
    int16_t controlReference;       /* Reference Input sample */
    
    int16_t coeffFactor;            /* Power of 2 factor used to multiply each coefficient
                                     *  to get more resolution. The error obtained in each
                                     *  iteration will be divided by this factor to compensate.
                                     * Put 0 in this if the coefficients were not multiplied.
                                     * A value of 4 here would be mean all the coeficients were
                                     *  multiplied by 2^4 = 16 */
    
    int16_t upperLimit;             /* Upper and lower limits of the PID output */
    int16_t lowerLimit;
} _PIDData;

extern void mPID(_PIDData* data);
extern void mPIDInit (_PIDData* controller);

#endif

