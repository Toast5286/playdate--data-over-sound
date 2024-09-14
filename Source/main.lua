import "CoreLibs/graphics"
import "CoreLibs/object"
import "CoreLibs/keyboard"

import "fftFunctions"
import "fftVisualization"
import "fftString"

local pd <const> = playdate
local gfx <const> = pd.graphics

_G.soundEffectPlayer = pd.sound.sampleplayer.new('soundEffect.wav')

local baseFreq = 172.265625*4
local FreqArray = {baseFreq,2*baseFreq,3*baseFreq,4*baseFreq,5*baseFreq,6*baseFreq,7*baseFreq,8*baseFreq}
local SampleFreq = 44100
local samplePerChar = 1024

local processing=0


local SelectedButton = "Text" --Options: Text, Transmit, Receive 
local keyboardOut = false
local MsgString = ""

function pd.keyboard.textChangedCallback()
    MsgString = pd.keyboard.text

end

function pd.AButtonDown()
    --Write
    gfx.clear()

    if SelectedButton == "Text" then

        if keyboardOut then
            pd.keyboard.hide()
            keyboardOut = false
        else
            pd.keyboard.show("")
            keyboardOut=true
        end

    --Transmit (Encode)
    elseif SelectedButton == "Transmit" then
        --The encoder needs a sample buffer with size samplePerChar*(#MsgString+1) for all the characters and the CRC characters
        local TimeNeeded = ((#MsgString+1)*samplePerChar*2/SampleFreq)

        pd.sound.micinput.startListening()
        local buffer = pd.sound.sample.new(TimeNeeded, pd.sound.kFormat16bitMono)
        pd.sound.micinput.recordToSample(buffer, LetsEncode)

    --Receive (Decode)
    elseif SelectedButton == "Receive" then

        pd.sound.micinput.startListening()
        local buffer = pd.sound.sample.new(10, pd.sound.kFormat16bitMono)
        pd.sound.micinput.recordToSample(buffer, LetsDecode)
    end

end

--In case the user doesnt wanna wait 10 seconds to finish the recording
function pd.AButtonUp()
    if SelectedButton == "Receive" then
        pd.sound.micinput.stopRecording()
    end
end

--Change States (left Button)
function pd.leftButtonUp()

    if SelectedButton == "Receive" then
        SelectedButton = "Transmit"
    elseif SelectedButton == "Transmit" then
        SelectedButton = "Text"
    end
    
end

--Change States (right Button)
function pd.rightButtonDown()
    
    if SelectedButton == "Transmit" then
        SelectedButton = "Receive"
    elseif SelectedButton == "Text" then
        SelectedButton = "Transmit"
    end
    
end

-- Display the graphics
function pd.update()
    gfx.clear()
    pd.graphics.drawText(MsgString, 10, 10)
    pd.graphics.drawText("Mode: "..SelectedButton.."\n(press A to activate and arrows to change)", 10, 195)

    if processing == 0 then
        processing = 1
        pd.sound.micinput.startListening()
        local buffer = pd.sound.sample.new(0.1, pd.sound.kFormat16bitMono)
        pd.sound.micinput.recordToSample(buffer, FFTProcess)
    end

end

--Run Encoder
function LetsEncode(recording)
    pd.sound.micinput.stopListening()
    if recording == nil then
        return
    end

    encodeString(recording,MsgString,FreqArray,2000,samplePerChar)
end

--Run Syncronizer and Decoder
function LetsDecode(recording)
    pd.sound.micinput.stopListening()
    if recording == nil then
        return
    end

    local startTime = pd.getCurrentTimeMilliseconds()

    local SampleObj = samplelib.samples.new(recording)
    local FFTObj = fftlib.fft.new(SampleObj,0,1024/4)
    fftlib.fft.runFFT(FFTObj)

    local endTime = pd.getCurrentTimeMilliseconds()
    print(endTime-startTime)

    --[[
    local start,max = Sync(recording,3000,FreqArray,samplePerChar)
    local startInSeconds = SampleToTime(start,SampleFreq)
    if start >= 0 then
        --spectrogram(recording,20,20,360,160,2500,samplePerChar/8,FreqArray[1],FreqArray[#FreqArray],startInSeconds,-1)
        MsgString = decodeString(recording,start,3000,FreqArray,samplePerChar)
        if MsgString == "" then
            MsgString = "Couldn't understand the message,\ncan you repeat it?"
        end
    else
        MsgString = "Couldn't find the start of the message,\ncan you repeat it?"
    end]]
end


function FFTProcess(recording)
    pd.sound.micinput.stopListening()
    if recording == nil then
        return
    end

    local startTime = pd.getCurrentTimeMilliseconds()

    local SampleObj = samplelib.samples.new(recording)
    local FFTObj = fftlib.fft.new(SampleObj,0,1024/4)
    fftlib.fft.runFFT(FFTObj)
    fftlib.fft.free(FFTObj)
    local endTime = pd.getCurrentTimeMilliseconds()
    print(endTime-startTime)
    processing = 0
end