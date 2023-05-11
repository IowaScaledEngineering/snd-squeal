class Sound
{
  protected:
    size_t dataSize;
    size_t byteCount;
    uint16_t sampleRate;

  public:
    virtual void open(void);
    virtual size_t available(void);
    virtual size_t read(uint8_t *buffer, size_t numBytes);
    virtual void close(void);
    uint16_t getSampleRate(void)
    {
      return sampleRate;
    }
};

class SdSound : public Sound
{
  char *fileName;
  size_t fileOffset;

  public:
    SdSound(char *fname, size_t numBytes, size_t offset, uint16_t sr)
    {
      fileName = strdup(fname);
      fileOffset = offset;
      dataSize = numBytes;
      sampleRate = sr;
    }
    ~SdSound()
    {
      free(fileName);
    }
    void open(void)
    {
      Serial.println(fileName);
      // Seek to fmt marker
      // Validate format
      // Seek until data section
      // Set dataSize
      byteCount = 0;
    }
    size_t available(void)
    {

    }
    size_t read(uint8_t *buffer, size_t numBytes)
    {

    }
    void close(void)
    {

    }
};

class MemSound : public Sound
{
  const uint8_t *dataPtr;

  public:
    MemSound(const uint8_t *sound, size_t soundSize, uint16_t sr)
    {
      dataPtr = sound;
      dataSize = soundSize;
      sampleRate = sr;
    }
    ~MemSound()
    {
      // No need to free dataPtr since it is const
    }
    void open(void)
    {
      byteCount = 0;
    }
    size_t available(void)
    {
      return(dataSize - byteCount);
    }
    size_t read(uint8_t *buffer, size_t numBytes)
    {
      size_t bytesToRead;
      if(available() < numBytes)
        bytesToRead = available();
      else
        bytesToRead = numBytes;
      memcpy(buffer, dataPtr+byteCount, bytesToRead);
      byteCount += bytesToRead;
      return bytesToRead;
    }
    void close(void)
    {
      return;
    }
};
