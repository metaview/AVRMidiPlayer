AVRMidiPlayer
=============

This project is based on the SID emulator for AVR http://www.roboterclub-freiburg.de/atmega_sound/atmegaSID.html 
It was enhanced to drive 5 channels and play Midi files after converting them with MidiParser into a C header file.
At the moment an ATMega8 powered by 14.3 MHz is used. Additionally a blue LED on pin C2 is fading every other second.
The PWM code for LED fading was also gathered somewhere on the net.
The SID emulator supports some of the SID registers, therefore http://www.dopeconnection.net/C64_SID.htm is an interesting resource.
