#*************************************************************************
#Title:    
#Authors:  Michael Petersen
#          
#File:     
#
#*************************************************************************

BASE_NAME = snd-squeal
#BOARD     = esp32:esp32:esp32s2:CDCOnBoot=cdc,UploadMode=cdc,PSRAM=enabled,PartitionScheme=huge_app
BOARD     = esp32:esp32:esp32s2:CDCOnBoot=cdc,UploadMode=cdc,PSRAM=enabled
OPTIONS   = --build-path ./build --build-property upload.maximum_size=4063232
PORT      = /dev/ttyACM0

ARDUINO   = arduino-cli

COMPILE   = $(ARDUINO) compile -b $(BOARD) $(OPTIONS) $(BASE_NAME) --warnings all

help:
	@echo "make compile.... Build $(BASE_NAME)"
	@echo "make upload..... Flash the firmware"
	@echo "make terminal... Open uart terminal"

# https://arduino.github.io/arduino-cli/0.32/commands/arduino-cli_compile/

compile:
	$(COMPILE)

compile-verbose:
	$(COMPILE) -v

upload:
	$(ARDUINO) upload -v -p $(PORT) $(BASE_NAME)

terminal:
	miniterm.py $(PORT) 115200

clean:
	rm -r ./build
