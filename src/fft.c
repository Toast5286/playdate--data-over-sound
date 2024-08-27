#include "samples.h"
#include "fft.h"

#define PI 3.1415926535897932384626433832795028841971

static PlaydateAPI* pd = NULL;

//FFT Struture and functions ---------------------------------

typedef struct
{
	float *data_re;
    float *data_im;
    uint32_t length;
    uint32_t FreqDomain;
} fftData;

int newFFT(lua_State* L);
int free_fft(lua_State* L);
int runFFT(lua_State* L);
int getAbsFFT(lua_State* L);
int getPhaseFFT(lua_State* L);
int getLengthFFT(lua_State* L);
int DomainFFT(lua_State* L);
int getAbsMaxFFT(lua_State* L);

//Real FFT algorithm
uint32_t addPading(fftData* f);
void fft(float *real, float *imag, int n);
void apply_hamming_window(float *real, int n);

static const lua_reg fftlib[] =
{
	{ "new",            newFFT},
	{ "free",           free_fft },
    { "runFFT",           runFFT },
	{ "getAbsFreq",      getAbsFFT },
    { "getPhaseFreq",      getPhaseFFT },
    { "getLength",	    getLengthFFT },
	{ "isFreqDomain",  DomainFFT },
    { "getAbsMax",  getAbsMaxFFT },
	{ NULL, NULL }
};
//------------------------------------------------------------

//Registering Functions---------------------------------------

void registerFFT(PlaydateAPI* playdate){
    pd = playdate;

	const char* err;
    registerSamples(pd);

	if ( !pd->lua->registerClass("fftlib.fft",fftlib,NULL, 0, &err) )
            pd->system->logToConsole("%s:%i: addFunction failed, %s", __FILE__, __LINE__, err);
}
//--------------------------------------------------------------

//FFT----------------------------------------------------------

/**
* Function:     newFFT
* Arguments:    s                   -samplelib.samples to get the samples
*               startIdx            -Int starting index to get the samples
*               endIdx              -Int end index to get the samples
*               
* Returns:      f                   -fftlib.fft object that contains the required samples
* Description:  Creates a fftlib.fft object containing the samples requested
**/
int newFFT(lua_State* L){
    Samples* s = pd->lua->getArgObject(1, "samplelib.samples", NULL);
    if((s == NULL) || (s->data == NULL)){
        return 0;
    }
    int startIdx = pd->lua->getArgInt(2);
    int endIdx = pd->lua->getArgInt(3);

    if((startIdx > (int)s->length) || (startIdx < 0)) startIdx = 0;
    if((endIdx > (int)s->length) || (endIdx < 0)) endIdx = (int)s->length;

    fftData *f = pd->system->realloc(NULL, (sizeof(fftData)));
    if((f == NULL)){
        return 0;
    }
    int size = endIdx-startIdx;
    f->length = size;
    //(uint32_t)

    f->data_re = pd->system->realloc(NULL, (sizeof(float)* size));
    f->data_im = pd->system->realloc(NULL, (sizeof(float) * size));

    int i=0,j=0;

    for(i=startIdx;i<endIdx;i++){
        f->data_re[j] = ((float)s->data[i]);
        f->data_im[j] = 0;

        j++;
    }

    f->FreqDomain = 0;

    pd->lua->pushObject(f, "fftlib.fft", 0);

	return 1;
}

/**
* Function:     free_fft
* Arguments:    f                   -fftlib.fft object to free
*               
* Returns:
* Description:  Frees memory
**/
int free_fft(lua_State* L){
    fftData* f = pd->lua->getArgObject(1, "fftlib.fft", NULL);

    if(f==NULL){
        return 0;
    }

	// realloc with size 0 to free
    pd->system->realloc(f->data_re, 0);
    pd->system->realloc(f->data_im, 0);
	pd->system->realloc(f, 0);

	return 0;
}

/**
* Function:     runFFT
* Arguments:    f                   -fftlib.fft object used to run FFT and store the results
*               
* Returns:
* Description:  Runs Padding + hamming_window + FFT
**/
int runFFT(lua_State* L){
    fftData* f = pd->lua->getArgObject(1, "fftlib.fft", NULL);
    if((f == NULL) || (f->data_re == NULL) || (f->data_im == NULL)){
        return 0;
    }

    f->length = addPading(f);
    int n = f->length;
    apply_hamming_window(f->data_re, n);
    fft(f->data_re,f->data_im,n);

    //Normalizing amplitude to be the same as the input signal
    f->data_re[0] /= (n/3.572);
    f->data_im[0] /= (n/3.572);
    for(int i=1;i<n;i++){
        f->data_re[i] *= 2/(n/3.572);
        f->data_im[i] *= 2/(n/3.572);
    }

    //Change the domain indicator
    if(f->FreqDomain == 0){
        f->FreqDomain = 1;  
    }else{
        f->FreqDomain = 0; 
    }

    return 0;
}

/**
* Function:     getAbsFFT
* Arguments:    f                   -fftlib.fft object to analyse
*               idx                 -Int bin to analyse
*               
* Returns:      absVal              -Float The absolute Value/Magnitude
* Description:  gets the magnitude for the specific frequency in the idx bin
**/
int getAbsFFT(lua_State* L){
    fftData* f = pd->lua->getArgObject(1, "fftlib.fft", NULL);
    int idx = pd->lua->getArgInt(2);

    if((f == NULL) || (f->data_re == NULL) || (f->data_im == NULL)){
        pd->lua->pushFloat(-1);
        return 1;
    }

    //Since we are in using a descreate singal, the frequency domain signal is infinite and repeats every f->length
    //These while loops makes sure the user still receives the correct Abs Value
    while(idx > (int)f->length){
        idx=idx-f->length;
    }
    while(idx<0){
        idx=idx+f->length;
    }

    double absVal = sqrt(pow(f->data_re[idx],2) + pow(f->data_im[idx],2));
    
    pd->lua->pushFloat((float)absVal);

    return 1;
}

/**
* Function:     getPhaseFFT
* Arguments:    f                   -fftlib.fft object to analyse
*               idx                 -Int bin to analyse
*               
* Returns:      phaseVal            -Float The Phase value
* Description:  gets the phase for the specific frequency in the idx bin
**/
int getPhaseFFT(lua_State* L){
    fftData* f = pd->lua->getArgObject(1, "fftlib.fft", NULL);
    int idx = pd->lua->getArgInt(2);

    if((f == NULL) || (f->data_re == NULL) || (f->data_im == NULL)){
        pd->lua->pushFloat(-1);
        return 1;
    }

    //Since we are in using a descreate singal, the frequency domain signal is infinite and repeats every f->length
    //These while loops makes sure the user still receives the correct Phase Value
    while(idx > (int)f->length){
        idx=idx-f->length;
    }
    while(idx < 0){
        idx=idx+f->length;
    }

    double phaseVal = atan2((double)f->data_im[idx],(double)f->data_re[idx]);

    pd->lua->pushFloat((float)phaseVal);

    return 1;
}

/**
* Function:     getLengthFFT
* Arguments:    f                   -fftlib.fft object to analyse
*               
* Returns:
* Description:  gets the length of the samples/bins in the fftlib.fft object
**/
int getLengthFFT(lua_State* L){
    fftData* f = pd->lua->getArgObject(1, "fftlib.fft", NULL);

    if((f == NULL) || (f->data_re == NULL) || (f->data_im == NULL)){
        pd->lua->pushInt(-1);
        return 1;
    }
    
    pd->lua->pushInt((int)f->length);

    return 1;
}


/**
* Function:     getLengthFFT
* Arguments:    f                   -fftlib.fft object to analyse
*               
* Returns:      f->FreqDomain       -Int value that represents if the object's values are in the frequency domain or the time domain
* Description:  Tells the user if the fftlib.fft object is in the time domain or frequency domain
**/
int DomainFFT(lua_State* L){
    fftData* f = pd->lua->getArgObject(1, "fftlib.fft", NULL);
    if((f == NULL) || (f->data_re == NULL) || (f->data_im == NULL)){
        pd->lua->pushInt(-1);
        return 1;
    }
    
    pd->lua->pushInt((int)f->FreqDomain);

    return 1;
}

/**
* Function:     getAbsMaxFFT
* Arguments:    f                   -fftlib.fft object to analyse
*               startIdx            -Int containing the starting index
*               endIdx              -Int containing the end index
*               
* Returns:      Max                 -Float max magnitude found
*               MaxFreq             -Int bin of the max magnitude found
* Description:  Searches between the startIdx and endIdx for the frequency with the highest magnitude and returns both the magnitude and its bin
**/
int getAbsMaxFFT(lua_State* L){
    fftData* f = pd->lua->getArgObject(1, "fftlib.fft", NULL);
    int startIdx = pd->lua->getArgInt(2);
    int endIdx = pd->lua->getArgInt(3);

    if((f == NULL) || (f->data_re == NULL) || (f->data_im == NULL)){
        pd->lua->pushFloat(1);
        pd->lua->pushInt(-1);
        return 2;
    }

    if(startIdx > (int)f->length || startIdx < 1) startIdx = 1;
    if(endIdx > (int)f->length || endIdx < 1) endIdx = f->length/2;

    double Max = 0;
    int MaxFreq = 0;

    for(int i = startIdx;i<endIdx;i++){
        double absVal = sqrt(pow(f->data_re[i],2) + pow(f->data_im[i],2));
        if(absVal>Max){
            Max = absVal;
            MaxFreq = i;
        }
    }
    
    
    pd->lua->pushFloat((float)Max);
    pd->lua->pushInt((int)MaxFreq);

    return 2;
}


//The Real FFT----------------------------
/**
* Function:     addPading
* Arguments:    f                   -fftlib.fft object to analyse
*               
* Returns:      newN                 -Int new f's length
* Description:  Creates Padding in the fftlib.fft object (Since FFT only works with powers of 2)
**/
uint32_t addPading(fftData* f){
    uint32_t n = f->length;
    int i=0;

    //Find the closest power of 2
    uint32_t newN = 1;
    while(n>newN && newN>0){
        newN = newN<<1;
    }
    if(n==newN) return n;

    //Allocate more space
    f->data_re = pd->system->realloc(f->data_re, (sizeof(float)* newN));
    f->data_im = pd->system->realloc(f->data_im, (sizeof(float) * newN));

    //Start padding
    for(i=n;i<newN;i++){
        f->data_re[i] = 0;
        f->data_im[i] = 0;
    }

    return newN;
}

/**
* Function:     fft
* Arguments:    *real               -float pointer to the real values of the sample
*               *imag               -float pointer to the imaginary values of the sample
*               n                   -int number of elements in the arrays
*               
* Returns:
* Description:  Runs FFT and stores the results in the input vectors
**/
void fft(float *real, float *imag, int n) {
    if (n <= 1) return;

    // Divide
    float *even_real = (float *)malloc(n/2 * sizeof(float));
    float *even_imag = (float *)malloc(n/2 * sizeof(float));
    float *odd_real = (float *)malloc(n/2 * sizeof(float));
    float *odd_imag = (float *)malloc(n/2 * sizeof(float));

    for (int i = 0; i < n / 2; i++) {
        even_real[i] = real[i * 2];
        even_imag[i] = imag[i * 2];
        odd_real[i] = real[i * 2 + 1];
        odd_imag[i] = imag[i * 2 + 1];
    }

    // Conquer (recursive FFT call)
    fft(even_real, even_imag, n / 2);
    fft(odd_real, odd_imag, n / 2);

    // Combine
    for (int k = 0; k < n / 2; k++) {
        float t_real = cos(-2 * PI * k / n) * odd_real[k] - sin(-2 * PI * k / n) * odd_imag[k];
        float t_imag = sin(-2 * PI * k / n) * odd_real[k] + cos(-2 * PI * k / n) * odd_imag[k];

        real[k] = even_real[k] + t_real;
        imag[k] = even_imag[k] + t_imag;
        real[k + n / 2] = even_real[k] - t_real;
        imag[k + n / 2] = even_imag[k] - t_imag;
    }

    free(even_real);
    free(even_imag);
    free(odd_real);
    free(odd_imag);
}

/**
* Function:     apply_hamming_window
* Arguments:    *real               -float pointer to the real values of the sample
*               n                   -int number of elements in the arrays
*               
* Returns:
* Description:  Applies a hamming window to the samples
**/
void apply_hamming_window(float *real, int n) {
    for (int i = 0; i < n; i++) {
        real[i] *= 0.54 - 0.46 * cos(2 * PI * i / (n - 1));
    }
}