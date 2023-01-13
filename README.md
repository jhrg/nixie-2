
Nixie clock code

10/1/22

The current version of this clock software is 0.1.0

A Nixie tube clock using Z573M tubes and a K155N?1 BCD to decimal driver.

This will use multiplexing to drive four Z573M tubes using one K155N?1 chip.

| Timer output | Arduino output	| Chip pin | Pin name |
|--------------|----------------|----------|----------|
|    OC0A	   |        6	    |   12	   |    PD6   |
|    OC0B	   |        5	    |   11	   |    PD5   |
|    OC1A	   |        9	    |   15	   |    PB1   |
|    OC1B	   |        10	    |   16	   |    PB2   |
|    OC2A	   |        11	    |   17	   |    PB3   |
|    OC2B	   |        3	    |   5	   |    PD3   |

*https://docs.arduino.cc/tutorials/generic/secrets-of-arduino-pwm

