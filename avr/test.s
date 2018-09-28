.include "boilerplate.inc"
.include "iosyms.inc"

; ------------------------------------------------------------------------------
.section .text


FUNCTION led_num
	in r25, PORTB
	andi r25, 0xf8
	out PORTB, r25
	in r25, PORTD
	andi r25, 0x0f
	out PORTD, r25
	cpi r24, 0x10
	LOC brsh RETURN
	in r19, PORTB
	ldi r25, 0
	movw ZL, r24
	subi ZL, lo8(-(led_b210))
	sbci ZH, hi8(-(led_b210))
	ld r18, Z
	or r18, r19
	out PORTB, r18
	in r18, PORTD
	movw ZL, r24
	subi ZL, lo8(-(led_d7654))
	sbci ZH, hi8(-(led_d7654))
	ld r24, Z
	or r24, r18
	out PORTD, r24

  LOC RETURN
	ret
ENDFUNC led_num


FUNCTION led_onoff
	cpse r24, __zero_reg__
	LOC rjmp SWITCH_ON
	cbi PORTD, 3
	ret

  LOC SWITCH_ON
	sbi PORTD, 3
	ret
ENDFUNC led_onoff


FUNCTION count
	lds r24, tick
	lds r25, tick+1
	lds XL, tick+2
	lds XH, tick+3
	adiw r24, 1
	adc XL, __zero_reg__
	adc XH, __zero_reg__
	sts tick, r24
	sts tick+1, r25
	sts tick+2, XL
	sts tick+3, XH
	lds r20, tick
	lds r21, tick+1
	lds r22, tick+2
	lds r23, tick+3
	in r24, PORTB
	andi r24, 0xf8
	out PORTB, r24
	in r24, PORTD
	andi r24, 0x0f
	out PORTD, r24
	in r19, PORTB
	mov r24, r21
	andi r24, 0x0f
	ldi r25, 0
	movw ZL, r24
	subi ZL, lo8(-(led_b210))
	sbci ZH, hi8(-(led_b210))
	ld r18, Z
	or r18, r19
	out PORTB, r18
	in r18, PORTD
	movw ZL, r24
	subi ZL, lo8(-(led_d7654))
	sbci ZH, hi8(-(led_d7654))
	ld r24, Z
	or r24, r18
	out PORTD, r24
	ret
ENDFUNC count


FUNCTION __vector_12, 3
	push r1
	push r0
	in r0, SREG
	push r0
	clr __zero_reg__
	pop r0
	out SREG, r0
	pop r0
	pop r1
	reti
ENDFUNC __vector_12


FUNCTION calculate_trig_high_low
	lds r24, trig_level
	lds r25, trig_hist
	mov r20, r24
	ldi r21, 0
	ldi r18, 0xff
	ldi r19, 0
	sub r18, r25
	sbc r19, __zero_reg__
	cp r20, r18
	cpc r21, r19
	LOC brge TOO_HIGH
	mov r18, r24
	add r18, r25
	sts trig_high, r18
	cp r25, r24
	LOC brsh TOO_LOW

  LOC NORMAL
	sub r24, r25
	sts trig_low, r24
	ret

  LOC TOO_HIGH
	ldi r18, 0xff
	sts trig_high, r18
	cp r25, r24
	LOC brlo NORMAL

  LOC TOO_LOW
	ldi r24, 1
	sts trig_low, r24
	ret
ENDFUNC calculate_trig_high_low



FUNCTION set_sample_rate
	; In: r24 = uint8_t idx
	mov ZL, r24
	ldi ZH, 0
	lsl ZL
	rol ZH
	subi ZL, lo8(-(sample_rate_clks))
	sbci ZH, hi8(-(sample_rate_clks))
	ld r18, Z
	ldd r19, Z+1
	ldi r24, (1 << WGM12) + 1
	sts TCCR1B, r24
	sts OCR1AH, r19
	sts OCR1AL, r18
	subi r18, -6
	sbci r19, -1
	ldi XL, 0xc5
	ldi XH, 0x4e
	call __umulhisi3
	lsr r25
	ror r24
	lsr r25
	ror r24
	sbiw r24, 0
	LOC breq SET_PRESCALER_2
	lsr r25
	ror r24
	sbiw r24, 0
	LOC breq SET_PRESCALER_2
	lsr r25
	ror r24
	sbiw r24, 0
	LOC breq SET_PRESCALER_2
	lsr r25
	ror r24
	sbiw r24, 0
	LOC breq SET_PRESCALER_2
	lsr r25
	ror r24
	sbiw r24, 0
	LOC breq SET_PRESCALER_3
	lsr r25
	ror r24
	sbiw r24, 0
	LOC breq SET_PRESCALER_4
	lsr r25
	ror r24
	sbiw r24, 0
	LOC breq SET_PRESCALER_5
	lsr r25
	ror r24
	or r24, r25
	LOC breq SET_PRESCALER_6

	ldi r24, (1 << ADEN) + (1 << ADATE) + (1 << ADIE) + 7

  LOC SET_ADCSRA
	sts ADCSRA, r24
	sts TCNT1H, __zero_reg__
	sts TCNT1L, __zero_reg__
	ldi r24, (1 << OCIE1B)
	sts TIMSK1, r24
	ret

  LOC SET_PRESCALER_2
	ldi r24, (1 << ADEN) + (1 << ADATE) + (1 << ADIE) + 2
	LOC rjmp SET_ADCSRA

  LOC SET_PRESCALER_3
	ldi r24, 4

  LOC SET_PRESCALE_N_MINUS_1
	subi r24, 1
	ori r24, (1 << ADEN) + (1 << ADATE) + (1 << ADIE)
	LOC rjmp SET_ADCSRA

  LOC SET_PRESCALER_4
	ldi r24, 5
	LOC rjmp SET_PRESCALE_N_MINUS_1

  LOC SET_PRESCALER_5
	ldi r24, 6
	LOC rjmp SET_PRESCALE_N_MINUS_1

  LOC SET_PRESCALER_6
	ldi r24, 7
	LOC rjmp SET_PRESCALE_N_MINUS_1
ENDFUNC set_sample_rate


FUNCTION select_vref
	in r25, PORTC
	andi r25, ~ 0x38
	out PORTC, r25
	in r19, PORTC
	ldi r25, 0
	movw ZL, r24
	subi ZL, lo8(-(vref_raw))
	sbci ZH, hi8(-(vref_raw))
	ld r18, Z
	lsl r18
	lsl r18
	lsl r18
	or r18, r19
	out PORTC, r18
	lsl r24
	rol r25
	movw ZL, r24
	subi ZL, lo8(-(vref_scale))
	sbci ZH, hi8(-(vref_scale))
	ld r24, Z
	ldd r25, Z+1
	sts vscale+1, r25
	sts vscale, r24
	ret
ENDFUNC select_vref

.equ .L_RECV_BUF_SIZE, 4
.equ .L_END_OF_TRACE, 0xffff

FUNCTION recv_push
	lds ZL, recv_idx
	ldi r25, 1
	add r25, ZL
	sts recv_idx, r25
	mov __tmp_reg__, ZL
	lsl r0
	sbc ZH, ZH
	subi ZL, lo8(-(recv_buf))
	sbci ZH, hi8(-(recv_buf))
	st Z, r24
	cpi r25, .L_RECV_BUF_SIZE
	LOC brlt RETURN
	sts recv_idx, __zero_reg__

  LOC RETURN
	ret
ENDFUNC recv_push


FUNCTION recvd_pop
	lds ZL, recv_idx
	subi ZL, 1
	sbrc ZL, 7
	LOC rjmp WRAP_AROUND
	sts recv_idx, ZL
	mov __tmp_reg__, ZL
	lsl r0
	sbc ZH, ZH
	subi ZL, lo8(-(recv_buf))
	sbci ZH, hi8(-(recv_buf))
	ld r24, Z
	ret

  LOC WRAP_AROUND
	ldi r24, .L_RECV_BUF_SIZE - 1
	sts recv_idx, r24
	ldi ZL, .L_RECV_BUF_SIZE - 1
	ldi ZH, 0
	subi ZL, lo8(-(recv_buf))
	sbci ZH, hi8(-(recv_buf))
	ld r24, Z
	ret
ENDFUNC recvd_pop


FUNCTION __vector_18, 15 ; USART_RX_vect
	push r1
	push r0
	in r0, SREG
	push r0
	clr __zero_reg__
	push r18
	push r19
	push r20
	push r21
	push r22
	push r23
	push r24
	push r25
	push XL
	push XH
	push ZL
	push ZH
/* prologue: Signal */
	lds r24, UDR0
	ldi r25, - '0'
	add r25, r24
	cpi r25, 10
	LOC brsh MAY_BE_VALID_CHAR
	lds ZL, recv_idx
	ldi r24, 1
	add r24, ZL
	sts recv_idx, r24
	mov __tmp_reg__, ZL
	lsl r0
	sbc ZH, ZH
	subi ZL, lo8(-(recv_buf))
	sbci ZH, hi8(-(recv_buf))
	st Z, r25
	cpi r24, .L_RECV_BUF_SIZE
	LOC brlt RETURN

  LOC WRAP_AROUND
	sts recv_idx, __zero_reg__

  LOC RETURN
/* epilogue start */
	pop ZH
	pop ZL
	pop XH
	pop XL
	pop r25
	pop r24
	pop r23
	pop r22
	pop r21
	pop r20
	pop r19
	pop r18
	pop r0
	out SREG, r0
	pop r0
	pop r1
	reti

  LOC MAY_BE_VALID_CHAR
	ldi r25, - 'a'
	add r25, r24
	cpi r25, 6
	LOC brsh NOT_LOWERCASE_HEX
	lds ZL, recv_idx
	ldi r25, 1
	add r25, ZL
	sts recv_idx, r25
	mov __tmp_reg__, ZL
	lsl r0
	sbc ZH, ZH
	subi ZL, lo8(-(recv_buf))
	sbci ZH, hi8(-(recv_buf))
	subi r24, 'a' - 10
	st Z, r24
	cpi r25, .L_RECV_BUF_SIZE
	LOC brlt RETURN
	sts recv_idx, __zero_reg__
	LOC rjmp RETURN

  LOC NOT_LOWERCASE_HEX
	ldi r25, - 'A'
	add r25, r24
	cpi r25, 6
	LOC brlo UPPERCASE_HEX
	cpi r24, 'R'
	brne .+2
	LOC rjmp CHAR_R_VOLTAGE_REF
	brsh .+2
	LOC rjmp LOWER_THAN_CHAR_R
	cpi r24, 'T'
	brne .+2
	LOC rjmp CHAR_T_TRIG_TYPE
	brsh .+2
	LOC rjmp CHAR_S_SAMPLE_TIME
	cpi r24, 'W'
	LOC brne RETURN

  ; CHAR_W_WIDTH
	lds ZL, recv_idx
	ldi XL, 0xff
	add XL, ZL
	sbrc XL, 7
	rjmp .L96
	mov __tmp_reg__, XL
	lsl r0
	sbc XH, XH
	subi XL, lo8(-(recv_buf))
	sbci XH, hi8(-(recv_buf))
	ld r24, X
	ldi r25, 0
	subi ZL, 2
	cpi ZL, 0xff
	brne .+2
	rjmp .L53
	sts recv_idx, ZL
	mov __tmp_reg__, ZL
	lsl r0
	sbc ZH, ZH
.L52:
	subi ZL, lo8(-(recv_buf))
	sbci ZH, hi8(-(recv_buf))
	ld r18, Z
	ldi r19, 16
	mul r18, r19
	add r24, r0
	adc r25, r1
	clr __zero_reg__
	lsl r24
	rol r25
	swap r24
	swap r25
	andi r25, 0xf0
	eor r25, r24
	andi r24, 0xf0
	eor r25, r24
	sts width+1, r25
	sts width, r24
	ldi r24, lo8(.L_END_OF_TRACE)
	ldi r25, hi8(.L_END_OF_TRACE)
	sts t+1, r25
	sts t, r24
	LOC rjmp RETURN

  LOC UPPERCASE_HEX
	lds ZL, recv_idx
	ldi r25, 1
	add r25, ZL
	sts recv_idx, r25
	mov __tmp_reg__, ZL
	lsl r0
	sbc ZH, ZH
	subi ZL, lo8(-(recv_buf))
	sbci ZH, hi8(-(recv_buf))
	subi r24, 'A' - 10
	st Z, r24
	cpi r25, .L_RECV_BUF_SIZE
	brlt .+2
	LOC rjmp WRAP_AROUND
	LOC rjmp RETURN

  LOC CHAR_T_TRIG_TYPE
	lds ZL, recv_idx
	subi ZL, 1
	sbrc ZL, 7
	rjmp .L59
	sts recv_idx, ZL
	mov __tmp_reg__, ZL
	lsl r0
	sbc ZH, ZH
.L60:
	subi ZL, lo8(-(recv_buf))
	sbci ZH, hi8(-(recv_buf))
	ld r24, Z
	ldi r25, 0
	sts trig_type+1, r25
	sts trig_type, r24
	lds r24, trig_level
	lds r25, trig_hist
.L91:
	mov r20, r24
	ldi r21, 0
	ldi r18, 0xff
	ldi r19, 0
	sub r18, r25
	sbc r19, __zero_reg__
	cp r20, r18
	cpc r21, r19
	brge .L77
	mov r18, r24
	add r18, r25
.L65:
	sts trig_high, r18
	cp r25, r24
	brsh .L78
	sub r24, r25
.L66:
	sts trig_low, r24
	LOC rjmp RETURN

  LOC LOWER_THAN_CHAR_R
	cpi r24, 'H'
	brne .+2
	LOC rjmp CHAR_H_HYSTERESIS
	cpi r24, 'L'
	breq .+2
	LOC rjmp RETURN

  ; CHAR_L_TRIG_LEVEL
	lds ZL, recv_idx
	ldi XL, 0xff
	add XL, ZL
	sbrc XL, 7
	rjmp .L97
	mov __tmp_reg__, XL
	lsl r0
	sbc XH, XH
	subi XL, lo8(-(recv_buf))
	sbci XH, hi8(-(recv_buf))
	ld r24, X
	subi ZL, lo8(-(-2))
	cpi ZL, 0xff
	brne .+2
	rjmp .L56
	sts recv_idx, ZL
	mov __tmp_reg__, ZL
	lsl r0
	sbc ZH, ZH
.L55:
	subi ZL, lo8(-(recv_buf))
	sbci ZH, hi8(-(recv_buf))
	ld r25, Z
	ldi r18, 16
	mul r25, r18
	add r24, r0
	clr __zero_reg__
	sts trig_level, r24
	lds r25, trig_hist
	rjmp .L91
.L77:
	ldi r18, 0xff
	rjmp .L65
.L78:
	ldi r24, 1
	rjmp .L66

  LOC CHAR_R_VOLTAGE_REF
	lds ZL, recv_idx
	subi ZL,  1
	sbrc ZL, 7
	rjmp .L67
	sts recv_idx, ZL
	mov __tmp_reg__, ZL
	lsl r0
	sbc ZH, ZH
.L68:
	subi ZL, lo8(-(recv_buf))
	sbci ZH, hi8(-(recv_buf))
	ld r24, Z
	andi r24, 3
	in r25, PORTC
	andi r25, 0xc7
	out PORTC, r25
	in r19, PORTC
	ldi r25, 0
	movw ZL, r24
	subi ZL, lo8(-(vref_raw))
	sbci ZH, hi8(-(vref_raw))
	ld r18, Z
	lsl r18
	lsl r18
	lsl r18
	or r18, r19
	out PORTC, r18
	lsl r24
	rol r25
	movw ZL, r24
	subi ZL, lo8(-(vref_scale))
	sbci ZH, hi8(-(vref_scale))
	ld r24, Z
	ldd r25, Z+1
	sts vscale+1, r25
	sts vscale, r24
	LOC rjmp RETURN

  LOC CHAR_S_SAMPLE_TIME
	lds ZL, recv_idx
	subi ZL, 1
	sbrc ZL, 7
	rjmp .L98
	sts recv_idx, ZL
	mov __tmp_reg__, ZL
	lsl r0
	sbc ZH, ZH
.L50:
	subi ZL, lo8(-(recv_buf))
	sbci ZH, hi8(-(recv_buf))
	ld ZL, Z
	andi ZL, 7
	ldi ZH, 0
	lsl ZL
	rol ZH
	subi ZL, lo8(-(sample_rate_clks))
	sbci ZH, hi8(-(sample_rate_clks))
	ld r18, Z
	ldd r19, Z+1
	ldi r24, 0x09
	sts TCCR1B, r24
	sts OCR1AH, r19
	sts OCR1AL, r18
	subi r18, -6
	sbci r19, -1
	ldi XL, -59
	ldi XH, 78
	call __umulhisi3
	lsr r25
	ror r24
	lsr r25
	ror r24
	sbiw r24, 0
	brne .+2
	rjmp .L81
	lsr r25
	ror r24
	sbiw r24, 0
	brne .+2
	rjmp .L81
	lsr r25
	ror r24
	sbiw r24, 0
	brne .+2
	rjmp .L81
	lsr r25
	ror r24
	sbiw r24, 0
	brne .+2
	rjmp .L81
	lsr r25
	ror r24
	sbiw r24, 0
	brne .+2
	rjmp .L82
	lsr r25
	ror r24
	sbiw r24, 0
	brne .+2
	rjmp .L83
	lsr r25
	ror r24
	sbiw r24, 0
	brne .+2
	rjmp .L84
	lsr r25
	ror r24
	or r24, r25
	brne .+2
	rjmp .L99
	ldi r24, lo8(-81)
.L69:
	sts ADCSRA, r24
	sts TCNT1H, __zero_reg__
	sts TCNT1L, __zero_reg__
	ldi r24, 4
	sts TIMSK1, r24
	ldi r24, lo8(.L_END_OF_TRACE)
	ldi r25, hi8(.L_END_OF_TRACE)
	sts t+1, r25
	sts t, r24
	LOC rjmp RETURN

  LOC CHAR_H_HYSTERESIS
	lds ZL, recv_idx
	subi ZL, 1
	sbrc ZL, 7
	rjmp .L63
	sts recv_idx, ZL
	mov __tmp_reg__, ZL
	lsl r0
	sbc ZH, ZH
.L64:
	subi ZL, lo8(-(recv_buf))
	sbci ZH, hi8(-(recv_buf))
	ld r25, Z
	sts trig_hist, r25
	lds r24, trig_level
	rjmp .L91
.L53:
	ldi r18, 3
	sts recv_idx, r18
	ldi ZL, 3
	ldi ZH, 0
	rjmp .L52
.L98:
	ldi r24, 3
	sts recv_idx, r24
	ldi ZL, 3
	ldi ZH, 0
	rjmp .L50
.L81:
	ldi r24, -86
	rjmp .L69
.L96:
	lds r24, recv_buf+3
	ldi r25, 0
	ldi r18, 2
	sts recv_idx, r18
	ldi ZL, 2
	ldi ZH, 0
	rjmp .L52
.L67:
	ldi r24, 3
	sts recv_idx, r24
	ldi ZL, 3
	ldi ZH, 0
	rjmp .L68
.L97:
	lds r24, recv_buf+3
	ldi r25, 2
	sts recv_idx, r25
	ldi ZL, 2
	ldi ZH, 0
	rjmp .L55
.L63:
	ldi r24, 3
	sts recv_idx, r24
	ldi ZL, 3
	ldi ZH, 0
	rjmp .L64
.L59:
	ldi r24, 3
	sts recv_idx, r24
	ldi ZL, 3
	ldi ZH, 0
	rjmp .L60
.L56:
	ldi r25, 3
	sts recv_idx, r25
	ldi ZL, 3
	ldi ZH, 0
	rjmp .L55
.L83:
	ldi r24, 5
.L70:
	subi r24, 1
	ori r24, 0xa8
	rjmp .L69
.L82:
	ldi r24, 4
	rjmp .L70
.L99:
	ldi r24, 7
	rjmp .L70
.L84:
	ldi r24, 6
	rjmp .L70
ENDFUNC __vector_18


FUNCTION __vector_21, 9	; ADC_vect
	push r1
	push r0
	in r0, SREG
	push r0
	clr __zero_reg__
	push r18
	push r19
	push r20
	push r21
	push r24
	push r25
/* prologue: Signal */
	lds r20, ADCH
	lds r18, vscale
	lds r19, vscale+1
	mul r20, r18
	movw r24, r0
	mul r20, r19
	add r25, r0
	clr __zero_reg__
	subi r24, 0x7f
	sbci r25, 0xff
	mov r24, r25
	clr r25
	cpi r24, 0xff
	cpc r25, __zero_reg__
	LOC brlo LESS_THAN_0xFF
	ldi r24, 0xfe
	ldi r25, 0

  LOC LESS_THAN_0xFF
	lds r25, trig_low
	cp r24, r25
	LOC brsh GE_TRIG_LOW
	sts trig_status, __zero_reg__

  LOC GE_TRIG_LOW
	lds r18, t
	lds r19, t+1
	cpi r18, 0xff
	ldi r25, 0xff
	cpc r19, r25
	LOC breq T_EQ_END_OF_TRACE

.L120:
	subi r18, 0xff
	sbci r19, 0xff
.L105:
	sts t+1, r19
	sts t, r18
	lds r20, width
	lds r21, width+1
	cp r18, r20
	cpc r19, r21
	brsh .L119
	sts UDR0, r24
.L100:
/* epilogue start */
	pop r25
	pop r24
	pop r21
	pop r20
	pop r19
	pop r18
	pop r0
	out SREG, r0
	pop r0
	pop r1
	reti
.L102:
	lds r25, trig_high
	cp r24, r25
	LOC brlo GE_TRIG_LOW
	ldi r25, 1
	sts trig_status, r25
	lds r18, t
	lds r19, t+1
	cpi r18, -1
	ldi r25, -1
	cpc r19, r25
	brne .L120

  LOC T_EQ_END_OF_TRACE
	lds r18, trig_type
	lds r19, trig_type+1
	cpi r18, 1
	cpc r19, __zero_reg__
	breq .L106
	cpi r18, 2
	cpc r19, __zero_reg__
	brne .L116
	lds r25, prev_trig_status
	tst r25
	breq .L118
	lds r25, trig_status
	cpse r25, __zero_reg__
	rjmp .L112
.L116:
	ldi r18, 1
	ldi r19, 0
	rjmp .L105
.L119:
	ldi r24, 0xff
	sts UDR0, r24
	lds r24, trig_status
	sts prev_trig_status, r24
	ldi r24, lo8(.L_END_OF_TRACE)
	ldi r25, hi8(.L_END_OF_TRACE)
	sts t+1, r25
	sts t, r24
	rjmp .L100
.L106:
	lds r25, prev_trig_status
	cpse r25, __zero_reg__
	rjmp .L118
	lds r25, trig_status
	cpse r25, __zero_reg__
	rjmp .L116
.L112:
	sts prev_trig_status, r25
	rjmp .L100
.L118:
	lds r25, trig_status
	rjmp .L112
ENDFUNC __vector_21

; ------------------------------------------------------------------------------
.section	.text.startup, "ax", @progbits


FUNCTION main
	sei
	
	in r24, SMCR
	andi r24, 0xf1
	out SMCR, r24

	in r24, DDRB
	ori r24, 7
	out DDRB, r24
	
	in r24, DDRD
	ori r24, 0xf8
	out DDRD, r24
	
	in r24, PORTB
	andi r24, 0xf8
	out PORTB, r24
	
	in r24, PORTD
	andi r24, 0x0f
	out PORTD, r24
	
	sts UBRR0H, __zero_reg__
	sts UBRR0L, __zero_reg__
	ldi r24, 2
	sts UCSR0A, r24
	ldi r24, 0x98
	sts UCSR0B, r24
	ldi r24, 0x26
	sts UCSR0C, r24

	lds r24, PRR
	andi r24, 0xfe
	sts PRR, r24
	
	cbi DDRC, 0
	in r24, DDRC
	ori r24, 0x38
	out DDRC, r24

	ldi r24, 1
	sts DIDR0, r24

	in r24, PORTC
	andi r24, 0xc7
	out PORTC, r24
	
	in r24, PORTC
	lds r25, vref_raw+1
	lsl r25
	lsl r25
	lsl r25
	or r24, r25
	out PORTC, r24

	lds r24, vref_scale+2
	lds r25, vref_scale+2+1
	sts vscale+1, r25
	sts vscale, r24

	ldi r24, 0x0e
	sts ADMUX, r24
	ldi r24, 0x87
	sts ADCSRA, r24
	ldi r20, 0x20
	ldi r21, 0
	ldi r18, 0
	ldi r19, 0
.L123:
	lds r24, ADCSRA
	ori r24, 0x40
	sts ADCSRA, r24
.L122:
	lds r24, ADCSRA
	sbrc r24, 6
	rjmp .L122
	lds r24, ADCL
	lds r22, ADCH
	ldi r25, 0
	or r25, r22
	add r18, r24
	adc r19, r25
	subi r20, 1
	sbc r21, __zero_reg__
	brne .L123
	movw YL, r18
	lsr YH
	ror YL
	swap YH
	swap YL
	andi YL, 0x0f
	eor YL, YH
	andi YH, 0x0f
	eor YL, YH
	mov r24, YL
	andi r24, 0x0f
	call led_num
	movw ZL, YL
	clr __tmp_reg__
	lsr ZH
	ror ZL
	ror __tmp_reg__
	lsr ZH
	ror ZL
	ror __tmp_reg__
	mov ZH, ZL
	mov ZL, __tmp_reg__
	movw r24, ZL
	ldi r22, 19
	ldi r23, 1
	call __udivmodhi4
	sts vref_scale+1, r23
	sts vref_scale, r22
	movw r20, YL
	swap r20
	swap r21
	andi r21, 0xf0
	eor r21, r20
	andi r20, 0xf0
	eor r21, r20
	movw r18, r20
	ldi XL, -27
	ldi XH, 41
	call __umulhisi3
	sub r18, r24
	sbc r19, r25
	lsr r19
	ror r18
	add r24, r18
	adc r25, r19
	lsr r25
	ror r24
	swap r25
	swap r24
	andi r24, 0x0f
	eor r24, r25
	andi r25, 0x0f
	eor r24, r25
	sts vref_scale+2+1, r25
	sts vref_scale+2, r24
	sts vref_scale+4+1, r25
	sts vref_scale+4, r24
	movw r18, ZL
	ldi XL, -85
	ldi XH, -86
	call __umulhisi3
	movw r20, r24
	lsl r20
	mov r20, r21
	rol r20
	sbc r21, r21
	neg r21
	sts vref_scale+6+1, r21
	sts vref_scale+6, r20
	ldi r24, 0x20
	sts ADMUX, r24
	ldi r24, 5
	sts ADCSRB, r24
	in r25, 0x8
	andi r25, 0xc7
	out PORTC, r25
	in r25, PORTC
	lds r18, vref_raw+3
	lsl r18
	lsl r18
	lsl r18
	or r25, r18
	out PORTC, r25
	sts vscale+1, r21
	sts vscale, r20
	ldi r24, 7
	call set_sample_rate
	sts OCR1BH, __zero_reg__
	sts OCR1BL, __zero_reg__
.L124:
	rjmp .L124
ENDFUNC main


	.local tick
	.comm tick, 4, 1

; ------------------------------------------------------------------------------
.section .rodata

	DB led_d7654, -16, 48, -32, 112, 48, 80, -48, 112, -16, 112, -16, -112, -64, -80, -64, -64
	DB led_b210, 3, 0, 5, 5, 6, 7, 7, 0, 7, 7, 6, 7, 3, 5, 7, 6

; ------------------------------------------------------------------------------
.section .bss
	.global rcount, prev_trig_status, trig_status, trig_hist, trig_type, recv_idx
	DB rcount
	DB prev_trig_status
	DB trig_status
	DB trig_hist
	DW trig_type
	DB recv_idx
	.comm recv_buf, 4, 1
	.comm vref_scale, 8, 1

; ------------------------------------------------------------------------------
.section .data
	.global vscale, vref_raw, sample_rate_clks, width, t, trig_low, trig_high, trig_level
	DW vscale, 256
	DB vref_raw, 1, 2, 4, 7
	DW sample_rate_clks, 80, 160, 320, 800, 1600, 3200, 3705, 3115
	DW width, 800
	DW t, -1
	DB trig_low, -128
	DB trig_high, -128
	DB trig_level, -128

; ------------------------------------------------------------------------------
.ident	"GCC: (GNU) 5.4.0"
.global __do_copy_data
.global __do_clear_bss
