/*
 ==============================================================================
 
 This file contains the basic framework code for a JUCE plugin editor.
 
 ==============================================================================
 */

#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "LoopFinder.h"

//==============================================================================
ASR10LoopFinderAudioProcessorEditor::ASR10LoopFinderAudioProcessorEditor(ASR10LoopFinderAudioProcessor& p)
: AudioProcessorEditor(&p), audioProcessor(p)
{
    // Debug initial state
    DBG("Sample length: " + juce::String(audioProcessor.getSampleLength()));
    DBG("Loop start: " + juce::String(audioProcessor.loopStartSample));
    DBG("Loop end: " + juce::String(audioProcessor.loopEndSample));
    
    // Initialize Start Fader
    startFader.setRange(0, audioProcessor.getSampleLength() > 0 ? audioProcessor.getSampleLength() - 1 : 1000, 1);
    startFader.setValue(audioProcessor.loopStartSample >= 0 ? audioProcessor.loopStartSample : 0);
    startFader.setSliderStyle(juce::Slider::LinearHorizontal);
    startFader.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20);
    startFader.setColour(juce::Slider::backgroundColourId, juce::Colours::darkgrey);
    startFader.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    startFader.setColour(juce::Slider::trackColourId, juce::Colours::lightblue);
    startFader.onValueChange = [this] {
        audioProcessor.setLoopStartSample(static_cast<int>(startFader.getValue()));
        repaint();
    };
    addAndMakeVisible(startFader);
    
    // Initialize End Fader
    endFader.setRange(0, audioProcessor.getSampleLength() > 0 ? audioProcessor.getSampleLength() - 1 : 1000, 1);
    endFader.setValue(audioProcessor.loopEndSample >= 0 ? audioProcessor.loopEndSample : audioProcessor.getSampleLength() - 1);
    endFader.setSliderStyle(juce::Slider::LinearHorizontal);
    endFader.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20);
    endFader.setColour(juce::Slider::backgroundColourId, juce::Colours::darkgrey);
    endFader.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    endFader.setColour(juce::Slider::trackColourId, juce::Colours::lightblue);
    endFader.onValueChange = [this] {
        audioProcessor.setLoopEndSample(static_cast<int>(endFader.getValue()));
        repaint();
    };
    addAndMakeVisible(endFader);
    
    // Initialize Auto Loop Toggle
    autoLoopToggle.setButtonText("Auto Loop");
    autoLoopToggle.setToggleState(audioProcessor.isAutoLoop(), juce::dontSendNotification);
    autoLoopToggle.setColour(juce::ToggleButton::tickColourId, juce::Colours::lightblue);
    autoLoopToggle.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::grey);
    autoLoopToggle.onClick = [this] { audioProcessor.setAutoLoop(autoLoopToggle.getToggleState()); };
    addAndMakeVisible(autoLoopToggle);
    
    // Initialize Load Button

    loadButton.setButtonText("Load Sample");
    loadButton.onClick = [this]()

    {
        DBG("Load button clicked"); // confirm click
        
        fileChooser = std::make_unique<juce::FileChooser>("Choose file",
                                                          audioProcessor.root,
                                                          "*"
                                                          );
        const auto fileChooserFlags = juce::FileBrowserComponent::openMode |
        juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectDirectories;
        
        fileChooser->launchAsync(fileChooserFlags, [this](const juce::FileChooser& chooser)
        {
            juce::File result (chooser.getResult());
            
            //now we have the result and are going to assign it to the convolution after we make sure it's an audio file
            if(result.getFileExtension() == ".wav" | result.getFileExtension() == ".mp3")
            {
                audioProcessor.savedFile = result; //gives us our reference for saving it
                audioProcessor.root = result.getParentDirectory().getFullPathName();
                DBG("File selected: " + result.getFullPathName());
                audioProcessor.loadSample(result);
                startFader.setRange(0, audioProcessor.getSampleLength() - 1, 1);
                endFader.setRange(0, audioProcessor.getSampleLength() - 1, 1);
                startFader.setValue(audioProcessor.loopStartSample);
                endFader.setValue(audioProcessor.loopEndSample);
                repaint();
            }
            
        });
    };
    addAndMakeVisible(loadButton);
    
    setSize(600, 400); // Default size
    setResizable(true, true); // Allow resizing, with aspect ratio unconstrained
    setResizeLimits(400, 300, 1200, 800); // Min: 400x300, Max: 1200x800
    
};


ASR10LoopFinderAudioProcessorEditor::~ASR10LoopFinderAudioProcessorEditor()
{
}

//==============================================================================
void ASR10LoopFinderAudioProcessorEditor::paint(juce::Graphics& g)
{
    DBG("Painting editor: " + juce::String(getWidth()) + "x" + juce::String(getHeight()));
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    // Waveform display (scales with editor)
    waveformBounds = juce::Rectangle<float>(10, 10, getWidth() - 20, getHeight() * 0.3f); // 50% height
    g.setColour(juce::Colours::grey);
    g.drawRect(waveformBounds);
    if (audioProcessor.getSampleLength() > 0 && audioProcessor.loopStartSample >= 0 && audioProcessor.loopEndSample > audioProcessor.loopStartSample)
    {
        audioProcessor.getThumbnail().drawChannels(g, waveformBounds.toNearestInt(), 0.0, audioProcessor.getThumbnail().getTotalLength(), 1.0f);
        float startX = waveformBounds.getX() + (waveformBounds.getWidth() * audioProcessor.loopStartSample / audioProcessor.getSampleLength());
        float endX = waveformBounds.getX() + (waveformBounds.getWidth() * audioProcessor.loopEndSample / audioProcessor.getSampleLength());
        g.setColour(juce::Colours::red.withAlpha(0.3f));
        g.fillRect(startX, waveformBounds.getY(), endX - startX, waveformBounds.getHeight());
        g.setColour(juce::Colours::red);
        g.drawVerticalLine(static_cast<int>(startX), waveformBounds.getY(), waveformBounds.getBottom());
        g.drawVerticalLine(static_cast<int>(endX), waveformBounds.getY(), waveformBounds.getBottom());
    }
    else
    {
        g.setColour(juce::Colours::white);
        g.drawText("No sample loaded", waveformBounds, juce::Justification::centred, true);
    }
    
    g.setColour(juce::Colours::white);
    g.drawText("Waveform", waveformBounds.getX(), waveformBounds.getY() - 30, 100, 20, juce::Justification::left);
    g.drawText("Start", startFader.getX(), waveformBounds.getBottom() + 10, 80, 20, juce::Justification::left); // Above start fader
    g.drawText("End", endFader.getX(), waveformBounds.getBottom() + getHeight() * 0.1f + 10, 80, 20, juce::Justification::left); //
    
    
    // Debug info
    juce::String debugInfo = "Sample length: " + juce::String(audioProcessor.getSampleLength()) +
    "\nLoop start: " + juce::String(audioProcessor.loopStartSample) +
    "\nLoop end: " + juce::String(audioProcessor.loopEndSample);
    g.setFont(12.0f);
    g.drawMultiLineText(debugInfo, getWidth() - 200, getHeight() - 100, 180);
}
void ASR10LoopFinderAudioProcessorEditor::resized()
{
    DBG("Resizing editor: " + juce::String(getWidth()) + "x" + juce::String(getHeight()));
    waveformBounds = juce::Rectangle<float>(10, 10, getWidth() - 20, getHeight() * 0.3f); // 50% height
    
    const float margin = 5.0f; // Reduced margin for closer edges
    const float componentHeight = 20.0f;
    const float spacing = getHeight() * 0.05f; // Reduced spacing (5% instead of 10%)
    const float gap = getHeight() * 0.05f; // Gap under waveform (5%)
    
    startFader.setBounds(margin * 10, waveformBounds.getBottom() + gap, getWidth() - margin * 20, componentHeight);
    endFader.setBounds(margin * 10, startFader.getBottom() + spacing, getWidth() - margin * 20, componentHeight);
    autoLoopToggle.setBounds(margin * 10, endFader.getBottom() + spacing, 100, componentHeight);
    loadButton.setBounds(margin * 10, autoLoopToggle.getBottom() + spacing, 100, componentHeight);
    



    
    /**
     can use these to layout the components first:
     const auto btnX = getWidth() * JUCE_LIVE_CONSTANT(0.25);
     const auto btnY = getHeight() * JUCE_LIVE_CONSTANT(0.5);
     const auto btnWidth = getWidth() * JUCE_LIVE_CONSTANT(0.1);
     const auto btnHeight = btnWidth * 0.5;
     
     loadBtn.setBounds(btnX, btnY, btnWidth, btnHeight);
     */
    
    DBG("startFader bounds: " + startFader.getBounds().toString());
    DBG("endFader bounds: " + endFader.getBounds().toString());
    DBG("autoLoopToggle bounds: " + autoLoopToggle.getBounds().toString());
    DBG("loadButton bounds: " + loadButton.getBounds().toString());
}
