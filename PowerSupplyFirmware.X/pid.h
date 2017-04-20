#ifndef PID_H
#define	PID_H

#include <xc.h> 
#include <stdint.h>
#include <dsp.h>

typedef struct {
    int16_t kp;
    int16_t ki;
    
    uint16_t measuredOutput;
    int16_t setpoint;
    
    int32_t integralTerm;
    uint16_t n;
} myPIData;

extern int16_t myPI(myPIData *data);

#endif

