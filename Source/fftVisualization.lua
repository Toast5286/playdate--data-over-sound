import "CoreLibs/graphics"
import "CoreLibs/object"
import "main"

import "fftFunctions"

local pd <const> = playdate
local gfx <const> = pd.graphics

--[[
**
* Function:     FFTAbsGraph
* Arguments:    sample                  -playdate.sound.sample that we want to analyse
*               x                       -The top left corner x coordenate 
*               y                       -The top left corner y coordenate 
*               w                       -The width of the graph
*               h                       -The hight of the graph
*               startSample             -Sample to start analysing
*               StartFreq               -Minimum frequency to display (Hz)
*               EndFreq                 -Maximum Frequency to display (Hz)
*               
* Returns:
* Description:  Displays a graph showing the results from the FFT of the sound sample
**]]
function FFTAbsGraph(sample,x,y,w,h,startSample,StartFreq,EndFreq)
    --Get samples
    local SampleObj = samplelib.samples.new(sample)
    local length = samplelib.samples.getLength(SampleObj)


    --Setup and Run FFT
    local FFTObj = fftlib.fft.new(SampleObj,startSample,startSample+1024)
    fftlib.fft.runFFT(FFTObj)

    --Get Usefull information
    local sampleSize = fftlib.fft.getLength(FFTObj)                             --Size of the fft after padding
    local StartIdx = 0                                                          --default
    local EndIdx = sampleSize/2                                                 --We use sampleSize/2, since the second half of the frequency spectrum is symmetrical to the first half
    if StartFreq>0 and StartFreq<44100 and EndFreq>0 and EndFreq<44100 then
        StartIdx = FreqToIdx(StartFreq,44100,sampleSize)                        --Get start frequency index
        EndIdx = FreqToIdx(EndFreq,44100,sampleSize)                            --Get end frequency index
    end
    local max,maxIdx = fftlib.fft.getAbsMax(FFTObj,StartIdx,EndIdx)             --Max magnitude in the spectrum between the StartIdx and EndIdx
    if maxIdx < 0 then
        print("Error in FFTAbsGraph function: Could not read AbsMax")
        return -1
    end

    --Check the amplitude of each frequency
    for i=StartIdx,EndIdx do
        local absVal = (fftlib.fft.getAbsFreq(FFTObj,i))/max    --Relative magnitude
        if absVal > 0 then
            gfx.fillRect(x+math.floor((i-StartIdx)*w/(EndIdx-StartIdx)),y+h-math.min(h,h*absVal),math.ceil(w/(EndIdx-StartIdx)),math.min(h,h*absVal))
        end
    end

    --Clean up
    samplelib.samples.free(SampleObj)
    fftlib.fft.free(FFTObj)
    
end

--[[
**
* Function:     spectrogram
* Arguments:    sample                  -playdate.sound.sample that we want to analyse
*               x                       -The top left corner x coordenate 
*               y                       -The top left corner y coordenate 
*               w                       -The width of the graph
*               h                       -The hight of the graph
*               threshold               -Minimum amplitude needed to turn on the pixel correspondig to the frequency
*               sampleSize              -Number of samples to do 1 FFT
*               StartFreq               -Minimum frequency to display (Hz)
*               EndFreq                 -Maximum Frequency to display (Hz)
*               StartTime               -Time on the recording to start the spectrogram (in seconds)
*               EndTime                 -Time on the recording to end the spectrogram   (in seconds)
*               
* Returns:
* Description:  Displays a graph showing the frequencies abouve the threshold (obtained from the FFT) of the sound sample over time between the given limits (in frequency and time)
**]]
function spectrogram(sample,x,y,w,h,threshold,sampleSize,StartFreq,EndFreq,StartTime,EndTime)
    gfx.clear()
    --Get Samples
    local SampleObj = samplelib.samples.new(sample)

    -- Get frequency limits
    local StartIdx = 0                                         --default
    local EndIdx = sampleSize/2                                --We use sampleSize/2, since the second half of the frequency spectrum is symmetrical to the first half

    --Get time limits
    local StartSample = TimeToSamples(StartTime,44100)
    local EndSample = TimeToSamples(EndTime,44100)
    local SampleLength = samplelib.samples.getLength(SampleObj)
    if StartSample > SampleLength or StartSample < 0 then
        StartSample=0
    end
    if EndSample > SampleLength or EndSample < 0 then
        EndSample=SampleLength
    end

    --Calculate the amount of segments that will need to run the fft
    local length = math.floor((EndSample-StartSample)/sampleSize)
    if length<0 then
        print("Error: Could not read SampleObj to get length")
        return 
    end

    for i=0,(length-1) do
        --Run FFT for each segment we need
        local FFTObj = fftlib.fft.new(SampleObj,i*sampleSize+StartSample,(i+1)*sampleSize+StartSample)
        fftlib.fft.runFFT(FFTObj)

        --Update the SartIdx and EndIdx based on the sampleSize
        sampleSize = fftlib.fft.getLength(FFTObj)                                   --SampleSize can varie due to the padding used in fft
        if StartFreq>0 and StartFreq<44100 and EndFreq>0 and EndFreq<44100 then
            StartIdx = FreqToIdx(StartFreq,44100,sampleSize)                        --Get start frequency index
            EndIdx = FreqToIdx(EndFreq,44100,sampleSize)                            --Get end frequency index
        end

        for j=StartIdx,EndIdx do
            local absVal = fftlib.fft.getAbsFreq(FFTObj,j)  --Get the magnitude
            --Plot
            if absVal > threshold then  
                gfx.fillRect(x+math.floor(i*w/length),y+h-math.floor(h*(j-StartIdx)/(EndIdx-StartIdx)),math.ceil(w/length),math.ceil(h/(EndIdx-StartIdx)))
            end
        end
        --Free FFT memory
        fftlib.fft.free(FFTObj)
    end
    --Free samples
    samplelib.samples.free(SampleObj)

end

--[[
**
* Function:     VisualizeSamples
* Arguments:    sample                  -playdate.sound.sample that we want to analyse
*               x                       -The top left corner x coordenate 
*               y                       -The top left corner y coordenate 
*               w                       -The width of the graph
*               h                       -The hight of the graph
*               MaxPressure             -Max amplitude of the wave
*               StartIdx               -Time on the recording to start the visualization (in samples)
*               EndIdx                 -Time on the recording to end the visualization   (in samples)
*               
* Returns:
* Description:  Displays a graph showing the samples stored in sample
**]]
function VisualizeSamples(sample,x,y,w,h,MaxPressure,StartIdx,EndIdx)
    --This function uses samples instead of time, because we can only visualize a couple of samples
    --Seconds is too big of a unit

    gfx.clear()
    --Get Samples
    local SampleObj = samplelib.samples.new(sample)
    local length = EndIdx-StartIdx      --Calculate the amount of samples to draw

    local lastAmp=0
    for i=StartIdx,EndIdx do
        --For each sample, draw the respective point and connect it with the previous point
        Amp = samplelib.samples.getSample(SampleObj,i)
        gfx.fillRect(x+math.floor(i*w/length),y+(h/2)-math.min((lastAmp*h/MaxPressure),(Amp*h/MaxPressure)),math.ceil(w/MaxPressure),math.max((lastAmp*h/MaxPressure)-(Amp*h/MaxPressure)+1,(Amp*h/MaxPressure)-(lastAmp*h/MaxPressure)))
        lastAmp=Amp
    end

    --Free samples
    samplelib.samples.free(SampleObj)
end