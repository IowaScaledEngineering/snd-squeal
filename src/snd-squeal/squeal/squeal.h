#include "flangeMix.h"

#define NUM_FLANGE_SOUNDS 1

const uint8_t * getSqueal(uint8_t num)
{
    switch(num)
    {
        case 1: return flangeMix_wav;
        default:
            return NULL;
    }
}

size_t getSquealSize(uint8_t num)
{
    switch(num)
    {
        case 1: return flangeMix_wav_len;
        default:
            return 0;
    }
}
