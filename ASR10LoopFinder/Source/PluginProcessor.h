/*
 ==============================================================================
 
 This file contains the basic framework code for a JUCE plugin processor.
 
 ==============================================================================
 */

#pragma once

#include <JuceHeader.h>
#include "LoopFinder.h"

//==============================================================================
/**
 */
class ASR10LoopFinderAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    ASR10LoopFinderAudioProcessor();
    ~ASR10LoopFinderAudioProcessor() override;
    
    //==============================================================================
    //required AudioProcessor overrides
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    
    //   #ifndef JucePlugin_PreferredChannelConfigurations
    //    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    //   #endif
    
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    
    //==============================================================================
    const juce::String getName() const override { return "ASR10LoopFinder"; }
    //==============================================================================
    //plugin metadata
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    
    //==============================================================================
    //program management (presets)
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const juce::String getProgramName (int index) override { return "Default"; }
    void changeProgramName (int index, const juce::String& newName) override {}
    
    //==============================================================================
    //state management - saving/loading parameters
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    void loadSample(const juce::File& file); //method to load audio file
    void findLoopPoints(); //detects loop points
    
    void setLoopStartSample(int sample);
    void setLoopEndSample(int sample);
    int getSampleLength() const { return sampleBuffer.getNumSamples(); }
    
    //auto loop control
    void setAutoLoop(bool enabled) { autoLoop = enabled; }
    bool isAutoLoop() const { return autoLoop; }
    int getNearestZeroCrossing(int sample) const;
    
    //==============================================================================
    //Editor related methods
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    
    // Waveform support
    juce::AudioThumbnail& getThumbnail() { return thumbnail; }
    
    int loopStartSample = -1; //sample index of loop start
    int loopEndSample = -1; //sample index of loop end
    int playhead = 0.0f; //tracks current position in the loop - float for smooth playback
    
    
private:
    //==============================================================================
    bool sampleLoaded = false; //tracks if sample is loaded
    juce::AudioBuffer<float> sampleBuffer; //buffer to hold our loaded sample
    double sampleRate = 44100.0; //current sample rate (ex 44100 hz)
    //    int samplesPerBlock = 0; //block size provided by the host
    
    
    juce::AudioFormatManager formatManager;
    std::vector<std::pair<int, LoopFinder::ZeroCrossingType>> zeroCrossings;
    bool autoLoop = true;
    juce::AudioThumbnail thumbnail{ 512, formatManager, thumbnailCache }; // For waveform display
    juce::AudioThumbnailCache thumbnailCache{ 5 };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ASR10LoopFinderAudioProcessor)
};
