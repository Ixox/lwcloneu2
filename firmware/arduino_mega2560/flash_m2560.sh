#!/bin/bash

avrdude -c usbasp -p atmega2560 -b 115200 -v -U flash:w:"./m2560/arduino_mega2560__m2560.hex:i"
