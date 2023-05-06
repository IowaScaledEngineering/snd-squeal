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
#define TEST1     15

uint8_t desiredVolume = VOL_NOM;  // 0 to 15
uint16_t volume = 0;              // 0 to 15000
uint8_t enable = 0;               // 0 or 1

#define VOL_UP_BUTTON 0x01
#define VOL_DN_BUTTON 0x02
#define EN1_INPUT     0x10
#define EN2_INPUT     0x20
#define EN3_INPUT     0x40
#define EN4_INPUT     0x80

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

  digitalWrite(TEST1, 1);

  // Turn off LED
  uint16_t ledHoldTime = (VOL_NOM == desiredVolume) ? 1000 : 100;
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
    if(desiredVolume < VOL_MAX)
      desiredVolume++;
      // FIXME: Add NVM volume write
    Serial.print("Vol Up: ");
    Serial.println(desiredVolume);
    digitalWrite(LEDB, 1);
  }

  // Find rising edge of volume down button
  if((buttonsPressed ^ oldButtonsPressed) & (buttonsPressed & VOL_DN_BUTTON))
  {
    pressTime = millis();
    if(desiredVolume > 0)
      desiredVolume--;
    // FIXME: Add NVM volume write
    Serial.print("Vol Dn: ");
    Serial.println(desiredVolume);
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
  uint16_t volumeTarget = 1000 * desiredVolume * enable;

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
  digitalWrite(TEST1, 0);
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

  if (!SD.begin()) {
    Serial.println("SD card initialization failed!");
    return;
  }

  // FIXME: read volume from NVM

  timer = timerBegin(0, 80, true);  // Timer 0, 80x prescaler = 1us
  timerAttachInterrupt(timer, &processVolume, false);  // level triggered
  timerAlarmWrite(timer, 10000, true);  // 80MHz / 80 / 10000 = 10ms, autoreload
  timerAlarmEnable(timer);
}

void play(Sound *wavSound)
{
  int i;
  size_t bytesRead;
  uint8_t fileBuffer[FILE_BUFFER_SIZE];
  int16_t sampleValue;

  if(wavSound->open())
  {
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
      bytesRead = wavSound->read(fileBuffer, (size_t)FILE_BUFFER_SIZE);
      for(i=0; i<bytesRead; i+=2)
      {
        // File is read on a byte basis, so convert into int16 samples, and step every 2 bytes
        sampleValue = *((int16_t *)(fileBuffer+i));
        sampleValue = sampleValue * volume / (1000 * VOL_NOM);
        // Write twice (left & right)
        I2S.write(sampleValue);
        I2S.write(sampleValue);
      }
    }
    wavSound->close();
    I2S.flush();
    digitalWrite(I2S_SD, 0);  // Disable amplifier
    // FIXME: why is this delay needed?  The amplifier should shut off quickly after I2S_SD falls
    delay(100);  // 50ms = click/pop, 75ms = no click/pop
    I2S.end();
  }
}

void loop()
{
  Serial.println("Starting...");

  std::vector<Sound *> squealSounds;
  squealSounds.push_back(new MemSound(getSqueal(1), getSquealSize(1), 16000));
  squealSounds.push_back(new MemSound(getSqueal(2), getSquealSize(2), 16000));
  squealSounds.push_back(new MemSound(getSqueal(3), getSquealSize(3), 16000));
  squealSounds.push_back(new MemSound(getSqueal(4), getSquealSize(4), 16000));
  squealSounds.push_back(new MemSound(getSqueal(5), getSquealSize(5), 16000));
  squealSounds.push_back(new MemSound(getSqueal(6), getSquealSize(6), 16000));
  squealSounds.push_back(new MemSound(getSqueal(7), getSquealSize(7), 16000));
  squealSounds.push_back(new MemSound(getSqueal(8), getSquealSize(8), 16000));
  squealSounds.push_back(new MemSound(getSqueal(9), getSquealSize(9), 16000));
  squealSounds.push_back(new MemSound(getSqueal(10), getSquealSize(10), 16000));
  squealSounds.push_back(new MemSound(getSqueal(11), getSquealSize(11), 16000));
  squealSounds.push_back(new MemSound(getSqueal(12), getSquealSize(12), 16000));
  squealSounds.push_back(new MemSound(getSqueal(13), getSquealSize(13), 16000));
  squealSounds.push_back(new MemSound(getSqueal(14), getSquealSize(14), 16000));
  squealSounds.push_back(new MemSound(getSqueal(15), getSquealSize(15), 16000));
  squealSounds.push_back(new MemSound(getSqueal(16), getSquealSize(16), 16000));

  while(1)
  {
    Serial.print("Heap free: ");
    Serial.println(esp_get_free_heap_size());

    uint8_t sampleNum = random(0, squealSounds.size());
    Serial.print("Playing... ");
    Serial.println(sampleNum);
    play(squealSounds[sampleNum]);
  }
}
