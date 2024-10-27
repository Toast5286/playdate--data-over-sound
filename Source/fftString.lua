import "CoreLibs/graphics"
import "CoreLibs/object"
import "main"

import "fftFunctions"
import "fftVisualization"
--[[
**
* Function:     encodeString
* Arguments:    SampleBuffer        -playdate.sound.sample to encode the message to
*               str                 -String to Encode
*               FreqArray           -Array containing the Frequencies used for encoding (for char the minimum size is 8)
*               Amp                 -Amplitude of each bit that represents a 1
*               samplePerChar       -Number of Samples to encode 1 character
*
* Returns: 
* Description:  Encodes string in to a sound sample and plays it
**]]
function encodeString(SampleBuffer,str,FreqArray,Amp,samplePerChar)

    str = CRCEncoder(str)

    local nChar = #str          --Num of characters

    --Create the SampleObj to manipulate the samples
    local SampleObj = samplelib.samples.new(SampleBuffer)
    local SampleFreq = 44100 --samplelib.samples.getSampleFreq(SampleObj)    --Samples per second
    samplelib.samples.syntheticData(SampleObj,-1,0,0,-1)    --Clear out old samples

    --Make sure we have enogh samples
    local length = samplelib.samples.getLength(SampleObj)
    if length<0 then
        print("Error: Could not read SampleObj to get length")
        return 
    end
    if length<(nChar*samplePerChar) then
        print("Error: sample needs at least ",(nChar*samplePerChar/SampleFreq),". ",length/SampleFreq,"seconds where given.")
        return 
    end

    --Start Encoding
    local AsciiInt = 0
    local BitArray = {}
    for i = 1, nChar do
        AsciiInt = string.byte(str,i)--We Encoding for each character 
        BitArray = {}
        --Check bit by bit and write it in the BitArray
        for j = 8,1,-1 do
            if (AsciiInt%2==1)then
                BitArray[j]=1
            else
                BitArray[j]=0
            end
            AsciiInt>>=1
        end

        --Encode character
        encodeByte(SampleObj,FreqArray,BitArray,Amp,(i-1)*samplePerChar,i*samplePerChar)

    end
    --Visualization
    --spectrogram(SampleBuffer,20,20,360,200,Amp,samplePerChar/4,FreqArray[1],FreqArray[#FreqArray],-1,nChar*samplePerChar*2/SampleFreq)

    --Copy to soundEffectPlayer
    soundEffectPlayer:setSample(SampleBuffer)
    soundEffectPlayer:play()
    samplelib.samples.free(SampleObj)
end

--[[
**
* Function:     decodeString
* Arguments:    sample              -playdate.sound.sample that contains the string
*               StartSample         -Sample where the string starts (use Sync function to find this)
*               threshold           -Minimum amplitude needed for a bit to be considered a 1
*               FreqArray           -Array containing the Frequencies used for encoding (for char the minimum size is 8)
*               samplePerChar       -Number of Samples to encode 1 character
*
* Returns:      str                 -String containing the decoded message or "" if message had 1 or more errors
* Description:  Decodes sound sample in to a string and checks if there was a error while receiving
**]]
function decodeString(sample,threshold,FreqArray,PrevStr)

    local SampleObj = samplelib.samples.new(sample)
    local length = samplelib.samples.getLength(SampleObj)
    local SampleFreq = 44100

    local BitArray = {}
    local AsciiInt = 0

    --Decoding char in msg
    --Decode 1 byte
    BitArray = decodeByte(SampleObj,FreqArray,threshold,0,128)
    samplelib.samples.free(SampleObj)
    --BitArray to String
    for j=1,#BitArray do
        AsciiInt<<=1
        if BitArray[j]<0 then
            print(BitArray[j])
            return PrevStr
        elseif BitArray[j]==1 then
            AsciiInt+=1
        end
    end

    --Check if byte isnt "/0", meaning end of string
    if AsciiInt==0 then
        return PrevStr
    end
    
    str = PrevStr..string.char(AsciiInt)
    --TODO: Error correction code here

    return str
end