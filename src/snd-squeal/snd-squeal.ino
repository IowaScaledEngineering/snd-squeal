/*************************************************************************
Title:    Squealer
Authors:  Michael Petersen <railfan@drgw.net>
File:     snd-squeal.ino
License:  GNU General Public License v3

LICENSE:
    Copyright (C) 2024 Michael Petersen

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

*************************************************************************/

#include <SPI.h>
#include <SD.h>
#include "driver/i2s.h"
#include <Preferences.h>
#include <vector>
#include <strings.h>
#include "esp_task_wdt.h"

// 10 sec WDT
#define WDT_TIMEOUT 10

#include "sound.h"

#include "squeal/flangeClip01.h"
#include "squeal/flangeClip02.h"
#include "squeal/flangeClip03.h"
#include "squeal/flangeClip04.h"
#include "squeal/flangeClip05.h"
#include "squeal/flangeClip06.h"
#include "squeal/flangeClip07.h"
#include "squeal/flangeClip08.h"
#include "squeal/flangeClip09.h"
#include "squeal/flangeClip10.h"
#include "squeal/flangeClip11.h"
#include "squeal/flangeClip12.h"
#include "squeal/flangeClip13.h"
#include "squeal/flangeClip14.h"
#include "squeal/flangeClip15.h"
#include "squeal/flangeClip16.h"
//#include "squeal/flangeClip17.h"
//#include "squeal/flangeClip18.h"

// Samples for each buffer
#define AUDIO_BUFFER_SIZE 512
#define AUDIO_BUFFER_NUM  4

// Bytes
#define FILE_BUFFER_SIZE 2048

// Pins
#define EN1       9
#define EN2       10
#define EN3       3
#define EN4       21
#define LEDA      11
#define LEDB      12
#define VOLDN     13
#define VOLUP     14
#define AUX1      15
#define AUX2      16
#define AUX3      17
#define AUX4      18
#define AUX5      8
#define I2S_SD    4
#define I2S_DATA  5
#define I2S_BCLK  6
#define I2S_LRCLK 7
#define SDCLK     36
#define SDMOSI    35
#define SDMISO    37
#define SDCS      34
#define SDDET     33

// Bit positions for inputs
#define VOL_UP_BUTTON 0x01
#define VOL_DN_BUTTON 0x02
#define EN1_INPUT     0x10
#define EN2_INPUT     0x20
#define EN3_INPUT     0x40
#define EN4_INPUT     0x80

// Volume
#define VOL_STEP_MAX  30
#define VOL_STEP_NOM  20

uint8_t volumeStep = 0;
uint16_t volume = 0;
uint8_t volumeUpCoef = 0;
uint8_t volumeDownCoef = 0;

uint16_t volumeLevels[] = {
  0,      // 0
  100,
  200,
  300,
  400,
  500,
  600,
  700,
  800,
  900,
  1000,   // 10
  1900,
  2800,
  3700,
  4600,
  5500,
  6400,
  7300,
  8200,
  9100,
  10000,  // 20
  11000,
  12000,
  13000,
  14000,
  15000,
  16000,
  17000,
  18000,
  19000,
  20000,  // 30
};

bool restart = false;

uint8_t enable = 0;              // 0 or 1
uint8_t silenceDecisecsMax = 0;
uint8_t silenceDecisecsMin = 0;

Preferences preferences;

uint8_t debounce(uint8_t debouncedState, uint8_t newInputs)
{
  static uint8_t clock_A = 0, clock_B = 0;
  uint8_t delta = newInputs ^ debouncedState;   //Find all of the changes
  uint8_t changes;

  clock_A ^= clock_B;                     //Increment the counters
  clock_B  = ~clock_B;

  clock_A &= delta;                       //Reset the counters if no changes
  clock_B &= delta;                       //were detected.

  changes = ~((~delta) | clock_A | clock_B);
  debouncedState ^= changes;
  return(debouncedState);
}

char* rtrim(char* in)
{
  char* endPtr = in + strlen(in) - 1;
  while (endPtr >= in && isspace(*endPtr))
    *endPtr-- = 0;

  return in;
}

char* ltrim(char* in)
{
  char* startPtr = in;
  uint32_t bytesToMove = strlen(in);
  while(isspace(*startPtr))
    startPtr++;
  bytesToMove -= (startPtr - in);
  memmove(in, startPtr, bytesToMove);
  in[bytesToMove] = 0;
  return in;
}

bool configKeyValueSplit(char* key, uint32_t keySz, char* value, uint32_t valueSz, const char* configLine)
{
  char lineBuffer[256];
  char* separatorPtr = NULL;
  char* lineBufferPtr = NULL;
  uint32_t bytesToCopy;

  separatorPtr = strchr(configLine, '=');
  if (NULL == separatorPtr)
    return false;

  memset(key, 0, keySz);
  memset(value, 0, valueSz);

  // Copy the part that's eligible to be a key into the line buffer
  bytesToCopy = separatorPtr - configLine;
  if (bytesToCopy > sizeof(lineBuffer)-1)
    bytesToCopy = sizeof(lineBuffer);
  memset(lineBuffer, 0, sizeof(lineBuffer));
  strncpy(lineBuffer, configLine, bytesToCopy);

  lineBufferPtr = ltrim(rtrim(lineBuffer));
  if (0 == strlen(lineBufferPtr) || '#' == lineBufferPtr[0])
    return false;

  strncpy(key, lineBufferPtr, keySz);

//  bytesToCopy = strlen(separatorPtr+1);
//  if (bytesToCopy > sizeof(lineBuffer)-1)
//    bytesToCopy = sizeof(lineBuffer);
  memset(lineBuffer, 0, sizeof(lineBuffer));
  // Changed to sizeof(lineBuffer)-1 below instead of bytesToCopy due to -Werror=stringop-overflow and -Werror=stringop-truncation
  strncpy(lineBuffer, separatorPtr+1, sizeof(lineBuffer)-1);
  lineBufferPtr = ltrim(rtrim(lineBuffer));
  if (0 == strlen(lineBufferPtr))
  {
    memset(key, 0, keySz);
    return false;
  }
  strncpy(value, lineBufferPtr, valueSz);
  return true;
}

hw_timer_t * timer = NULL;

void IRAM_ATTR processVolume(void)
{
  static uint8_t buttonsPressed = 0, oldButtonsPressed = 0;
  static unsigned long pressTime = 0;
  uint8_t inputStatus = 0;

//  digitalWrite(AUX5, 1);

  // Turn off LED
  uint16_t ledHoldTime = (VOL_STEP_NOM == volumeStep) ? 1000 : 100;
  if((millis() - pressTime) > ledHoldTime)
  {
    digitalWrite(LEDB, 0);
  }

  // Read inputs
  if(digitalRead(VOLUP))
    inputStatus &= ~VOL_UP_BUTTON;
  else
    inputStatus |= VOL_UP_BUTTON;

  if(digitalRead(VOLDN))
    inputStatus &= ~VOL_DN_BUTTON;
  else
    inputStatus |= VOL_DN_BUTTON;

  if(digitalRead(EN1))
    inputStatus &= ~EN1_INPUT;
  else
    inputStatus |= EN1_INPUT;

  if(digitalRead(EN2))
    inputStatus &= ~EN2_INPUT;
  else
    inputStatus |= EN2_INPUT;

  if(digitalRead(EN3))
    inputStatus &= ~EN3_INPUT;
  else
    inputStatus |= EN3_INPUT;

  if(digitalRead(EN4))
    inputStatus &= ~EN4_INPUT;
  else
    inputStatus |= EN4_INPUT;

  // Debounce
  buttonsPressed = debounce(buttonsPressed, inputStatus);

  // Find rising edge of volume up button
  if((buttonsPressed ^ oldButtonsPressed) & (buttonsPressed & VOL_UP_BUTTON))
  {
    pressTime = millis();
    if(volumeStep < VOL_STEP_MAX)
    {
      volumeStep++;
      preferences.putUChar("volume", volumeStep);
    }
    Serial.print("Vol Up: ");
    Serial.println(volumeStep);
    digitalWrite(LEDB, 1);
  }

  // Find rising edge of volume down button
  if((buttonsPressed ^ oldButtonsPressed) & (buttonsPressed & VOL_DN_BUTTON))
  {
    pressTime = millis();
    if(volumeStep > 0)
    {
      volumeStep--;
      preferences.putUChar("volume", volumeStep);
    }
    Serial.print("Vol Dn: ");
    Serial.println(volumeStep);
    digitalWrite(LEDB, 1);
  }

  // Check for serial input
  if(Serial.available() > 0)
  {
    uint8_t serialChar = Serial.read();
    switch(serialChar)
    {
      case 'a':
        if(silenceDecisecsMax < 255)
        {
          silenceDecisecsMax++;
          preferences.putUChar("silenceMax", silenceDecisecsMax);
        }
        Serial.print("Silence Max: ");
        Serial.print(silenceDecisecsMax/10.0, 1);
        Serial.println("s");
        break;
      case 'z':
        if(silenceDecisecsMax > silenceDecisecsMin)
        {
          silenceDecisecsMax--;
          preferences.putUChar("silenceMax", silenceDecisecsMax);
        }
        Serial.print("Silence Max: ");
        Serial.print(silenceDecisecsMax/10.0, 1);
        Serial.println("s");
        break;

      case 's':
        if(silenceDecisecsMin < silenceDecisecsMax)
        {
          silenceDecisecsMin++;
          preferences.putUChar("silenceMin", silenceDecisecsMin);
        }
        Serial.print("Silence Min: ");
        Serial.print(silenceDecisecsMin/10.0, 1);
        Serial.println("s");
        break;
      case 'x':
        if(silenceDecisecsMin > 0)
        {
          silenceDecisecsMin--;
          preferences.putUChar("silenceMin", silenceDecisecsMin);
        }
        Serial.print("Silence Min: ");
        Serial.print(silenceDecisecsMin/10.0, 1);
        Serial.println("s");
        break;

      case 'd':
        if(volumeUpCoef < 255)
        {
          volumeUpCoef++;
          preferences.putUChar("volumeUp", volumeUpCoef);
        }
        Serial.print("Volume Up Coef: ");
        Serial.println(volumeUpCoef);
        break;
      case 'c':
        if(volumeUpCoef > 1)
        {
          volumeUpCoef--;
          preferences.putUChar("volumeUp", volumeUpCoef);
        }
        Serial.print("Volume Up Coef: ");
        Serial.println(volumeUpCoef);
        break;

      case 'f':
        if(volumeDownCoef < 255)
        {
          volumeDownCoef++;
          preferences.putUChar("volumeDown", volumeDownCoef);
        }
        Serial.print("Volume Down Coef: ");
        Serial.println(volumeDownCoef);
        break;
      case 'v':
        if(volumeDownCoef > 1)
        {
          volumeDownCoef--;
          preferences.putUChar("volumeDown", volumeDownCoef);
        }
        Serial.print("Volume Down Coef: ");
        Serial.println(volumeDownCoef);
        break;

      case 'q':
        restart = true;
        break;
    }
  }

  enable = (buttonsPressed & (EN1_INPUT | EN2_INPUT | EN3_INPUT | EN4_INPUT)) ? 1 : 0;

  if(enable)
  {
    digitalWrite(LEDA, 1);
  }
  else
  {
    digitalWrite(LEDA, 0);
  }

  // Process volume
  uint16_t deltaVolume;
  uint16_t volumeTarget;
  volumeTarget = volumeLevels[volumeStep] * enable;

  if(volume < volumeTarget)
  {
    deltaVolume = (volumeTarget - volume);
    if((deltaVolume > 0) && (deltaVolume < volumeUpCoef))
      deltaVolume = volumeUpCoef;  // Make sure it goes all the way to min or max
    volume += deltaVolume  / volumeUpCoef;
//    Serial.println(volume);
  }
  else if(volume > volumeTarget)
  {
    deltaVolume = (volume - volumeTarget);
    if((deltaVolume > 0) && (deltaVolume < volumeDownCoef))
      deltaVolume = volumeDownCoef;  // Make sure it goes all the way to min or max
    volume -= deltaVolume / volumeDownCoef;
//    Serial.println(volume);
  }

  oldButtonsPressed = buttonsPressed;
//  digitalWrite(AUX5, 0);
}

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200);

  pinMode(VOLDN, INPUT_PULLUP);
  pinMode(VOLUP, INPUT_PULLUP);
  pinMode(I2S_SD, OUTPUT);
  digitalWrite(I2S_SD, 0);  // Disable amplifier

  pinMode(LEDA, OUTPUT);
  pinMode(LEDB, OUTPUT);
  digitalWrite(LEDA, 0);
  digitalWrite(LEDB, 0);

  pinMode(EN1, INPUT_PULLUP);
  pinMode(EN2, INPUT_PULLUP);
  pinMode(EN3, INPUT_PULLUP);
  pinMode(EN4, INPUT_PULLUP);

  pinMode(AUX1, OUTPUT);
  pinMode(AUX2, OUTPUT);
  pinMode(AUX3, OUTPUT);
  pinMode(AUX4, OUTPUT);
  pinMode(AUX5, OUTPUT);

  esp_task_wdt_init(WDT_TIMEOUT, true);  // 1s timeout
  esp_task_wdt_add(NULL); //add current thread to WDT watch
  esp_task_wdt_reset();

  timer = timerBegin(0, 80, true);  // Timer 0, 80x prescaler = 1us
  timerAttachInterrupt(timer, &processVolume, false);  // level triggered
  timerAlarmWrite(timer, 10000, true);  // 80MHz / 80 / 10000 = 10ms, autoreload
}

void play(Sound *wavSound)
{
  size_t i;
  size_t bytesRead;
  uint8_t fileBuffer[FILE_BUFFER_SIZE];
  int16_t sampleValue;
  uint32_t outputValue;
  size_t bytesWritten;

  esp_task_wdt_reset();

  wavSound->open();
  
  i2s_port_t i2s_num = I2S_NUM_0;
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = wavSound->getSampleRate(),
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = AUDIO_BUFFER_NUM,
      .dma_buf_len = AUDIO_BUFFER_SIZE,
      .use_apll = 0,
      .tx_desc_auto_clear = true,
      .fixed_mclk = -1,
      .mclk_multiple = I2S_MCLK_MULTIPLE_DEFAULT,
      .bits_per_chan = I2S_BITS_PER_CHAN_DEFAULT,
  };

  i2s_pin_config_t pin_config = {
      .mck_io_num = I2S_PIN_NO_CHANGE,
      .bck_io_num = I2S_BCLK,
      .ws_io_num = I2S_LRCLK,
      .data_out_num = I2S_DATA,
      .data_in_num = I2S_PIN_NO_CHANGE,
  };

  i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
  i2s_set_pin(i2s_num, &pin_config);

  digitalWrite(I2S_SD, 1);  // Enable amplifier
//  digitalWrite(AUX4, 1);

  while(wavSound->available())
  {
    esp_task_wdt_reset();
//    digitalWrite(AUX1, 1);
    bytesRead = wavSound->read(fileBuffer, (size_t)FILE_BUFFER_SIZE);
//    digitalWrite(AUX1, 0);
//    digitalWrite(AUX2, 1);
    // Audio buffer samples are in 16-bit chunks, so step by two
    for(i=0; i<bytesRead; i+=2)
    {
//      digitalWrite(AUX3, 1);
      esp_task_wdt_reset();
      // File is read on a byte basis, so convert into int16 samples, and step every 2 bytes
      sampleValue = *((int16_t *)(fileBuffer+i));
      int32_t adjustedValue = sampleValue * volume / volumeLevels[VOL_STEP_NOM];
      if(adjustedValue > 32767)
        sampleValue = 32767;
      else if(adjustedValue < -32768)
        sampleValue = -32768;
      else
        sampleValue = adjustedValue;
      // Combine into 32 bit word (left & right)
      outputValue = (sampleValue<<16) | (sampleValue & 0xffff);
      i2s_write(i2s_num, &outputValue, 4, &bytesWritten, portMAX_DELAY);
//      digitalWrite(AUX3, 0);
    }
//    digitalWrite(AUX2, 0);
    if(restart)
      break;  // Stop playing and return to the main loop
  }

  // Fill the buffer with zeros.
  for(i=0; i<(AUDIO_BUFFER_NUM*AUDIO_BUFFER_SIZE); i++)
  {
      // Fill all buffers with zeros
//      digitalWrite(AUX5, 1);
      esp_task_wdt_reset();
      outputValue = 0;
      i2s_write(i2s_num, &outputValue, 4, &bytesWritten, portMAX_DELAY);
//      digitalWrite(AUX5, 0);
  }
  // By the time we're done, it must be playing "zero", so it's safe to disable the amplifier
  digitalWrite(I2S_SD, 0);  // Disable amplifier
//  digitalWrite(AUX4, 0);
  i2s_driver_uninstall(i2s_num);
  wavSound->close();
}

void loop()
{
  bool usingSdSounds = false;
  size_t fileNameLength;
  File rootDir;
  File wavFile;
  const char *fileName;
  uint16_t channels = 0;
  uint32_t sampleRate = 0;
  uint16_t bitsPerSample = 0;
  uint32_t wavDataSize = 0;
  uint8_t i;

  std::vector<Sound *> squealSounds;

  esp_task_wdt_reset();
  timerAlarmDisable(timer);

  Serial.println("     _____   ____   _    _  ______            _       ______  _____");
  Serial.println("    / ____| / __ \\ | |  | ||  ____|    /\\    | |     |  ____||  __ \\");
  Serial.println("   | (___  | |  | || |  | || |__      /  \\   | |     | |__   | |__) |");
  Serial.println("    \\___ \\ | |  | || |  | ||  __|    / /\\ \\  | |     |  __|  |  _  /");
  Serial.println("    ____) || |__| || |__| || |____  / ____ \\ | |____ | |____ | | \\ \\");
  Serial.println("   |_____/  \\___\\_\\ \\____/ |______|/_/    \\_\\|______||______||_|  \\_\\");
  Serial.println("\nM O T I O N   A C T I V A T E D   F L A N G E   S Q U E A L   S Y S T E M\n");

  Serial.print("Version: ");
  Serial.println(VERSION_STRING);

  Serial.print("Git Rev: ");
  Serial.println(GIT_REV, HEX);

  // Read NVM configuration
  preferences.begin("squeal", false);
  volumeStep = preferences.getUChar("volume", VOL_STEP_NOM);
  silenceDecisecsMax = preferences.getUChar("silenceMax", 50);
  silenceDecisecsMin = preferences.getUChar("silenceMin", 0);

  volumeUpCoef = preferences.getUChar("volumeUp", 10);
  if(0 == volumeUpCoef)
  {
    volumeUpCoef = 1;
    preferences.putUChar("volumeUp", volumeUpCoef);
  }

  volumeDownCoef = preferences.getUChar("volumeDown", 8);
  if(0 == volumeDownCoef)
  {
    volumeDownCoef = 1;
    preferences.putUChar("volumeDown", volumeDownCoef);
  }

  esp_task_wdt_reset();

  // Check SD card
  if(SD.begin())
  {
    // Check for and read config file
    File f = SD.open("/config.txt");
    if (f)
    {
      while(f.available())
      {
        char keyStr[128];
        char valueStr[128];
        bool kvFound = configKeyValueSplit(keyStr, sizeof(keyStr), valueStr, sizeof(valueStr), f.readStringUntil('\n').c_str());
        if (!kvFound)
          continue;

        // Okay, looks like we have a valid key/value pair, see if it's something we care about
        if (0 == strcmp(keyStr, "silenceMax"))
        {
          silenceDecisecsMax = atoi(valueStr);
        }
        else if (0 == strcmp(keyStr, "silenceMin"))
        {
          silenceDecisecsMin = atoi(valueStr);
        }
        else if (0 == strcmp(keyStr, "volumeUp"))
        {
          volumeUpCoef = atoi(valueStr);
        }
        else if (0 == strcmp(keyStr, "volumeDown"))
        {
          volumeDownCoef = atoi(valueStr);
        }
      }
    }
    f.close();

    // Find WAV files
    rootDir = SD.open("/");
    while(true)
    {
      esp_task_wdt_reset();
      wavFile = rootDir.openNextFile();

      if (!wavFile)
      {
        break;  // No more files
      }
      if(wavFile.isDirectory())
      {
        Serial.print("  Skipping directory: ");
        Serial.println(wavFile.name());
      }
      else
      {
        fileName = wavFile.name();
        fileNameLength = strlen(fileName);
        if(fileNameLength < 5)
          continue;  // Filename too short (x.wav = min 5 chars)
        const char *extension = &fileName[strlen(fileName)-4];
        if(strcasecmp(extension, ".wav"))
        {
          Serial.print("  Ignoring: ");
          Serial.println(fileName);
          continue;  // Not a wav file (by extension anyway)
        }
        
        if(!wavFile.find("fmt "))  // Includes trailing space
        {
          Serial.print("! No fmt section: ");
          Serial.println(fileName);
          continue;
        }

        wavFile.seek(wavFile.position() + 6);  // Seek to number of channels
        wavFile.read((uint8_t*)&channels, 2);  // Read channels - WAV is little endian, only works if uC is also little endian

        if(channels > 1)
        {
          Serial.print("! Not mono: ");
          Serial.println(fileName);
          continue;
        }

        wavFile.read((uint8_t*)&sampleRate, 4);  // Read sample rate - WAV is little endian, only works if uC is also little endian

        if((8000 != sampleRate) && (16000 != sampleRate) && (32000 != sampleRate) && (44100 != sampleRate))
        {
          Serial.print("! Incorrect sample rate: ");
          Serial.println(fileName);
          continue;
        }

        wavFile.seek(wavFile.position() + 6);  // Seek to bits per sample
        wavFile.read((uint8_t*)&bitsPerSample, 2);  // Read bits per sample - WAV is little endian, only works if uC is also little endian

        if(16 != bitsPerSample)
        {
          Serial.print("! Not 16-bit: ");
          Serial.println(fileName);
          continue;
        }

        if(!wavFile.find("data"))
        {
          Serial.print("! No data section: ");
          Serial.println(fileName);
          continue;
        }

        // If we got here, then it looks like a valid wav file
        // Get data length and offset

        wavFile.read((uint8_t*)&wavDataSize, 4);  // Read data size - WAV is little endian, only works if uC is also little endian
        // Offset is now the current position

        Serial.print("+ Adding ");
        Serial.print(fileName);
        Serial.print(" (");
        Serial.print(sampleRate);
        Serial.print(",");
        Serial.print(wavDataSize);
        Serial.print(",");
        Serial.print(wavFile.position());
        Serial.println(")");

        squealSounds.push_back(new SdSound(fileName, wavDataSize, wavFile.position(), sampleRate));
        usingSdSounds = true;
      }
      wavFile.close();
    }
    rootDir.close();
  }

  // Print configuration values
  Serial.print("Volume: ");
  Serial.println(volumeStep);
  Serial.print("Silence Max: ");
  Serial.print(silenceDecisecsMax/10.0, 1);
  Serial.println("s");
  Serial.print("Silence Min: ");
  Serial.print(silenceDecisecsMin/10.0, 1);
  Serial.println("s");
  Serial.print("Volume Up Coef: ");
  Serial.println(volumeUpCoef);
  Serial.print("Volume Down Coef: ");
  Serial.println(volumeDownCoef);

  Serial.println("");

  esp_task_wdt_reset();

  if(usingSdSounds)
  {
    Serial.print("Using SD card sounds (");
    Serial.print(squealSounds.size());
    Serial.println(")");
    // Quadruple blink blue
    digitalWrite(LEDA, 1); delay(250); digitalWrite(LEDA, 0); delay(250);
    esp_task_wdt_reset();
    digitalWrite(LEDA, 1); delay(250); digitalWrite(LEDA, 0); delay(250);
    esp_task_wdt_reset();
    digitalWrite(LEDA, 1); delay(250); digitalWrite(LEDA, 0); delay(250);
    esp_task_wdt_reset();
    digitalWrite(LEDA, 1); delay(250); digitalWrite(LEDA, 0); delay(250);
    esp_task_wdt_reset();
  }
  else
  {
    squealSounds.push_back(new MemSound(1, flangeClip01_wav, flangeClip01_wav_len, 16000));
    squealSounds.push_back(new MemSound(2, flangeClip02_wav, flangeClip02_wav_len, 16000));
    squealSounds.push_back(new MemSound(3, flangeClip03_wav, flangeClip03_wav_len, 16000));
    squealSounds.push_back(new MemSound(4, flangeClip04_wav, flangeClip04_wav_len, 16000));
    squealSounds.push_back(new MemSound(5, flangeClip05_wav, flangeClip05_wav_len, 16000));
    squealSounds.push_back(new MemSound(6, flangeClip06_wav, flangeClip06_wav_len, 16000));
    squealSounds.push_back(new MemSound(7, flangeClip07_wav, flangeClip07_wav_len, 16000));
    squealSounds.push_back(new MemSound(8, flangeClip08_wav, flangeClip08_wav_len, 16000));
    squealSounds.push_back(new MemSound(9, flangeClip09_wav, flangeClip09_wav_len, 16000));
    squealSounds.push_back(new MemSound(10, flangeClip10_wav, flangeClip10_wav_len, 16000));
    squealSounds.push_back(new MemSound(11, flangeClip11_wav, flangeClip11_wav_len, 16000));
    squealSounds.push_back(new MemSound(12, flangeClip12_wav, flangeClip12_wav_len, 16000));
    squealSounds.push_back(new MemSound(13, flangeClip13_wav, flangeClip13_wav_len, 16000));
    squealSounds.push_back(new MemSound(14, flangeClip14_wav, flangeClip14_wav_len, 16000));
    squealSounds.push_back(new MemSound(15, flangeClip15_wav, flangeClip15_wav_len, 16000));
    squealSounds.push_back(new MemSound(16, flangeClip16_wav, flangeClip16_wav_len, 16000));
//    squealSounds.push_back(new MemSound(17, flangeClip17_wav, flangeClip17_wav_len, 16000));
//    squealSounds.push_back(new MemSound(18, flangeClip18_wav, flangeClip18_wav_len, 16000));

    Serial.print("Using built-in sounds (");
    Serial.print(squealSounds.size());
    Serial.println(")");
    // Double blink blue
    digitalWrite(LEDA, 1); delay(250); digitalWrite(LEDA, 0); delay(250);
    esp_task_wdt_reset();
    digitalWrite(LEDA, 1); delay(250); digitalWrite(LEDA, 0); delay(250);
    esp_task_wdt_reset();
  }

  timerAlarmEnable(timer);

  while(1)
  {
    esp_task_wdt_reset();
    Serial.print("Heap free: ");
    Serial.println(esp_get_free_heap_size());

    // Don't play the same sample twice in a row
    // Have to initialize to something, so will never play sample 255 first
    // Can't be zero since it would never play anything with a single sample
    static uint8_t lastSampleNum = 255;
    uint8_t sampleNum;
    sampleNum = random(0, squealSounds.size());
    if(squealSounds.size() > 2)
    {
      // With three or more sounds, don't repeat the last one
      while(sampleNum == lastSampleNum)
      {
        esp_task_wdt_reset();
        sampleNum = random(0, squealSounds.size());
        Serial.println("*");
      }
    }
    Serial.print("Playing... ");
    Serial.println(sampleNum);
    play(squealSounds[sampleNum]);
    lastSampleNum = sampleNum;

    if(restart)
    {
      restart = false;
      Serial.print("\n*** Restarting ***\n\n");
      squealSounds.clear();
      break;  // Restart the loop() function
    }

    // Add some silence
    uint8_t silenceDecisecs = random(silenceDecisecsMin, silenceDecisecsMax);
    Serial.print("Silence... ");
    Serial.print(silenceDecisecs/10.0, 1);
    Serial.println("s");
    for(i=0; i<silenceDecisecs; i++)
    {
      esp_task_wdt_reset();
      Serial.print(".");
      delay(100);
    }
    Serial.println("");
  }
}
