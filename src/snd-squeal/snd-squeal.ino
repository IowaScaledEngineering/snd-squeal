#include <SPI.h>
#include <SD.h>
#include <I2S.h>
#include <vector>

#include "sound.h"
#include "squeal/squeal.h"

// Samples
#define AUDIO_BUFFER_SIZE 1024
// Bytes
#define FILE_BUFFER_SIZE AUDIO_BUFFER_SIZE * 2

// Volume
#define VOL_MAX   15
#define VOL_NOM   10
#define VOL_DLY   200

// Pins
#define EN1       9
#define EN2       10
#define EN3       3
#define EN4       46
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

uint8_t desiredVolume = 1;  // 0 to 15
uint8_t volume = 0;         // 0 to 150
uint8_t enable = 0;         // 0 or 1

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200);

  pinMode(VOLDN, INPUT_PULLUP);
  pinMode(VOLUP, INPUT_PULLUP);
  pinMode(I2S_SD, OUTPUT);
  digitalWrite(I2S_SD, 1);  // Enable amplifier

  pinMode(LEDA, OUTPUT);
  pinMode(LEDB, OUTPUT);
  digitalWrite(LEDA, 0);
  digitalWrite(LEDB, 0);

  pinMode(EN1, INPUT_PULLUP);
  pinMode(EN2, INPUT_PULLUP);
  pinMode(EN3, INPUT_PULLUP);
  pinMode(EN4, INPUT_PULLUP);

  if (!SD.begin()) {
    Serial.println("SD card initialization failed!");
    return;
  }

  I2S.setSckPin(I2S_BCLK);
  I2S.setFsPin(I2S_LRCLK);
  I2S.setDataPin(I2S_DATA);

  I2S.setBufferSize(AUDIO_BUFFER_SIZE);

  if(!I2S.begin(I2S_PHILIPS_MODE, 16000, 16))
  {
      Serial.println("Failed to initialize I2S!");
      while (1)
      {
        digitalWrite(LEDA, 1);  digitalWrite(LEDB, 0);  delay(150);
        digitalWrite(LEDA, 0);  digitalWrite(LEDB, 0);  delay(150);
        digitalWrite(LEDA, 1);  digitalWrite(LEDB, 0);  delay(150);
        digitalWrite(LEDA, 0);  digitalWrite(LEDB, 0);  delay(150);
        digitalWrite(LEDA, 0);  digitalWrite(LEDB, 1);  delay(150);
        digitalWrite(LEDA, 0);  digitalWrite(LEDB, 0);  delay(150);
        digitalWrite(LEDA, 0);  digitalWrite(LEDB, 1);  delay(150);
        digitalWrite(LEDA, 0);  digitalWrite(LEDB, 0);  delay(150);
      }
  }

  // FIXME: read volume from NVM

}

void processVolume(void)
{
  static unsigned long pressTime = 0;
  static unsigned long stepTime = 0;
  bool holdoffDone = (millis() - pressTime) > VOL_DLY;
  if(holdoffDone)
  {
    // Turn off LED if past delay time
    digitalWrite(LEDB, 0);
  }

  if(!digitalRead(VOLUP) && holdoffDone)
  {
    pressTime = millis();
    if(desiredVolume < VOL_MAX)
      desiredVolume++;
      // FIXME: Add NVM volume write
    Serial.print("Vol Up: ");
    Serial.println(desiredVolume);
    digitalWrite(LEDB, 1);
  }
  else if(!digitalRead(VOLDN) && holdoffDone)
  {
    pressTime = millis();
    if(desiredVolume > 0)
      desiredVolume--;
    // FIXME: Add NVM volume write
    Serial.print("Vol Dn: ");
    Serial.println(desiredVolume);
    digitalWrite(LEDB, 1);
  }

  enable = !digitalRead(EN1) || !digitalRead(EN2);

  if(enable)
  {
    digitalWrite(LEDA, 1);
  }
  else
  {
    digitalWrite(LEDA, 0);
  }

  if(volume < (10 * desiredVolume * enable))
  {
    volume++;
  }
  else if(volume > (10 * desiredVolume * enable))
  {
    volume--;
  }
}

void play(Sound *wavSound)
{
  int i;
  size_t bytesRead;
  uint8_t fileBuffer[FILE_BUFFER_SIZE];
  int16_t sampleValue;

  if(wavSound->open())
  {
    while(wavSound->available())
    {
      processVolume();
      // Audio buffer samples are in 16-bit chunks, so multiply by two to get # of bytes to read
      bytesRead = wavSound->read(fileBuffer, (size_t)FILE_BUFFER_SIZE);
      for(i=0; i<bytesRead; i+=2)
      {
        // File is read on a byte basis, so convert into int16 samples, and step every 2 bytes
        sampleValue = *((int16_t *)(fileBuffer+i));
        sampleValue = sampleValue * volume / 100;
        // Write twice (left & right)
        I2S.write(sampleValue);
        I2S.write(sampleValue);
      }
    }
    wavSound->close();
    I2S.flush();
  }
}

void loop()
{
  Serial.println("Starting...");

  std::vector<Sound *> squealSounds;
  squealSounds.push_back(new MemSound(getSqueal(1), getSquealSize(1)));
  squealSounds.push_back(new MemSound(getSqueal(2), getSquealSize(2)));
  squealSounds.push_back(new MemSound(getSqueal(3), getSquealSize(3)));
  squealSounds.push_back(new MemSound(getSqueal(4), getSquealSize(4)));
  squealSounds.push_back(new MemSound(getSqueal(5), getSquealSize(5)));
  squealSounds.push_back(new MemSound(getSqueal(6), getSquealSize(6)));
  squealSounds.push_back(new MemSound(getSqueal(7), getSquealSize(7)));
  squealSounds.push_back(new MemSound(getSqueal(8), getSquealSize(8)));
  squealSounds.push_back(new MemSound(getSqueal(9), getSquealSize(9)));
  squealSounds.push_back(new MemSound(getSqueal(10), getSquealSize(10)));
  squealSounds.push_back(new MemSound(getSqueal(11), getSquealSize(11)));
  squealSounds.push_back(new MemSound(getSqueal(12), getSquealSize(12)));
  squealSounds.push_back(new MemSound(getSqueal(13), getSquealSize(13)));
  squealSounds.push_back(new MemSound(getSqueal(14), getSquealSize(14)));
  squealSounds.push_back(new MemSound(getSqueal(15), getSquealSize(15)));
  squealSounds.push_back(new MemSound(getSqueal(16), getSquealSize(16)));

  while(1)
  {
    Serial.print("Heap free: ");
    Serial.println(esp_get_free_heap_size());

/*
    Serial.println(squealSounds[0]->available());
    Serial.println(squealSounds[1]->available());
    Serial.println(squealSounds[2]->available());
    Serial.println(squealSounds[3]->available());
    Serial.println(squealSounds[4]->available());
    Serial.println(squealSounds[5]->available());
    Serial.println(squealSounds[6]->available());
    Serial.println(squealSounds[7]->available());
    Serial.println(squealSounds[8]->available());
    Serial.println(squealSounds[9]->available());
    Serial.println(squealSounds[10]->available());
    Serial.println(squealSounds[11]->available());
    Serial.println(squealSounds[12]->available());
    Serial.println(squealSounds[13]->available());
    Serial.println(squealSounds[14]->available());
    Serial.println(squealSounds[15]->available());
*/
    
    uint8_t sampleNum = random(0, squealSounds.size());
    Serial.print("Playing... ");
    Serial.println(sampleNum);
    play(squealSounds[sampleNum]);

/* 
    Serial.println("");
    delay(500);
    digitalWrite(LEDA, 1);  delay(150);
    digitalWrite(LEDA, 0);  delay(150);
    digitalWrite(LEDB, 1);  delay(150);
    digitalWrite(LEDB, 0);  delay(150);
 */
  }
}
