MEGA2560 / Xavier Hosxe note : 
===================
My goal for this fork is to add the following feature that i need for my pincab :
. A working MPU6050 driver (Accelerometer)
. Possibility to turn down power after a delay (to avoid toys to burn).
  This is possible on 4 pins (6,7,8) using real PWM for high frequency (30Khz). (Software PWM made lots of noise with the soleoid).
. Builtin led indicator (MPU6050 problem + health blink)
. Add a silence input pin to disable noisy toys. Toogle : presse once for silence and agin to noise back (KEY_MUTE_TOYS)

LWCloneU2
=========

A firmware for Atmel AVR microcontroller for controlling LEDs or Light Bulbs via USB *and* a Joystick/Mouse/Keyboard Encoder.

The device is compatible with the LED-WIZ controller on the USB protocol level and thus can be used with many existing software.
Additionally the firmware allows to add panel support, i.e. up to 4 yoysticks, 1 mouse, 1 keyboard and more. That is with one board you can get an input encoder and an LED output controller perfectly suited for MAME.

The LWCloneU2 project contains a compatible driver DLL "ledwiz.dll" replacement that fixes some bugs of the original one and does not block your main application, i.e. the I/O is fully asynchron.


Supported Hardware
==================
- Custom Breakout Board with ATMega32U2
- Arduino Leonardo (ATMega32U4)
- Arduino Mega 2560 (tested with Rev. 3)
- Arduino Uno Rev. 2/3 (untested)


Building the firmware
=====================

In order to build all this, you need a recent toolchain for AVR microcontroller, e.g. the 'AVR Toolchain 3.4.2-1573' from Atmel or the one that is bundled with the Atmel AVRStudio.
Get the sources from the Git repository, then do a 'git submodule update --init' in order to get the required LUFA (USB framework) sources. Then a 'make' should build the firmwares for all supported platforms.


Building the Windows DLL
========================

There are project files for Visualstudio 2008 Express and Visualstudio 2012 Express. The VS 2012 solution supports creating a 64 bit DLL.
