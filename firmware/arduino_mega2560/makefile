
# DIRS =  m2560 m16u2
DIRS =  m2560 
# DIRS =  m16u2

I2C_DIR = ../i2cmaster
I2C_MAKEFILE = makefile.twimaster


all:
	for i in $(DIRS); do make --directory=$$i clean; make --directory=$$i all; done

clean:
	for i in $(DIRS); do make --directory=$$i clean; done

i2c:
	make -C $(I2C_DIR) -f $(I2C_MAKEFILE) clean
	make -C $(I2C_DIR) -f $(I2C_MAKEFILE) 
