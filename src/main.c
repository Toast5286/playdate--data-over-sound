#include <stdio.h>
#include <stdlib.h>
#define TARGET_EXTENSION 1

#include "fft.h"
#include "pd_api.h"

static PlaydateAPI* pd = NULL;

int eventHandler(PlaydateAPI* playdate, PDSystemEvent event, uint32_t arg) {
    if ( event == kEventInitLua )
	{
		pd = playdate;
        
	    registerFFT(pd);
    }
    return 0;
}
