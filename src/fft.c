#include "samples.h"
#include "fft.h"

#define PI 3.1415926535897932384626433832795028841971f
#define FIXED_SHIFT 16  // Fixed-point shift (16-bit fractional part)
#define FIXED_SCALE (1 << FIXED_SHIFT)  // 2^16 for scaling float to fixed-point

static PlaydateAPI* pd = NULL;

//FFT Struture and functions ---------------------------------

typedef struct
{
	int32_t *data_re;
    int32_t *data_im;
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
void fft_fixed_iterative(int32_t *real, int32_t *imag, uint32_t n);
void apply_hamming_window(int32_t *real, int n);

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

    f->data_re = pd->system->realloc(NULL, (sizeof(int32_t )* size));
    f->data_im = pd->system->realloc(NULL, (sizeof(int32_t ) * size));

    int i=0,j=0;

    for(i=startIdx;i<endIdx;i++){
        f->data_re[j] = ((int32_t)s->data[i]);
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
    fft_fixed_iterative(f->data_re,f->data_im,n);

    
    //Normalizing amplitude to be the same as the input signal
    f->data_re[0] =(int32_t) (f->data_re[0]*1.87f/n);
    f->data_im[0] =(int32_t) (f->data_im[0]*1.87f/n);
    for(int i=1;i<n;i++){
        f->data_re[i] =(int32_t) (f->data_re[i] * 3.74f/(n));
        f->data_im[i] =(int32_t) (f->data_im[i] * 3.74f/(n));
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

    float absVal = sqrtf(powf((float)f->data_re[idx],2) + powf((float)f->data_im[idx],2));
    
    pd->lua->pushFloat(absVal);

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

    float phaseVal = atan2f((float)f->data_im[idx],(float)f->data_re[idx]);

    pd->lua->pushFloat(phaseVal);

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

    float Max = 0;
    int MaxFreq = 0;

    for(int i = startIdx;i<endIdx;i++){
        float absVal = sqrtf(powf((float)f->data_re[i],2) + powf((float)f->data_im[i],2));
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
    f->data_re = pd->system->realloc(f->data_re, (sizeof(int32_t)* newN));
    f->data_im = pd->system->realloc(f->data_im, (sizeof(int32_t) * newN));

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

// Converts a float to fixed-point
static inline int32_t float_to_fixed(float x) {
    return (int32_t)(x * FIXED_SCALE);
}

// Converts fixed-point back to float
static inline float fixed_to_float(int32_t x) {
    return (float)x / FIXED_SCALE;
}

// Fixed-point multiplication (Q16)
static inline int32_t fixed_mul(int32_t a, int32_t b) {
    return (int32_t)((int64_t)a * b >> FIXED_SHIFT);
}

// Fixed-point FFT butterfly operation
static inline void fixed_butterfly(int32_t *real1, int32_t *imag1, int32_t *real2, int32_t *imag2, int32_t wr, int32_t wi) {
    // Fixed-point complex multiplication and addition
    int32_t tr = fixed_mul(*real2, wr) - fixed_mul(*imag2, wi);
    int32_t ti = fixed_mul(*real2, wi) + fixed_mul(*imag2, wr);

    *real2 = *real1 - tr;
    *imag2 = *imag1 - ti;
    *real1 = *real1 + tr;
    *imag1 = *imag1 + ti;
}

// Bit-reversal function (unchanged)
static inline uint32_t reverse_bits(uint32_t n, uint32_t log2n) {
    uint32_t result = 0;
    for (uint32_t i = 0; i < log2n; i++) {
        result = (result << 1) | (n & 1);
        n >>= 1;
    }
    return result;
}

// Iterative Fixed-Point FFT
void fft_fixed_iterative(int32_t *real, int32_t *imag, uint32_t n) {
    uint32_t log2n = 0;
    for (uint32_t i = n; i > 1; i >>= 1) {
        log2n++;
    }

    // Bit-reversal permutation
    for (uint32_t i = 0; i < n; i++) {
        uint32_t j = reverse_bits(i, log2n);
        if (i < j) {
            // Swap real and imaginary parts
            int32_t temp_real = real[i];
            real[i] = real[j];
            real[j] = temp_real;

            int32_t temp_imag = imag[i];
            imag[i] = imag[j];
            imag[j] = temp_imag;
        }
    }

    // FFT computation
    for (uint32_t s = 1; s <= log2n; s++) {
        uint32_t m = 1 << s;
        uint32_t m2 = m >> 1;

        // Calculate fixed-point twiddle factors (precompute cos/sin)
        int32_t wm_real = float_to_fixed(cosf(-2.0f * PI / m));
        int32_t wm_imag = float_to_fixed(sinf(-2.0f * PI / m));

        for (uint32_t k = 0; k < n; k += m) {
            int32_t w_real = float_to_fixed(1.0f);
            int32_t w_imag = 0;

            // Perform FFT butterfly operations
            for (uint32_t j = 0; j < m2; j++) {
                fixed_butterfly(&real[k + j], &imag[k + j], &real[k + j + m2], &imag[k + j + m2], w_real, w_imag);

                // Update twiddle factors for the next butterfly
                int32_t temp_real = fixed_mul(w_real, wm_real) - fixed_mul(w_imag, wm_imag);
                w_imag = fixed_mul(w_real, wm_imag) + fixed_mul(w_imag, wm_real);
                w_real = temp_real;
            }
        }
    }
}

/**
* Function:     apply_hamming_window
* Arguments:    *real               -float pointer to the real values of the sample
*               n                   -int number of elements in the arrays
*               
* Returns:
* Description:  Applies a hamming window to the samples
**/
void apply_hamming_window(int32_t  *real, int n) {
    for (int i = 0; i < n; i++) {
        real[i] =(uint32_t) (real[i] * (0.54f - 0.46f * cosf(2 * PI * i / (n - 1))));
    }
}