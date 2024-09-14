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
    local SampleFreq = samplelib.samples.getSampleFreq(SampleObj)    --Samples per second
    samplelib.samples.syntheticData(SampleObj,-1,0,0,-1)    --Clear out old samples

    --Make sure we have enogh samples
    local length = samplelib.samples.getLength(SampleObj)
    if length<0 then
        print("Error: Could not read SampleObj to get length")
        return 
    end
    if length<(nChar*samplePerChar*2) then
        print("Error: sample needs at least ",(nChar*samplePerChar*2/SampleFreq),". ",length/SampleFreq,"seconds where given.")
        return 
    end

    --Start Encoding
    local AsciiInt = 0
    local BitArray = {}
    for i = 1, nChar do
        AsciiInt = string.byte(str,i)--We Encoding for each character 
        BitArray = {}
        --Check bit by bit ans write it in the BitArray
        for j = 8,1,-1 do
            if (AsciiInt%2==1)then
                BitArray[j]=1
            else
                BitArray[j]=0
            end
            AsciiInt>>=1
        end

        --Encode character
        encodeByte(SampleObj,FreqArray,BitArray,Amp,(2*i-1)*samplePerChar,2*i*samplePerChar)

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
function decodeString(sample,StartSample,threshold,FreqArray,samplePerChar)

    local SampleObj = samplelib.samples.new(sample)
    local length = samplelib.samples.getLength(SampleObj)
    local SampleFreq = samplelib.samples.getSampleFreq(SampleObj)

    local BitArray = {}
    local AsciiInt = 0
    local strArray = {}

    --Decoding char in msg
    for i=StartSample,length,samplePerChar*2 do
        local BitArray = {}
        --Decode 1 byte
        AsciiInt = 0
        BitArray = decodeByte(SampleObj,FreqArray,threshold,i,i+samplePerChar/2)

        for j=1,#BitArray do
            AsciiInt<<=1
            if BitArray[j]==1 then
                AsciiInt+=1
            end
        end


        --Check if byte isnt "/0", meaning end of string
        if AsciiInt==0 then
            break
        end
        strArray[#strArray+1] = AsciiInt    --Add to array

    end
    samplelib.samples.free(SampleObj)
    --Writing down in string form
    --Since ""already includes /0, we need to append from the beginning of the string,
    --so we start from the end and work our way to the first character
    local str = ""
    for k=(#strArray-1),1,-1 do
        str = string.char(strArray[k])..str
    end
    
    --Check if the bits were correctly received
    if CRCDecoder(str,strArray[#strArray]) == 1 then
        print("Error in decodeString function: CRC was not correct.")
        return ""
    end

    return str
end