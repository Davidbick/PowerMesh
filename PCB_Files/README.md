## Component List

- ST8500
- STLD1
- NUCLEO-F446RE
- STM32F446RET6

-SWD Needs: 

 Bare minimum: GND, SWCLK, SWDIO

The other signals:

    VDDD: Some programmers want your VDDD so they can match their IO signal levels to the ones your MCU uses and if they don't detect any voltage on VDDD they won't work.

    NRST: If, for some reason, the programmer can't do a software-reset (like, for example, you're using the SWDIO/SWCLK pins as IO in your program), you have to connect via hard reset.

    SWO: If you select "Trace Asynchronous Sw" as your debug interface, this pin will be used for debug-printfs.

You solder a 1.27mm pitch 2x5 male header (like the Samtec FTSH-105-01-L-DV-K).
