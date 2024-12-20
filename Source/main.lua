import "CoreLibs/graphics"
import "CoreLibs/object"
import "CoreLibs/keyboard"

import "fftFunctions"
import "fftVisualization"
import "fftString"

local pd <const> = playdate
local gfx <const> = pd.graphics

_G.soundEffectPlayer = pd.sound.sampleplayer.new('soundEffect.wav')

local baseFreq = 344.53125
local FreqArray = {{baseFreq,4*baseFreq},{7*baseFreq,10*baseFreq},{13*baseFreq,16*baseFreq},{19*baseFreq,22*baseFreq},{25*baseFreq,28*baseFreq},{31*baseFreq,34*baseFreq},{37*baseFreq,40*baseFreq},{43*baseFreq,46*baseFreq}}
local SampleFreq = 44100
local samplePerChar = 1470*2.5

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
        local TimeNeeded = ((#MsgString+1)*samplePerChar*2/SampleFreq)+0.01
        
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
    if SelectedButton == "Receive" then
        if processing == 0 then
            processing = 1
            pd.sound.micinput.stopListening()
            pd.sound.micinput.startListening()
            local buffer = pd.sound.sample.new(0.01, pd.sound.kFormat16bitMono)
            pd.sound.micinput.recordToSample(buffer, LetsDecode)
        end
    end

    pd.graphics.drawText(MsgString, 10, 10)
    pd.graphics.drawText("Mode: "..SelectedButton.."\n(press A to activate and arrows to change)", 10, 195)


end

--Run Encoder
function LetsEncode(recording)
    pd.sound.micinput.stopListening()
    if recording == nil then
        return
    end

    encodeString(recording,MsgString,FreqArray,600,samplePerChar)
end

--Run Syncronizer and Decoder
function LetsDecode(recording)
    pd.sound.micinput.stopListening()
    if recording == nil then
        return
    end

    local startTime = pd.getCurrentTimeMilliseconds()
    local Msg = decodeString(recording,10,FreqArray,"")

    local endTime = pd.getCurrentTimeMilliseconds()
    --print(endTime-startTime)
    MsgString = Msg
    print(Msg)
    processing = 0
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

    FFTAbsGraph(recording,20,20,360,160,0,0,20000)

    local endTime = pd.getCurrentTimeMilliseconds()
    print(endTime-startTime)
end