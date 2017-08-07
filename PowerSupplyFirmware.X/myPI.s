.global _myPI

; https://www.embeddedrelated.com/showarticle/121.php
; w0 = Puntero a la estructura del controlador PI

_myPI:
    push    w8
    push    w9
    push    w10
    push    w11
    push    w12
    push    w13
    push    w14
    push    CORCON

    mov #0x00EB, w8	; Configuramos registro CORCON en modo integer y con saturación
    mov w8, CORCON	;  normal en los acumuladores y memoria de datos

    mov [w0 + 0], w6	; Kp
    mov [w0 + 2], w4	; Ki
    mov [w0 + 4], w1	; Salida obtenida del sistema (por ejemplo, valor leido del ADC)
    mov [w0 + 6], w2	; Setpoint
    add	w0, #8, w9	; Direccion de acumulacion integral de 32 bit
    mov [w0 + 12], w5	; Factor de escala N

; Calculo del error con saturacion			
    lac     w2, a	; A = Setpoint
    lac     w1, b	; B = Salida Medida
    sub     a		; A = Setpoint - Salida Medida
    sac	    a, w7	; Guardamos el error saturado a 16 bit en w7	

; Cargamos los 32 bit de la acumulacion integral al acumulador B
    mov	    [w9++], w0	; Guardamos la parte baja
    mov	    w0, ACCBL 
    mov	    [w9], w0	; Guardamos la parte alta
    mov	    w0, ACCBH

; Calculamos el termino integral:
; integral = integral + Ki*e ---- Ki ya está multiplicado por el período de muestreo
; b	   =    b     + w4*w7
    mac	    w4*w7, b

; Calculamos el termino proporcional:
; proporcional = Kp*e
;      a      =  w6*w7
    mpy	    w6*w7, a

; Guardamos el termino integral
    mov	    ACCBH, w0	; Guardamos la parte alta
    mov	    w0, [w9--]
    mov	    ACCBL, w0	; Guardamos la parte baja
    mov	    w0, [w9]

; Calculo de la salida del PID
    sftac   a, w5	; Desplazamos el termino proporcional N veces a la derecha
    sftac   b, #16	; Desplazamos el termino integral 16 veces a la derecha
    add	    a		; Sumamos los dos acumuladores

    sftac   a, #-16	; Desplazamos el proporcional 16 veces a la izquierda
			;  porque la instrucción 'sac' toma los bits ACCxH del
			;  acumulador
    sac.r   a, w0	; Como la función definida en C devuelve un int16_t el
			;  compilador retorna el contenido de w0

    pop	    CORCON
    pop     w14
    pop     w13
    pop     w12
    pop     w11
    pop     w10
    pop     w9
    pop     w8
    return
.end
