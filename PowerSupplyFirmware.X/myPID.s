.equ    offsetabcCoefficients, 0
.equ    offseterrorHistory, 2
.equ    offsetcontrolOutput, 4
.equ    offsetmeasuredOutput, 6
.equ    offsetcontrolReference, 8
.equ    offsetcoeffFactor, 10
.equ	offsetupperLimit, 12
.equ	offsetlowerLimit, 14

.section .libdsp, code

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; _PID:
; Prototype:
;              _PIDData mPID ( tPID *fooPIDStruct )
;
;
;   controlOutput[n] = controlOutput[n-1]
;                    + errorHistory[n] * abcCoefficients[0]
;                    + errorHistory[n-1] * abcCoefficients[1]
;                    + errorHistory[n-2] * abcCoefficients[2]
;
;  where:
;   abcCoefficients[0] = Kp + Ki*(Ts/2) + Kd/Ts
;   abcCoefficients[1] = -Kp + Ki*(Ts/2) - (2*Kd)/Ts
;   abcCoefficients[2] = Kd/Ts
;   errorHistory[n] = measuredOutput[n] - referenceInput[n]
;  where:
;   abcCoefficients, errorHistory, controlOutput,
;    measuredOutput and controlReference are all members of the data
;    structure _PIDData in pid.h
;
; Input:
;       w0 = Address of tPID data structure

; Return:
;       w0 = Address of tPID data structure
;
; System resources usage:
;       {w0..w5}        used, not restored
;       {w8,w10,w11}    saved, used, restored
;        AccA, AccB     used, not restored
;        CORCON         saved, used, restored
;
; DO and REPEAT instruction usage.
;       0 level DO instruction
;       0 REPEAT intructions

;............................................................................

        .global _mPID                   ; provide global scope to routine
_mPID:

.ifdef __dsPIC33E
	push	DSRPAG
	movpag #0x0001, DSRPAG
.endif

        ; Save working registers.
        push    w8
        push    w10
        push    CORCON                  ; Save CORCON status
	
	mov	#0x00EB, w8		; Integer mode with saturation in accumulators A/B and data memory
	mov	w8, CORCON

        mov [w0 + #offsetabcCoefficients], w8	    ; w8 = Base Address of _abcCoefficients array
        mov [w0 + #offseterrorHistory], w10	    ; w10 = Address of errorHistory array (state/delay line)

        mov [w0 + #offsetcontrolOutput], w1
        mov [w0 + #offsetmeasuredOutput], w2
        mov [w0 + #offsetcontrolReference], w3

        ; Calculate most recent error with saturation, no limit checking required
        lac     w3, a                   ; A = PID.controlReference
        lac     w2, b                   ; B = PID.measuredOutput
        sub     a			; A = PID.controlReference - PID.measuredOutput
	
	mov [w0 + #offsetcoeffFactor], w3   ; Save the number of shifts in w3, it won't be used anymore
	sftac	a, w3			    ; A = (PID.controlReference - PID.measuredOutput) / 2^PID.coeffFactor
        sac.r   a, [w10]		    ; PID.errorHistory[n] = Sat(Rnd(A))

        ; Calculate PID Control Output
        clr     a, [w8]+=2, w4, [w10]+=2, w5            ; w4 = (Kp + Ki*(Ts/2) + Kd/Ts) = a, w5 = errorHistory[n]
	mov	w1, ACCAL				; A = controlOutput[n-1]
        mac     w4*w5, a, [w8]+=2, w4, [w10]+=2, w5     ; A += a * errorHistory[n]
                                                        ; w4 = (-Kp + Ki*(Ts/2) - (2*Kd)/Ts) = b, w5 = errorHistory[n-1]
        mac     w4*w5, a, [w8], w4, [w10]-=2, w5        ; A += b * errorHistory[n-1]
                                                        ; w4 = Kd/Ts = c, w5 = errorHistory[n-2]
        mac     w4*w5, a, [w10]+=2, w5                  ; A += c * errorHistory[n-2]
                                                        ; w5 = errorHistory[n-1]
                                                        ; w10 = &errorHistory[n-2]
	sftac	a, #-16					; shift the result in A because the instruction 'sac.r' uses the high side of A
        sac.r	a, w1                                   ; controlOutput[n] = Sat(Rnd(A))
	
	; Check if the output is above the upper limit
	mov	[w0 + #offsetupperLimit], w2		; use w2, it won't be used anymore
	sub	w1, w2, w3				; controlOutput[n] - PID.upperLimit. Use w3, won't be used anymore.
	bra	LE, checkLess				; if(controlOutput[n] < PID.upperLimit), jump
	mov	[w0 + #offsetupperLimit], w1		; else set the output to the upper limit
	bra	storeOutput
checkLess:
	; Check if the output is below the lower limit
	mov	[w0 + #offsetlowerLimit], w2		; use w2, it won't be used anymore
	sub	w1, w2, w3				; controlOutput[n] - PID.lowerLimit. Use w3, won't be used anymore.
	bra	GT, storeOutput				; if(controlOutput[n] > PID.lowerLimit), jump
	mov	[w0 + #offsetlowerLimit], w1		; else set the output to the lower limit
    
storeOutput:
        mov     w1, [w0 + #offsetcontrolOutput]		; Store the limited output now controlOutput[n]

        ;Update the error history on the delay line
        mov     w5, [w10]               ; errorHistory[n-2] = errorHistory[n-1]
        mov     [w10 + #-4], w5         ; errorHistory[n-1] = errorHistory[n]
        mov     w5, [--w10]

        pop     CORCON                  ; restore CORCON
        pop     w10			; Restore working registers.
        pop     w8

.ifdef __dsPIC33E
	pop	DSRPAG
.endif
        return
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; _PIDInit:
;
; Prototype:
; void PIDInit ( _PIDData *fooPIDStruct )
;
; Operation: This routine clears the delay line elements in the array
;            _ControlHistory, as well as clears the current PID output
;            element, _ControlOutput
;
; Input:
;       w0 = Address of data structure _PIDData (type defined in pid.h)
;
; Return:
;       (void)
;
; System resources usage:
;       w0             used, restored
;
; DO and REPEAT instruction usage.
;       0 level DO instruction
;       0 REPEAT intructions
;
; Program words (24-bit instructions):
;       11
;
; Cycles (including C-function call and return overheads):
;       13
;............................................................................
        .global _mPIDInit		   ; provide global scope to routine
_mPIDInit:

.ifdef __dsPIC33E
	push	DSRPAG
 	movpag #0x0001, DSRPAG
.endif
 
        push	w0
        add	#offsetcontrolOutput, w0   ; Set up pointer for controlOutput
	clr	[w0]                       ; Clear controlOutput
	pop	w0
	push	w0
                                        ;Set up pointer to the base of
                                        ;controlHistory variables within tPID
        mov     [w0 + #offseterrorHistory], w0
                                        ; Clear controlHistory variables
                                        ; within tPID
        clr     [w0++]                  ; ControlHistory[n] = 0
        clr     [w0++]                  ; ControlHistory[n-1] = 0
        clr     [w0]                    ; ControlHistory[n-2] = 0
        pop     w0                      ;Restore pointer to base of tPID

.ifdef __dsPIC33E
	pop	DSRPAG
.endif
        return
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        .end	; end global

