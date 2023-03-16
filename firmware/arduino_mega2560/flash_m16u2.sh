#!/bin/bash

avrdude -c usbasp -p m16u2 -b 115200 -v -U flash:w:"./ m16u2/arduino_mega2560__m16u2.hex:i"
