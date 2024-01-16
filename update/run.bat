echo off
rem copy build-x.y.z directory here, then zip all the files for distribution
set count=0
:loop
.\esptool\esptool.exe --chip esp32s2 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x1000 "./build-1.0.0/snd-squeal.ino.bootloader.bin" 0x8000 "./build-1.0.0/snd-squeal.ino.partitions.bin" 0xe000 "./build-1.0.0/boot_app0.bin" 0x10000 "./build-1.0.0/snd-squeal.ino.bin"
rem .\esptool\esptool.exe --chip esp32s2 read_mac
if %errorlevel% GTR 0 (
	echo *************************************************
	echo  Failed, retrying...
	echo *************************************************

	set /A count=count+1
	if %count% LSS 2 (
		goto :loop
	)
) else (
	echo *************************************************
	echo  Success!
	echo *************************************************
)
%COMSPEC%
