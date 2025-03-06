/*
 ==============================================================================
 
 This file contains the basic framework code for a JUCE plugin processor.
 
 ==============================================================================
 */


#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <juce_core/juce_core.h>
using namespace juce;

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
    formatManager.registerBasicFormats();
    //initialize the sample buffer with 2 channels (stereo), 0 samples (empty)
    //        sampleBuffer.setSize(2, 0);
    //    sampleLoaded = false;
    
}



ASR10LoopFinderAudioProcessor::~ASR10LoopFinderAudioProcessor()
{
}

//==============================================================================
void ASR10LoopFinderAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    DBG("prepareToPlay start: playPosition=" + juce::String(playhead));
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    this->sampleRate = sampleRate;
//    this->samplesPerBlock = samplesPerBlock;
    juce::File sampleFile("/Users/tarynblakemiller/Desktop/___2025_BREAKS/TS_BD_116_can_break_brushes.wav");
    
    if (sampleFile.existsAsFile()) {
        loadSample(sampleFile);
        DBG("Sample loaded from: " + sampleFile.getFullPathName());
    } else {
        DBG("Sample file not found: " + sampleFile.getFullPathName());
        // Maybe show a file chooser dialog here or load a default sample
    }
//    loadSample(juce::File("/Users/tarynblakemiller/Desktop/___2025_BREAKS/TS_BD_116_can_break_brushes.wav"));
//    DBG("prepareToPlay start: playPosition=" + juce::String(playhead));
    if (sampleLoaded)
    {
        findLoopPoints(); // Recompute loop points if sample rate changes
        thumbnail.setSource(new juce::FileInputSource(juce::File("/Users/tarynblakemiller/Desktop/___2025_BREAKS/TS_BD_116_can_break_brushes.wav")));
    }
}

void ASR10LoopFinderAudioProcessor::releaseResources()
{
    thumbnail.setSource(nullptr);
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
    
    juce::ScopedNoDenormals noDenormals; //utility to prevent denormal numbers
    
    if (!sampleLoaded || sampleBuffer.getNumSamples() == 0 || loopStartSample == -1 || loopEndSample == -1 || loopStartSample >= loopEndSample)
    {
        buffer.clear(); //no valid sample - output silence
        return;
    }
    
    //    int numSamples = 0;
    //    int sampleLength = 0;
    //    int loopStart = 0;
    //    int loopEnd = 0;
    
    
    //    if (sampleLoaded)
    //    {
    //        numSamples = buffer.getNumSamples();
    //        sampleLength = sampleBuffer.getNumSamples();
    //        loopStart = static_cast<int>(0.25 * sampleLength);
    //        loopEnd = static_cast<int>(0.99 * sampleLength);
    int loopLength = loopEndSample - loopStartSample;
    static int playhead = 0;
    
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        float* channelData = buffer.getWritePointer(channel);
        const float* sampleData = sampleBuffer.getReadPointer(channel % sampleBuffer.getNumChannels());
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            int sampleIndex = loopStartSample + (playhead + i) % loopLength;
            channelData[i] = sampleData[sampleIndex];
        }
    }
    playhead = (playhead + buffer.getNumSamples()) % loopLength;//update playhead;
    //    }
}

void ASR10LoopFinderAudioProcessor::loadSample(const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader != nullptr)
    {
        int numSamples = static_cast<int>(reader->lengthInSamples);
        sampleBuffer.setSize(reader->numChannels, numSamples); //resize the buffer for stereo
        reader->read(&sampleBuffer, 0, numSamples, 0, true, true); //read into buffer
        sampleLoaded = true;
        findLoopPoints(); //sets loop points after loading
        thumbnail.setSource(new FileInputSource(file)); // Update thumbnail
        DBG("Sample loaded: " + file.getFullPathName() +
            ", channels: " + juce::String(reader->numChannels) +
            ", samples: " + juce::String(numSamples));
    }
    else
    {
        sampleLoaded = false;
        loopStartSample = -1;
        loopEndSample = -1;
        zeroCrossings.clear();
        DBG("Failed to load sample: " + file.getFullPathName());
    }
}

void ASR10LoopFinderAudioProcessor::findLoopPoints()
{
    DBG("Entering findLoopPoints");
    if (!sampleLoaded || sampleBuffer.getNumSamples() == 0)
    {
        DBG("No sample loaded or empty buffer");
        loopStartSample = -1;
        loopEndSample = -1;
        zeroCrossings.clear();
        return;
    }
    
    LoopFinder finder(sampleBuffer, sampleRate);
    zeroCrossings = finder.getAllZeroCrossings();
    DBG("Found " + juce::String(zeroCrossings.size()) + " zero crossings");
    
    //    auto [defaultStart, defaultEnd] = finder.findDefaultLoopPoints();
    //    loopStartSample = defaultStart;
    //    loopEndSample = defaultEnd;
    
    // Set defaults to 25% and 99% of sample length
    int sampleLength = sampleBuffer.getNumSamples();
    loopStartSample = static_cast<int>(0.25 * sampleLength); // ~91241 for 364966
    loopEndSample = static_cast<int>(0.99 * sampleLength);   // ~361316 for 364966
    
    // Optional: Refine to nearest zero crossings if autoLoop is on
    if (autoLoop && !zeroCrossings.empty())
    {
        loopStartSample = getNearestZeroCrossing(loopStartSample);
        loopEndSample = getNearestZeroCrossing(loopEndSample);
        if (loopStartSample >= loopEndSample)
        {
            loopEndSample = jmin(sampleLength - 1, loopStartSample + static_cast<int>(sampleRate * 0.1));
        }
    }
    
    DBG("loopStartSample: " + juce::String(loopStartSample) +
        ", loopEndSample: " + juce::String(loopEndSample) +
        ", loopLength: " + juce::String(loopEndSample - loopStartSample));
}

void ASR10LoopFinderAudioProcessor::setLoopStartSample(int sample)
{
    int maxSample = sampleBuffer.getNumSamples() - 1;
    if (autoLoop && !zeroCrossings.empty())
    {
        sample = getNearestZeroCrossing(sample);
    }
    loopStartSample = juce::jlimit(0, maxSample, sample);
    if (loopStartSample >= loopEndSample && loopEndSample != -1)
    {
        loopEndSample = juce::jmin(maxSample, loopStartSample + static_cast<int>(sampleRate / 1000.0));
    }
    DBG("Set loopStartSample to: " + juce::String(loopStartSample));
}

void ASR10LoopFinderAudioProcessor::setLoopEndSample(int sample)
{
    int maxSample = sampleBuffer.getNumSamples() - 1;
    if (autoLoop && !zeroCrossings.empty())
    {
        sample = getNearestZeroCrossing(sample);
    }
    loopEndSample = juce::jlimit(0, maxSample, sample); // JUCE utility
    if (loopEndSample <= loopStartSample && loopStartSample != -1)
    {
        loopStartSample = juce::jmax(0, loopEndSample - static_cast<int>(sampleRate * 0.1)); // JUCE utility
    }
    DBG("Set loopEndSample to: " + juce::String(loopEndSample));
}

int ASR10LoopFinderAudioProcessor::getNearestZeroCrossing(int sample) const
{
    if (zeroCrossings.empty()) return sample;
    int nearest = zeroCrossings[0].first;
    int minDiff = std::abs(sample - nearest);
    for (const auto& crossing : zeroCrossings)
    {
        int diff = std::abs(sample - crossing.first);
        if (diff < minDiff)
        {
            minDiff = diff;
            nearest = crossing.first;
        }
    }
    return nearest;
}


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
