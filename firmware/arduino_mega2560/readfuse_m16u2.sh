#!/bin/bash

 avrdude -c usbasp -p m16u2  -b 19200 -U lfuse:r:-:i -v
 