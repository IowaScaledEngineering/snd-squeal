#include <SPI.h>
#include <SD.h>
#include <I2S.h>
#include <Preferences.h>
#include <vector>
#include <strings.h>

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
//#include "squeal/flangeClip11.h"
#include "squeal/flangeClip12.h"
#include "squeal/flangeClip13.h"
#include "squeal/flangeClip14.h"
#include "squeal/flangeClip15.h"
//#include "squeal/flangeClip16.h"
//#include "squeal/flangeClip17.h"
//#include "squeal/flangeClip18.h"

// Samples
#define AUDIO_BUFFER_SIZE 1024

// Bytes - needs to be 2x audio buffer since audio buffer is 16-bit samples
#define FILE_BUFFER_SIZE (AUDIO_BUFFER_SIZE * 2)

// Volume
#define VOL_MAX       15
#define VOL_NOM       10
#define VOL_UP_COEF   10
#define VOL_DN_COEF   8

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

uint8_t volumeStep = 0;        // 0 to 15
uint16_t volume = 0;              // 0 to 15000
uint8_t enable = 0;               // 0 or 1

// Bit positions for inputs
#define VOL_UP_BUTTON 0x01
#define VOL_DN_BUTTON 0x02
#define EN1_INPUT     0x10
#define EN2_INPUT     0x20
#define EN3_INPUT     0x40
#define EN4_INPUT     0x80

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

hw_timer_t * timer = NULL;

void IRAM_ATTR processVolume(void)
{
  static uint8_t buttonsPressed = 0, oldButtonsPressed = 0;
  static unsigned long pressTime = 0;
  uint8_t inputStatus = 0;

  digitalWrite(AUX5, 1);

  // Turn off LED
  uint16_t ledHoldTime = (VOL_NOM == volumeStep) ? 1000 : 100;
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
    if(volumeStep < VOL_MAX)
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
  if(0 == volumeStep)
  {
    volumeTarget = 0;
  }
  else if((1 <= volumeStep) && (volumeStep <= 10))
  {
    //  Logarithmic attenuation volume (20 to 10240)
    volumeTarget = 10 * (1 << volumeStep) * enable;
  }
  else
  {
    //  Linear amplification volume (11264 to 15360)
    volumeTarget = (10 + 2*(volumeStep-10)) * 1024 * enable;
  }

  if(volume < volumeTarget)
  {
    deltaVolume = (volumeTarget - volume);
    if((deltaVolume > 0) && (deltaVolume < VOL_UP_COEF))
      deltaVolume = VOL_UP_COEF;  // Make sure it goes all the way to min or max
    volume += deltaVolume  / VOL_UP_COEF;
//    Serial.println(volume);
  }
  else if(volume > volumeTarget)
  {
    deltaVolume = (volume - volumeTarget);
    if((deltaVolume > 0) && (deltaVolume < VOL_DN_COEF))
      deltaVolume = VOL_DN_COEF;  // Make sure it goes all the way to min or max
    volume -= deltaVolume / VOL_DN_COEF;
//    Serial.println(volume);
  }

  oldButtonsPressed = buttonsPressed;
  digitalWrite(AUX5, 0);
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

  delay(1000);
  Serial.print('.');
  delay(1000);
  Serial.print('.');
  delay(1000);
  Serial.println('.');

  Serial.print("Version: ");
  Serial.println(VERSION_STRING);

  Serial.print("Git Rev: ");
  Serial.println(GIT_REV, HEX);

  preferences.begin("squeal", false);
  volumeStep = preferences.getUChar("volume", 10);
  Serial.print("Initial Volume: ");
  Serial.println(volumeStep);

  timer = timerBegin(0, 80, true);  // Timer 0, 80x prescaler = 1us
  timerAttachInterrupt(timer, &processVolume, false);  // level triggered
  timerAlarmWrite(timer, 10000, true);  // 80MHz / 80 / 10000 = 10ms, autoreload
}

void play(Sound *wavSound)
{
  int i;
  size_t bytesRead;
  uint8_t fileBuffer[FILE_BUFFER_SIZE];
  int16_t sampleValue;

  wavSound->open();
  I2S.setSckPin(I2S_BCLK);
  I2S.setFsPin(I2S_LRCLK);
  I2S.setDataPin(I2S_DATA);
  I2S.setBufferSize(AUDIO_BUFFER_SIZE);

  if(!I2S.begin(I2S_PHILIPS_MODE, wavSound->getSampleRate(), 16))
  {
    Serial.println("Failed to initialize I2S!");
    return;  // Fail and try the next one
  }
  digitalWrite(I2S_SD, 1);  // Enable amplifier

  while(wavSound->available())
  {
    // Audio buffer samples are in 16-bit chunks, so multiply by two to get # of bytes to read
//    digitalWrite(AUX1, 1);
    bytesRead = wavSound->read(fileBuffer, (size_t)FILE_BUFFER_SIZE);
//    digitalWrite(AUX1, 0);
//    digitalWrite(AUX2, 1);
    for(i=0; i<bytesRead; i+=2)
    {
//      digitalWrite(AUX3, 1);  // Note: causes some audio interference - crosstalk?
      // File is read on a byte basis, so convert into int16 samples, and step every 2 bytes
      sampleValue = *((int16_t *)(fileBuffer+i));
      sampleValue = sampleValue * volume / (1024 * VOL_NOM);
      // Write twice (left & right)
      I2S.write(sampleValue);
      I2S.write(sampleValue);
//      digitalWrite(AUX3, 0);
    }
//    digitalWrite(AUX2, 0);
  }
  I2S.flush();
  delay(2 * 1000 * AUDIO_BUFFER_SIZE / wavSound->getSampleRate());  // Let buffer finish, length of 2 audio buffers in millisecs
  digitalWrite(I2S_SD, 0);  // Disable amplifier
  I2S.end();
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

  if(SD.begin())
  {
    rootDir = SD.open("/");
    while(true)
    {
      wavFile = rootDir.openNextFile();

      if (!wavFile)
      {
        break;  // No more files
      }
      if(wavFile.isDirectory())
      {
        Serial.print("  Skipping directory ");
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

  Serial.println("");

  if(usingSdSounds)
  {
    Serial.print("Using SD card sounds (");
    Serial.print(squealSounds.size());
    Serial.println(")");
    // Quadruple blink blue
    digitalWrite(LEDA, 1); delay(250); digitalWrite(LEDA, 0); delay(250);
    digitalWrite(LEDA, 1); delay(250); digitalWrite(LEDA, 0); delay(250);
    digitalWrite(LEDA, 1); delay(250); digitalWrite(LEDA, 0); delay(250);
    digitalWrite(LEDA, 1); delay(250); digitalWrite(LEDA, 0); delay(250);
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
//    squealSounds.push_back(new MemSound(11, flangeClip11_wav, flangeClip11_wav_len, 16000));
    squealSounds.push_back(new MemSound(12, flangeClip12_wav, flangeClip12_wav_len, 16000));
    squealSounds.push_back(new MemSound(13, flangeClip13_wav, flangeClip13_wav_len, 16000));
    squealSounds.push_back(new MemSound(14, flangeClip14_wav, flangeClip14_wav_len, 16000));
    squealSounds.push_back(new MemSound(15, flangeClip15_wav, flangeClip15_wav_len, 16000));
//    squealSounds.push_back(new MemSound(16, flangeClip16_wav, flangeClip16_wav_len, 16000));
//    squealSounds.push_back(new MemSound(17, flangeClip17_wav, flangeClip17_wav_len, 16000));
//    squealSounds.push_back(new MemSound(18, flangeClip18_wav, flangeClip18_wav_len, 16000));
    Serial.print("Using built-in sounds (");
    Serial.print(squealSounds.size());
    Serial.println(")");
    // Double blink blue
    digitalWrite(LEDA, 1); delay(250); digitalWrite(LEDA, 0); delay(250);
    digitalWrite(LEDA, 1); delay(250); digitalWrite(LEDA, 0); delay(250);
  }

  timerAlarmEnable(timer);

  while(1)
  {
    Serial.print("Heap free: ");
    Serial.println(esp_get_free_heap_size());

    uint8_t sampleNum = random(0, squealSounds.size());
    Serial.print("Playing... ");
    Serial.println(sampleNum);
    play(squealSounds[sampleNum]);
    if(!usingSdSounds)
    {
      // Add some silence
      uint8_t silenceDecisecs = random(0, 50);
      Serial.print("Silence... ");
      Serial.print(silenceDecisecs/10.0, 1);
      Serial.println("s");
      for(i=0; i<silenceDecisecs; i++)
      {
        delay(100);
      }
    }
  }
}
