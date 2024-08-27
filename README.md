
# playdate--data-over-sound

Implementation of Data over sound using the playdate console.

This project was inspired by [SquidGod](https://www.youtube.com/@SquidGodDev).


## How to run (Windows)

Open the X64 Native Tools Command Prompt for VS 2019, navigate into the build directory using:

```bash
  cd /[path]/build
```
and run:

```bash
  nmake
```
There should be a file called DataOverSound.pdx. Open the playdate simulator and drag and drop the DataOverSound_V2.pdx in to it.

If there was a error previously or you're using a difrent OS, please compile using the options on the [Inside Playdate with C](https://sdk.play.date/2.5.0/Inside%20Playdate%20with%20C.html#_building_for_the_simulator_using_nmake).


## File organization

All .c and .h files are in the /src directory and all lua files are in /Source directory.

## How does it work

This Demo uses amplitude in certain frequenies to convay the bits of a character. It adds a extra character (Cycle Redundancy Check byte) for bit parity check (error detection).

The receiver tries listening to the frequenies that were used for encoding and extracts the bit information.

## Fast Fourier Transform

For the FFT to give a good aproximation, it uses a hamming window before running the FFT. It uses Cooley-Tukey radix-2 FFT algorithm that does need the number of samples to be a power of 2. To resolve this, it applies padding before running the algorithm.

## Visualization Functions

For better understanding of the encoding process or if you just wanna play with the frequency domain, there are 3 functions in the fftVisualization.lua file.

- FFTAbsGraph - Shows a graphical representation of the frequency domain of a playdate.sound.sample object.

- spectrogram - Shows the frequency domain over time (only over a threshold) of a playdate.sound.sample object.

- VisualizeSamples - Shows the samples over time of a playdate.sound.sample object.

## samplelib.samples object (samples.c)

This object contains the information needed for sampling. It does not contain its own information, as it only access the information stored in a playdate.sound.sample object.

Must be freed after being used or risk memory leaks.

## fftlib.fft object (fft.c)

This object contains the information needed to run the FFT. It does contain a copy of it's samples and modifies them to the frequeny domain.

Must be freed after being used or risk memory leaks.

## Author

- [@Toast5286](https://github.com/Toast5286)


[![MIT License](https://img.shields.io/badge/License-MIT-green.svg)](https://choosealicense.com/licenses/mit/)


