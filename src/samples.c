#include "samples.h"

static PlaydateAPI* pd = NULL;

//Samples Struture and functions ---------------------------------
int extract_samples(lua_State* L);
int free_samples(lua_State* L);
int syntheticDataCreator(lua_State* L);
int getSample(lua_State* L);
int getNumSample(lua_State* L);
int getSampleFreq(lua_State* L);
int getSigEnergy(lua_State* L);

static const lua_reg samplelib[] =
{
	{ "new",            extract_samples},
	{ "free",           free_samples },
    { "syntheticData",  syntheticDataCreator},
	{ "getSample",      getSample },
    { "getLength",	    getNumSample },
	{ "gerSampleFreq",  getSampleFreq },
    { "getSignalEnergy",  getSigEnergy },
	{ NULL, NULL }
};
//-----------------------------------------------------------

//Registering Functions---------------------------------------
void registerSamples(PlaydateAPI* playdate){
    pd = playdate;

	const char* err;

	if ( !pd->lua->registerClass("samplelib.samples",samplelib,NULL, 0, &err) )
            pd->system->logToConsole("%s:%i: addFunction failed, %s", __FILE__, __LINE__, err);
}


//Sampler-----------------------------------------------------
/**
* Function:     extract_samples
* Arguments:    sample                  -playdate.sound.sample that we want to store
*               
* Returns:      s                       -samplelib.samples that we want to create
* Description:  Creates a samplelib.samples object
**/
int extract_samples(lua_State* L) {
    // Check if the first argument is a userdata (which should be a sound.sample)
    AudioSample* sample = pd->lua->getArgObject(1, "playdate.sound.sample", NULL);

    if((sample == NULL)){
        return 0;
    }

    Samples *s = pd->system->realloc(NULL, (sizeof(Samples)));
    s->data = NULL;

    // Get the raw data and length
    pd->sound->sample->getData(sample, &s->data, &s->SFormat, &s->SFreq, &s->length);

    s->length = (uint32_t) (s->length/2);

    if (s->data == NULL) {
        pd->lua->pushString("Failed to retrieve sample data");
        return 1;
    }

    // Push the raw audio data to the Lua stack
    pd->lua->pushObject(s, "samplelib.samples", 0);

    // Return the table
    return 1;

}

//Sampler-----------------------------------------------------
/**
* Function:     free_samples
* Arguments:    s                  -samplelib.samples to free
*               
* Returns:
* Description:  Frees memory
**/
int free_samples(lua_State* L){
    Samples* s = pd->lua->getArgObject(1, "samplelib.samples", NULL);
    
	// realloc with size 0 to free
	pd->system->realloc(s, 0);

	return 0;
}

//Sampler-----------------------------------------------------
/**
* Function:     syntheticDataCreator
* Arguments:    s                   -samplelib.samples to store the new data
*               freq                -Float that contains the frequency for the sinusoidal wave (if -1 will give a continuous signal)
*               Amp                 -Int that represents the amplitude of the sinusoidal wave
*               startIdx            -Int containing the starting index to create the synthetic data
*               endIdx              -Int containing the end index to create the synthetic data
*               
* Returns:
* Description:  Creates continuose of sinusoidal waves and stores there samples in object s
**/
int syntheticDataCreator(lua_State* L){
    const float pi = (float) 3.14159265358979323846;

    Samples* s = pd->lua->getArgObject(1, "samplelib.samples", NULL);
    float freq = pd->lua->getArgFloat(2);
    int Amp = pd->lua->getArgInt(3);
    int startIdx = pd->lua->getArgInt(4);
    int endIdx = pd->lua->getArgInt(5);

    if((s == NULL) || (s->data == NULL)){
        return 0;
    }
    if(startIdx > (int)s->length || startIdx < 0) startIdx = 0;
    if(endIdx > (int)s->length || endIdx < 0) endIdx = (int)s->length;
    

    float val=0;
    if(freq<=0){
        for(int i=startIdx;i<endIdx;i++){
	        s->data[i] = (short int)Amp;
        }

    }else{
        for(int i=startIdx;i<endIdx;i++){
            val = Amp*cos(freq* (((float)i)/((float)s->SFreq)) * 2*pi);
	        s->data[i] += (short int)val;
        }
    }

	return 0;
}
//Sampler-----------------------------------------------------
/**
* Function:     getSample
* Arguments:    s                   -samplelib.samples to store the new data
*               sampleN             -Int index of the sample we want to return
*               
* Returns:      s->data[sampleN]    -Int The sample that was requested
* Description:  Gets a sample stored in object s
**/
int getSample(lua_State* L){
    Samples* s = pd->lua->getArgObject(1, "samplelib.samples", NULL);
    int sampleN = pd->lua->getArgInt(2);
    if((s == NULL) || (s->data == NULL)){
        pd->lua->pushInt(-1);
        return 1;
    }

    if(sampleN > (int)s->length || sampleN < 0){
        pd->lua->pushInt(0);
	    return 1;
    }

    pd->lua->pushInt((int) s->data[sampleN]);

	return 1;
}

//Sampler-----------------------------------------------------
/**
* Function:     getNumSample
* Arguments:    s                   -samplelib.samples to store the new data
*               
* Returns:      s->length           -Int The length of the stored sample
* Description:  Gets the length of object s
**/
int getNumSample(lua_State* L){
    Samples* s = pd->lua->getArgObject(1, "samplelib.samples", NULL);
    if((s == NULL) || (s->data == NULL)){
        pd->lua->pushInt(-1);
        return 1;
    }

    pd->lua->pushInt((int)s->length);

	return 1;
}

//Sampler-----------------------------------------------------
/**
* Function:     getSampleFreq
* Arguments:    s                   -samplelib.samples to store the new data
*               
* Returns:      s->SFreq           -Int The Sampling frequency
* Description:  Gets a sampling frequency of object s
**/
int getSampleFreq(lua_State* L){
    Samples* s = pd->lua->getArgObject(1, "samplelib.samples", NULL);
    if((s == NULL) || (s->data == NULL)){
        pd->lua->pushInt(-1);
        return 1;
    }

    pd->lua->pushInt((int)s->SFreq);

	return 1;
}

//Sampler-----------------------------------------------------
/**
* Function:     getSigEnergy
* Arguments:    s                   -samplelib.samples to store the new data
*               startIdx            -Int Index to start analysing
*               endIdx              -Int Index to end the analysis
*               
* Returns:      Energy              -Float represent the energy of the signal
* Description:  Get a estimation of the energy of the signal between the start index and end index
**/
int getSigEnergy(lua_State* L){
    Samples* s = pd->lua->getArgObject(1, "samplelib.samples", NULL);
    int startIdx = pd->lua->getArgInt(2);
    int endIdx = pd->lua->getArgInt(3);

    if((s == NULL) || (s->data == NULL)){
        return 0;
    }

    if(startIdx > (int)s->length || startIdx < 0) startIdx = 0;
    if(endIdx > (int)s->length || endIdx < 0) endIdx = (int)s->length;

    float Energy = 0;
    float mean = 0;

    for(int i=startIdx;i<endIdx;i++){
        mean+=s->data[i];
    }
    mean/=(endIdx-startIdx);

    for(int i=startIdx;i<endIdx;i++){
        Energy+=pow((s->data[i]-mean),2);
    }
    Energy/=(endIdx-startIdx);

    pd->lua->pushFloat(Energy);

	return 1;
}