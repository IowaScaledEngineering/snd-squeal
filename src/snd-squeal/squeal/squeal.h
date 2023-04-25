#include "flange01.h"
#include "flange02.h"
#include "flange03.h"
#include "flange04.h"
#include "flange05.h"
#include "flange06.h"
#include "flange07.h"
#include "flange08.h"
#include "flange09.h"
#include "flange10.h"
#include "flange11.h"
#include "flange12.h"
#include "flange13.h"
#include "flange14.h"
#include "flange15.h"
#include "flange16.h"

#define NUM_FLANGE_SOUNDS 16

const uint8_t * getSqueal(uint8_t num)
{
    switch(num)
    {
        case 1: return flange01_wav;
        case 2: return flange02_wav;
        case 3: return flange03_wav;
        case 4: return flange04_wav;
        case 5: return flange05_wav;
        case 6: return flange06_wav;
        case 7: return flange07_wav;
        case 8: return flange08_wav;
        case 9: return flange09_wav;
        case 10: return flange10_wav;
        case 11: return flange11_wav;
        case 12: return flange12_wav;
        case 13: return flange13_wav;
        case 14: return flange14_wav;
        case 15: return flange15_wav;
        case 16: return flange16_wav;
        default:
            return NULL;
    }
}

const size_t getSquealSize(uint8_t num)
{
    switch(num)
    {
        case 1: return flange01_wav_len;
        case 2: return flange02_wav_len;
        case 3: return flange03_wav_len;
        case 4: return flange04_wav_len;
        case 5: return flange05_wav_len;
        case 6: return flange06_wav_len;
        case 7: return flange07_wav_len;
        case 8: return flange08_wav_len;
        case 9: return flange09_wav_len;
        case 10: return flange10_wav_len;
        case 11: return flange11_wav_len;
        case 12: return flange12_wav_len;
        case 13: return flange13_wav_len;
        case 14: return flange14_wav_len;
        case 15: return flange15_wav_len;
        case 16: return flange16_wav_len;
        default:
            return NULL;
    }
}
