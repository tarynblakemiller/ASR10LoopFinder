/*
 ==============================================================================
 
 This file contains the basic framework code for a JUCE plugin processor.
 
 ==============================================================================
 */

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ASR10LoopFinderAudioProcessor::ASR10LoopFinderAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
: AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
                  .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
#endif
                  .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
                  )
#endif
{
    //initialize the sample buffer with 2 channels (stereo), 0 samples (empty)
    sampleBuffer.setSize(2, 0);
    sampleLoaded = false;
    
}



ASR10LoopFinderAudioProcessor::~ASR10LoopFinderAudioProcessor()
{
}

//==============================================================================
void ASR10LoopFinderAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    DBG("prepareToPlay start: playPosition=" + juce::String(playPosition));
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    this->sampleRate = sampleRate;
    this->samplesPerBlock = samplesPerBlock;
    loadSample(juce::File("/Users/tarynblakemiller/Desktop/___2025_BREAKS/TS_BD_116_can_break_brushes.wav"));
    DBG("prepareToPlay start: playPosition=" + juce::String(playPosition));
}

void ASR10LoopFinderAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ASR10LoopFinderAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo() ||
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}
#endif

void ASR10LoopFinderAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    int numSamples = 0;
    int sampleLength = 0;
    int loopStart = 0;
    int loopEnd = 0;
    int loopLength = 0;

    if (sampleLoaded)
    {
        numSamples = buffer.getNumSamples();
        sampleLength = sampleBuffer.getNumSamples();
        loopStart = static_cast<int>(0.25 * sampleLength);
        loopEnd = static_cast<int>(0.99 * sampleLength);
        loopLength = loopEnd - loopStart;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            float* channelData = buffer.getWritePointer(channel);
            const float* sampleData = sampleBuffer.getReadPointer(channel % 2);
            for (int sample = 0; sample < numSamples; ++sample)
            {
                int pos = (playPosition % loopLength) + loopStart;
                channelData[sample] = sampleData[pos];
                playPosition++;
                if (playPosition >= loopLength) playPosition = 0;
            }
        }
    }
}

void ASR10LoopFinderAudioProcessor::loadSample(const juce::File& file)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader != nullptr)
    {
        int numSamples = static_cast<int>(reader->lengthInSamples);
        sampleBuffer.setSize(2, numSamples); //resize the buffer for stereo
        reader->read(&sampleBuffer, 0, numSamples, 0, true, true); //read into buffer
        sampleLoaded = true;
//        findLoopPoints(); //sets loop points after loading
    }
    else
    {
        sampleLoaded = false;
    }
}

//void ASR10LoopFinderAudioProcessor::findLoopPoints()
//{
//    DBG("Entering findLoopPoints");
//    if(!sampleLoaded || sampleBuffer.getNumSamples() == 0)
//    {
//        DBG("No sample loaded or empty buffer");
//        return;
//    }
//
//    int sampleLength = sampleBuffer.getNumSamples();
//    loopStartSample = static_cast<int>(0.25 * sampleLength); //25%
//    loopEndSample = loopStartSample; //initially same as start
//
//    const float* leftChannel = sampleBuffer.getReadPointer(0); //left channel
//    const float* rightChannel = sampleBuffer.getReadPointer(1);
//    int searchLimit = sampleLength - 1;
//
//    //find first zero crossing at 25%
//    DBG("Searching for start, initial loopStartSample: " + juce::String(loopStartSample));
//    while(loopStartSample < searchLimit && (leftChannel[loopStartSample] * leftChannel[loopStartSample + 1] > 0 || rightChannel[loopStartSample] * rightChannel[loopStartSample + 1] > 0))
//    {
//        loopStartSample++;
//    }
//
//    //initial end to start
//    loopEndSample = loopStartSample;
//    //next zero crossing for single-cycle
//    int minCycleLength = static_cast<int>(sampleRate / 1000.0);  // Min 1 kHz cycle (~44 samples at 44.1 kHz)
//    int maxCycleLength = static_cast<int>(sampleRate / 50.0);    // Max 50 Hz cycle (~882 samples)
//    int targetEnd = loopStartSample + maxCycleLength;  // Aim for longer loop
//    if (targetEnd > searchLimit) targetEnd = searchLimit;
//
//    DBG("Searching for end, targetEnd: " + juce::String(targetEnd));
//    loopEndSample = targetEnd;
//    while (loopEndSample > loopStartSample + minCycleLength &&
//           (leftChannel[loopEndSample] * leftChannel[loopEndSample + 1] > 0 ||
//            rightChannel[loopEndSample] * rightChannel[loopEndSample + 1] > 0))
//    {
//        loopEndSample--;
//    }
//
//    // Ensure loop end is valid
//    if (loopEndSample - loopStartSample < minCycleLength)
//    {
//        loopEndSample = loopStartSample + minCycleLength;
//        if (loopEndSample > sampleLength - 1) loopEndSample = sampleLength - 1;
//        DBG("Adjusted to minCycleLength");
//    }
//    DBG("loopStartSample: " + juce::String(loopStartSample) +
//        ", loopEndSample: " + juce::String(loopEndSample) +
//        ", loopLength: " + juce::String(loopEndSample - loopStartSample));
//}


//==============================================================================
//bool ASR10LoopFinderAudioProcessor::hasEditor() const
//{
//    return true; // (change this to false if you choose to not supply an editor)
//}

juce::AudioProcessorEditor* ASR10LoopFinderAudioProcessor::createEditor()
{
    return new ASR10LoopFinderAudioProcessorEditor (*this);
}

//==============================================================================
void ASR10LoopFinderAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void ASR10LoopFinderAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ASR10LoopFinderAudioProcessor();
}
