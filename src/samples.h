#ifndef samples_h
#define samples_h

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define TARGET_EXTENSION 1

#include "pd_api.h"

typedef struct
{
	short int *data;
    SoundFormat SFormat;
    uint32_t SFreq;
    uint32_t length;
} Samples;

void registerSamples(PlaydateAPI* playdate);

#endif /* samples_h */