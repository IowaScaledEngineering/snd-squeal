class Sound
{
  protected:
    size_t dataSize;
    size_t byteCount;

  public:
    virtual bool open(void);
    virtual size_t available(void);
    virtual size_t read(uint8_t *buffer, size_t numBytes);
    virtual void close(void);
};

class SdSound : public Sound
{
  char *fileName;

  public:
    SdSound(char *fname)
    {
      fileName = strdup(fname);
    }
    ~SdSound()
    {
      free(fileName);
    }
    virtual bool open(void)
    {
      Serial.println(fileName);
      return true;
    }
    virtual size_t available(void)
    {

    }
    virtual size_t read(uint8_t *buffer, size_t numBytes)
    {

    }
    virtual void close(void)
    {

    }
};

class MemSound : public Sound
{
  const uint8_t *dataPtr;

  public:
    MemSound(const uint8_t *sound, size_t soundSize)
    {
      dataPtr = sound;
      dataSize = soundSize;
      byteCount = 0;
    }
    ~MemSound()
    {
      // No need to free dataPtr since it is const
    }
    virtual bool open(void)
    {
      return true;
    }
    virtual size_t available(void)
    {
      return(dataSize - byteCount);
    }
    virtual size_t read(uint8_t *buffer, size_t numBytes)
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
    virtual void close(void)
    {
      byteCount = 0;
    }
};
