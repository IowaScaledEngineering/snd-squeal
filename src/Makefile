#*************************************************************************
#Title:    
#Authors:  Michael Petersen
#          
#File:     
#
#*************************************************************************

BASE_NAME = snd-squeal
BOARD     = esp32:esp32:esp32s2:CDCOnBoot=cdc,UploadMode=cdc,PSRAM=enabled,PartitionScheme=huge_app
PORT      = /dev/ttyACM0

ARDUINO   = arduino-cli

help:
	@echo "make compile.... Build $(BASE_NAME)"
	@echo "make upload..... Flash the firmware"
	@echo "make terminal... Open uart terminal"

# https://arduino.github.io/arduino-cli/0.32/commands/arduino-cli_compile/

compile:
	$(ARDUINO) compile -b $(BOARD) $(BASE_NAME) --warnings all

compile-verbose:
	$(ARDUINO) compile -v -b $(BOARD) $(BASE_NAME) --warnings all

upload:
	$(ARDUINO) upload -b $(BOARD) $(BASE_NAME)

terminal:
	miniterm.py $(PORT) 115200
