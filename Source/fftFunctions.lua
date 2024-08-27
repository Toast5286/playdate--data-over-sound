import "CoreLibs/graphics"
import "CoreLibs/object"
import "main"

local pd <const> = playdate
local gfx <const> = pd.graphics
--[[
**
* Function:     FreqToIdx
* Arguments:    Freq                -Frequency who's bin we want to find
*               SamplingFreq        -Number of samples recorded per second
*               SampleSize          -Number of samples that are gonna be used in the FFT
*
* Returns:      Idx                 -Index corresponding to the frequency bin of frequency Freq
* Description:  Finds the Bin in the FFT corresponding to the frequency Freq
**]]
function FreqToIdx(Freq,SamplingFreq,SampleSize)
    local Idx = Freq*(SampleSize/SamplingFreq)

    if Idx - math.floor(Idx) > math.ceil(Idx)-Idx then
        return math.ceil(Idx)
    end
        return math.floor(Idx)
end

--[[
**
* Function:     IdxToFreq
* Arguments:    Idx                 -Bin index who's center frequency we want to find 
*               SamplingFreq        -Number of samples recorded per second
*               SampleSize          -Number of samples that are gonna be used in the FFT
*
* Returns:      Freq                 -The central frequency
* Description:  Finds the center frequency to the input Bin index of a FFT
**]]
function IdxToFreq(Idx,SamplingFreq,SampleSize)
    
    return Idx*(SamplingFreq/SampleSize)
    
end

--[[
**
* Function:     TimeToSamples
* Arguments:    Time                -Time in seconds we want to convert 
*               SamplingFreq        -Number of samples recorded per second
*
* Returns:      Samples             -Coresponding Sample from the recording
* Description:  Finds the Sample for that specific time
**]]
function TimeToSamples(Time,SamplingFreq)
    return Time*SamplingFreq
end

--[[
**
* Function:     SampleToTime
* Arguments:    Sample              -Sample we want to convert 
*               SamplingFreq        -Number of samples recorded per second
*
* Returns:      Time                -Coresponding Time from the recording
* Description:  Finds the time in seconds for that specific sample
**]]
function SampleToTime(Sample,SamplingFreq)
    return Sample/SamplingFreq 
end

--[[
**
* Function:     encodeByte
* Arguments:    SampleObj           -Sample object from samplelib.samples to store the data
*               FreqArray           -Array containing the frequencies used to encode each bit
*               BitArray            -Array containing 0 and 1 that represent the bits we want to encode
*               Amp                 -Amplitude for the sinusoidal wave that will encode the bits
*               StartSample         -Sample to start the sinusoidal wave
*               EndSample           -Sample to end the sinusoidal wave
*
* Returns:      
* Description:  Encodes the bits in BitArray in the frequencies in FreqArray with amplitude Amp between StartSample and EndSample in to the SampleObj
**]]
function encodeByte(SampleObj,FreqArray,BitArray,Amp,StartSample,EndSample)
    local NumFreq = #FreqArray
    local NumBits = #BitArray

    --Check if the number of bits is the same as the number of frequencies
    if NumFreq > NumBits then
        print("Error in function encodeByte: More frequencies(",NumFreq,") then bits(",NumBits,")")
        return
    elseif NumFreq < NumBits  then
        print("Error in function encodeByte: More bits(",NumBits,") then frequencies(",NumFreq,")")
        return
    end

    --Apply a sinusoidal wave to represent the bit i with Frequency i
    for i = 1,NumBits do
        if BitArray[i] == 1 then
            samplelib.samples.syntheticData(SampleObj,FreqArray[i],Amp,StartSample,EndSample) 
        end
    end

end

--[[
**
* Function:     decodeByte
* Arguments:    SampleObj           -Sample object from samplelib.samples to search the data
*               FreqArray           -Array containing the frequencies used to encode each bit
*               threshold           -Minimum amplitude needed for a bit to be considered a 1
*               StartSample         -Sample to start the FFT
*               EndSample           -Sample to end the FFT
*
* Returns:      BitArray            -Array containing the decoded bits
* Description:  Decodes the bits in SampleObj on the frequencies of FreqArray between StartSample and EndSample
**]]
function decodeByte(SampleObj,FreqArray,threshold,StartSample,EndSample)
    local mag
    local SampleFreq = 44100

    local BitArray = {}

    --Initialize FFT
    local FFTObj = fftlib.fft.new(SampleObj,StartSample,EndSample)
    fftlib.fft.runFFT(FFTObj)
    local SampleSize = fftlib.fft.getLength(FFTObj)

    --Decoding bit in each frequency
    for i =1,#FreqArray do
        mag = fftlib.fft.getAbsFreq(FFTObj,FreqToIdx(FreqArray[i],SampleFreq,SampleSize))  --Get the magnitude
        if mag > threshold then
            BitArray[i] = 1
        else
            BitArray[i] = 0
        end

    end
    fftlib.fft.free(FFTObj)
    return BitArray
end

--[[
**
* Function:     Sync
* Arguments:    sample              -playdate.sound.sample that contains the string
*               threshold           -Minimum amplitude needed for a bit to be considered a 1
*               FreqArray           -Array containing the frequencies used to encode each bit
*               samplePerChar       -Number of Samples to encode 1 character
*
* Returns:      StartSampleAprox    -Aproximatly the sample that the message starts
*               mag                 -The magnitude of one of the frequencies in FreqArray that was detected
* Description:  Linearly searches threw the samples in the frequencies in FreqArray and returns the sample that it thinks is the start of the message
**]]
function Sync(sample,threshold,FreqArray,samplePerChar)

    local SampleObj = samplelib.samples.new(sample)
    local length = samplelib.samples.getLength(SampleObj)
    if length<0 then
        print("Error in Sync function: Could not read length from SampleObj")
        return 
    end

    local SampleFreq = 44100                    --Samples per second
    local ScanningSampleSize = samplePerChar/2  --We'll scan half with half of the samplePerChar to not skip the actull frequencies

    local mag = 0
    local StartSampleAprox = -1

    --Find the first possibility of it being the starting signal
    for i = 0,length-ScanningSampleSize,ScanningSampleSize do
        mag = 0
        local FFTObj = fftlib.fft.new(SampleObj,i,i+ScanningSampleSize)
        fftlib.fft.runFFT(FFTObj)

        for j=1,#FreqArray do
            mag = fftlib.fft.getAbsFreq(FFTObj,FreqToIdx(FreqArray[j],SampleFreq,ScanningSampleSize))  --Get the magnitude
            if mag > threshold then
                StartSampleAprox = i
                break
            end
        end
        if mag > threshold then
            fftlib.fft.free(FFTObj)
            break
        end
        fftlib.fft.free(FFTObj)
    end

    samplelib.samples.free(SampleObj)
    return StartSampleAprox,mag
    
end
--[[
**
* Function:     CRCEncoder          (Cycle Redundancy Check)
* Arguments:    str                 -String to create a Cycle Redundancy Check byte
*               
* Returns:      str + CRC           -The original string with the CRC byte (as a character) appended
* Description:  Creates a Cycle Redundancy Check for a string and appends it to the end
**]]
function CRCEncoder(str)
    --CycleRedundancyCheck to make sure there are no bit errors in the transmission
    local CRC = 0
    for i=1,#str do
        CRC = CRC ~ string.byte(str,i)
    end

    return str .. string.char( CRC )
    
end

--[[
**
* Function:     CRCDecoder          (Cycle Redundancy Check)
* Arguments:    str                 -String to check for bit errors
*               CRC                 -Cycle Redundancy Check byte as a number (not a char)
*               
* Returns:      0                   -if it thinks there is no bit errors
*               1                   -if it thinks theres a bit error
* Description:  Checks for bit errors using a Cycle Redundancy Check
**]]
function CRCDecoder(str,CRC)
    --CycleRedundancyCheck to make sure there are no bit errors in the reception
    local PossibleCRC = 0

    for i=1,#str do
        PossibleCRC = PossibleCRC ~ string.byte(str,i)
    end

    if PossibleCRC == CRC then
        return 0
    else
        return 1
    end
end