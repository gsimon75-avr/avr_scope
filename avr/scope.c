#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <inttypes.h>
#include <util/delay.h>
#include <stdio.h>
#include <math.h>

void
led_num(uint8_t n) {
    static const uint8_t led_b210[0x10] = {3,0,5,5, 6,7,7,0, 7,7,6,7, 3,5,7,6};
    static const uint8_t led_d7654[0x10] =
{0xf0,0x30,0xe0,0x70, 0x30,0x50,0xd0,0x70, 0xf0,0x70,0xf0,0x90, 0xc0,0xb0,0xc0,0xc0};

    PORTB &= 0xf8;
    PORTD &= 0x0f;
    if (n < 0x10) {
        PORTB |= led_b210[n];
        PORTD |= led_d7654[n];
    }
}

void
led_onoff(uint8_t onoff) {
    if (onoff)
        PORTD |= 0x08;
    else
        PORTD &= 0xf7;
}


void
count(void)
{
    static volatile uint32_t tick = 0;
    tick++;
    led_num((tick >> 8) & 0x0f);
    //led_num((tick >> 0) & 0x0f);
}

ISR(TIMER1_COMPB_vect) {                                                                                                       
    //count();
}

#define BAUDVALUE(n) (((F_CPU+(4*(n))) / (8*(n))) - 1)

typedef enum {
    TRIG_NONE,
    TRIG_RISING,
    TRIG_FALLING,
} trig_type_t;

#define RECV_BUF_SIZE_BITS 2
#define RECV_BUF_SIZE (1 << RECV_BUF_SIZE_BITS)

#define END_OF_TRACE 0xffff

uint8_t recv_buf[RECV_BUF_SIZE];
int8_t recv_idx = 0; // points to the next writable slot

trig_type_t trig_type = TRIG_NONE;
uint8_t trig_level = 0x80;
uint8_t trig_hist = 0;
uint8_t trig_high = 0x80;
uint8_t trig_low = 0x80;
uint16_t t = END_OF_TRACE, width = 800;

// NOTE: trig_status goes 0 when value < trig_level - trig_hist,
// and goes to 1 when value >= trig_level + trig_hist
uint8_t trig_status = 0;
uint8_t prev_trig_status = 0;

void
calculate_trig_high_low(void) {
    trig_high = (trig_level < (0xff - trig_hist)) ? (trig_level + trig_hist) : 0xff;
    trig_low = (trig_level > trig_hist) ? (trig_level - trig_hist) : 1;
}

#define SAMPLE_RATE_FOR_US(x)   ((x) * F_CPU/1000000UL)
uint16_t sample_rate_clks[8] = {
    SAMPLE_RATE_FOR_US(   5),
    SAMPLE_RATE_FOR_US(  10),
    SAMPLE_RATE_FOR_US(  20),
    SAMPLE_RATE_FOR_US(  50),
    SAMPLE_RATE_FOR_US( 100),
    SAMPLE_RATE_FOR_US( 200),
    SAMPLE_RATE_FOR_US( 500),
    SAMPLE_RATE_FOR_US(1000),
};

#define PRESCALER_FOR_US(x) ((SAMPLE_RATE_FOR_US(x)+6)/13)
uint16_t prescalers[8] = {
    PRESCALER_FOR_US(   5),
    PRESCALER_FOR_US(  10),
    PRESCALER_FOR_US(  20),
    PRESCALER_FOR_US(  50),
    PRESCALER_FOR_US( 100),
    PRESCALER_FOR_US( 200),
    PRESCALER_FOR_US( 500),
    PRESCALER_FOR_US(1000),
};

uint8_t vref_raw[4] = { // vref 3-bit ladder dac value for a given unit voltage
    1, // 2mV
    2, // 5mV
    4, // 10mV
    7, // 20mV
};
uint16_t vref_scale[4];
uint16_t vscale = 0x100; // scale factor for ADC data

void
set_sample_rate(uint8_t idx) {
    uint8_t ps = 2; // adc prescaler bits

    // setup timer1
    TCCR1B = _BV(WGM12) | 1; // ctc mode up to OCR1A, clk = sysclk / 1
    OCR1A = sample_rate_clks[idx];

    // set up adc prescaler
    uint16_t n = prescalers[idx];
    for (ps = 0; (ps < 8) && n; ps++)
        n >>= 1;
    // decrement ps and clamp to 2 if less
    if (ps <= 3)
        ps = 2;
    else
        ps--;
    
    ADCSRA = _BV(ADEN) | _BV(ADATE) | _BV(ADIE) | ps; // ena, irq ena, prescaler=x
    //led_num(ps);

    TCNT1 = 0; // timer reset
    TIMSK1 = _BV(OCIE1B); // NOTE adc triggers *not* on TCNT1==OCR1B but on this irq
}


void select_vref(uint8_t n) {
    PORTC &= ~0x38;
    //led_num(vref_raw[n]);
    PORTC |= vref_raw[n] << 3;
    vscale = vref_scale[n];
}

void recv_push(uint8_t x) {
    recv_buf[recv_idx++] = x;
    if (recv_idx >= RECV_BUF_SIZE)
        recv_idx = 0;
}

uint8_t recvd_pop(void) {
    if (--recv_idx < 0)
        recv_idx = RECV_BUF_SIZE - 1;
    return recv_buf[recv_idx];
}

volatile uint8_t rcount = 0;
ISR(USART_RX_vect) {
    //led_num(rcount++ & 0x0f);
    uint8_t recvd = UDR0;
    if (('0' <= recvd) && (recvd <= '9')) 
        recv_push(recvd - '0');
    else if (('a' <= recvd) && (recvd <= 'f')) 
        recv_push(recvd - 'a' + 10);
    else if (('A' <= recvd) && (recvd <= 'F')) 
        recv_push(recvd - 'A' + 10);
    else switch (recvd) {
        case 'S': // 'Speed' - sample delay: 0=5us, 1=10us, 2=20us, 3=50us, 4=100us, 5=200us, 6=500us, 7=1ms
            set_sample_rate(recvd_pop() & 0x07);
            t = END_OF_TRACE;
            break;

        case 'W': // 'Width'
            width = 32 * (uint16_t)(recvd_pop() + (recvd_pop() << 4));
            t = END_OF_TRACE;
            break;

        case 'L': // 'Trig level'
            trig_level = recvd_pop() + (recvd_pop() << 4);
            calculate_trig_high_low();
            break;

        case 'T': // 'Trig type'
            trig_type = recvd_pop();
            calculate_trig_high_low();
            break;

        case 'H': // 'Hysteresis'
            trig_hist = recvd_pop();
            calculate_trig_high_low();
            break;

        case 'R': // 'Reference' voltage
            // 0=2mV, 1=5mV, 2=10mV, 3=20mV
            select_vref(recvd_pop() & 0x03);
            break;
    }
}

ISR(ADC_vect) {
    uint16_t v16 = (vscale * ADCH + 0x7f) >> 8;
    uint8_t value = (v16 > 0xfe) ? 0xfe : v16;

    if (value < trig_low)
        trig_status = 0;
    else if (value >= trig_high)
        trig_status = 1;

    if (t == END_OF_TRACE) { // wait for trig condition
        switch (trig_type) {
            case TRIG_RISING:
                if (prev_trig_status || !trig_status) {
                    prev_trig_status = trig_status;
                    return;
                }
                break;

            case TRIG_FALLING:
                if (!prev_trig_status || trig_status) {
                    prev_trig_status = trig_status;
                    return;
                }
                break;

            case TRIG_NONE: // no trig, go on
                break;
        }
        t = 0;
    }

    if (++t >= width) {
        UDR0 = 0xff;
        prev_trig_status = trig_status;
        t = END_OF_TRACE;
    }
    else {
        UDR0 = value;
    }

}

int
main(void) {
    int i;
    sei();
    set_sleep_mode(SLEEP_MODE_IDLE);

    /****************************************************************
     * LED output setup
     */
    DDRB |= 0x07; // B{0,1,2} are output
    DDRD |= 0xf8; // D{3,4,5,6,7} are output
    led_num(0x80); // off

    /****************************************************************
     * UART setup for 115200/8/E/1
     */
    UBRR0 = BAUDVALUE(2000000UL); // NOTE: ensure that UART is faster than ADC
    UCSR0A = _BV(U2X0); // double speed
    UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00) | _BV(UPM01); // 8bits, even pty, 1 stop

    /****************************************************************
     * ADC setup for measuring Vcc
     */
    PRR &= ~_BV(PRADC); // adc power reduce off
    DDRC &= ~0x01; // C0 is input
    DDRC |= 0x38; // C{3,4,5} are output
    DIDR0 = _BV(ADC0D);

    /*while (1) {
        uint8_t i;
        for (i = 0; i < 8; i++) {
            PORTC = i << 3;
            led_num(i);
            _delay_ms(5000);
        }
    }*/


    // measure Vcc: choose vref dac=2 (=Vcc/4 >= 1.1V), vref=aref, input=bandgap, prescaler=1:128
    //select_vref(0); // vref for setting#1 = Vcc * 1/8 =~ 0.625V 
    select_vref(1); // vref for setting#1 = Vcc * 2/8 =~ 1.25V
    //select_vref(2); // vref for setting#1 = Vcc * 4/8 =~ 2.5V
    //select_vref(3); // vref for setting#1 = Vcc * 7/8 =~ 4.375V
    ADMUX = 0x0e;
    ADCSRA = _BV(ADEN) | 0x07;
    uint16_t v11 = 0;
    for (i = 0; i < 0x20; i++) { // max 0x40 repetitions
        ADCSRA |= _BV(ADSC);
        while (ADCSRA & _BV(ADSC))
            ;
        uint16_t v = ADCL;
        v |= ((uint16_t)ADCH) << 8;
        v11 += v;
    }
    v11 >>= 5;
    {
        // when vref=Vcc*2/8, 1.1V=1100mV reads as `v11`/4 (we're now on 10bits, will be on 8bits)
        led_num((v11 >> 0) & 0x0f);

        // setting #0: unit= 2mV vref=Vcc*1/8 multiplier=(v11/4 *  2 / 1100) * 2/1 * 256=v11*1024/1100
        // setting #1: unit= 5mV vref=Vcc*2/8 multiplier=(v11/4 *  5 / 1100) * 2/2 * 256=v11*1280/1100
        // setting #2: unit=10mV vref=Vcc*4/8 multiplier=(v11/4 * 10 / 1100) * 2/4 * 256=v11*1280/1100
        // setting #3: unit=20mV vref=Vcc*7/8 multiplier=(v11/4 * 20 / 1100) * 2/7 * 256=v11*10240/7700
        vref_scale[0] = v11 * 64 / 275;
        vref_scale[1] = v11 * 16 / 55;
        vref_scale[2] = v11 * 16 / 55;
        vref_scale[3] = v11 * 64 / 192; // should be * 128 / 385, but we have only 16-10=6 bits for the multiplication
    }

    /****************************************************************
     * ADC setup for normal use
     */
    ADMUX = _BV(ADLAR); // vref=AREF, lar=1, chan=0
    ADCSRB = 5; // start conversion when TCNT1 == OCR1B
    select_vref(3);
    set_sample_rate(7);
    OCR1B = 0;


    /****************************************************************
     * Main loop
     */
    while (1)
        ;

    return 0;
}
