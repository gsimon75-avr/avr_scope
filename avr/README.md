## Input

ADC0


## Serial format

2 mbps, 8 bits, even pty, 1 stop


## Protocol

PC to AVR: stack-like commands, args first as hex digits, then 1-letter commands, unknown chars are ignored

<n>S: adc prescaler, 7..1, 1 is too fast
<nn>W: scan width = 0x20 * nn
<nn>T: rising trig level nn
<nn>t: falling trig level nn
<n>H: trig hysteresis n

AVR to PC: byte stream of <width> scans, 0..0xfe
the last byte is always 0xff to provide sync info


## 7-segment wiring:
A - D6
B - D5
C - D4
D - B0
E - D7
F - B1
G - B2
P - D3 -> D2

                  DDDD BBB
   A B C D E F G  7654 210 
0  1 1 1 1 1 1 0  1111 011 
1  0 1 1 0 0 0 0  0011 000 
2  1 1 0 1 1 0 1  1110 101 
3  1 1 1 1 0 0 1  0111 101 
4  0 1 1 0 0 1 1  0011 110 
5  1 0 1 1 0 1 1  0101 111 
6  1 0 1 1 1 1 1  1101 111 
7  1 1 1 0 0 0 0  0111 000 
8  1 1 1 1 1 1 1  1111 111 
9  1 1 1 1 0 1 1  0111 111  
a  1 1 1 0 1 1 1  1111 110 
b  0 0 1 1 1 1 1  1001 111 
c  1 0 0 1 1 1 0  1100 011 
d  0 1 1 1 1 0 1  1011 101 
e  1 0 0 1 1 1 1  1100 111 
f  1 0 0 0 1 1 1  1100 110 

## DC sampling

C0 is connected to the input through a 22k resistor. The input has a 100M impedance, so that 22k
won't affect it significantly, but to reach the 40mA absolute maximum value the input voltage
shall exceed 880V - not impossible but highly unlikely.

FIXME: a fast reverse-directed diode would be a good idea to clamp the negative inputs as well.

## AC sampling

A 2 x 47k divisor is connected between VRef and GND, producing a midpoint. C1 is connected to
this midpoint via a 1M resistor, and to the input via a 10uF capacitor.

This way the AC component of the input is readable on ADC1, centered in the current voltage range
