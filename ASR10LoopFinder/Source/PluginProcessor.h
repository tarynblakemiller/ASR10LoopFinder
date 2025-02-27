/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

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
    //Editor related methods
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

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

private:
    //==============================================================================
    double sampleRate = 0.0; //current sample rate (ex 44100 hz)
    int samplesPerBlock = 0; //block size provided by the host
    juce::AudioBuffer<float> sampleBuffer; //buffer to hold our loaded sample
    bool sampleLoaded = false; //tracks if sample is loaded
    int playPosition = 0; //tracks current position in the loop
    int loopStartSample = 0; //sample index of loop start
    int loopEndSample = 0; //sample index of loop end
    void loadSample(const juce::File& file); //method to load audio file
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ASR10LoopFinderAudioProcessor)
};
