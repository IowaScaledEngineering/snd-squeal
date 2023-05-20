#include "flangeClip01.h"
#include "flangeClip02.h"
#include "flangeClip03.h"
#include "flangeClip04.h"
#include "flangeClip05.h"
#include "flangeClip06.h"
#include "flangeClip07.h"
#include "flangeClip08.h"
#include "flangeClip09.h"
#include "flangeClip10.h"
//#include "flangeClip11.h"
#include "flangeClip12.h"
#include "flangeClip13.h"
#include "flangeClip14.h"
#include "flangeClip15.h"
//#include "flangeClip16.h"
//#include "flangeClip17.h"
//#include "flangeClip18.h"

const uint8_t * getSqueal(uint8_t num)
{
    switch(num)
    {
        case 1: return flangeClip01_wav;
        case 2: return flangeClip02_wav;
        case 3: return flangeClip03_wav;
        case 4: return flangeClip04_wav;
        case 5: return flangeClip05_wav;
        case 6: return flangeClip06_wav;
        case 7: return flangeClip07_wav;
        case 8: return flangeClip08_wav;
        case 9: return flangeClip09_wav;
        case 10: return flangeClip10_wav;
        case 11: return NULL;
        case 12: return flangeClip12_wav;
        case 13: return flangeClip13_wav;
        case 14: return flangeClip14_wav;
        case 15: return flangeClip15_wav;
        case 16: return NULL;
        case 17: return NULL;
        case 18: return NULL;
        default:
            return NULL;
    }
}

size_t getSquealSize(uint8_t num)
{
    switch(num)
    {
        case 1: return flangeClip01_wav_len;
        case 2: return flangeClip02_wav_len;
        case 3: return flangeClip03_wav_len;
        case 4: return flangeClip04_wav_len;
        case 5: return flangeClip05_wav_len;
        case 6: return flangeClip06_wav_len;
        case 7: return flangeClip07_wav_len;
        case 8: return flangeClip08_wav_len;
        case 9: return flangeClip09_wav_len;
        case 10: return flangeClip10_wav_len;
        case 11: return 0;
        case 12: return flangeClip12_wav_len;
        case 13: return flangeClip13_wav_len;
        case 14: return flangeClip14_wav_len;
        case 15: return flangeClip15_wav_len;
        case 16: return 0;
        case 17: return 0;
        case 18: return 0;
        default:
            return 0;
    }
}
