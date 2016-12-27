# USBemani

The USBemani is a control board intended to replace the original control board that was part of Konami's Arcade-Style Controller (ASC) for the game **beatmania IIDX**. The USBemani was created to provide a means to use the ASC with software simulators, such as **Lunatic Rave 2**, among others; and provides some of the following improvements:

* Lower-latency turntable input, allowing for a simulation of an analog turntable,
* Absolute-position tracking for the turntable, in simulators that allow such a feature,
* Lower-latency input compared to the use of a USB adapter, and
* A means to continue to use the controller with the PS2 with the lower-latency turntable, mimicing an arcade-style experience on the console.

Additionally, I've also included code for the original HID bootloader, as well as software used to configure the USBemani; the configuration software was written with compilation using MinGW in mind, and I have not tested whether or not this still compiles.

Due to current circumstances and overall busy-ness with work, I am no longer able to work on this code. As such, this code is a very rough pull, with some minor cleanup. It still compiles with avr-gcc, however. I've included the last LUFA library that this code has been compiled against. Over the years I've become a better C developer, and some day I may revisit this project in order to optimize the code and implement some better practices. For now, this code is offered in an 'at your own risk' fashion; I cannot assist with any issue you may experience with using this code.


