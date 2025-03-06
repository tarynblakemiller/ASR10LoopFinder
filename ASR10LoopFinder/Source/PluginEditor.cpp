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
    : AudioProcessorEditor(&p), processor(p)
{
    // Debug initial state
    DBG("Sample length: " + juce::String(processor.getSampleLength()));
    DBG("Loop start: " + juce::String(processor.loopStartSample));
    DBG("Loop end: " + juce::String(processor.loopEndSample));

    // Initialize Start Fader
    startFader.setRange(0, processor.getSampleLength() > 0 ? processor.getSampleLength() - 1 : 1000, 1);
    startFader.setValue(processor.loopStartSample >= 0 ? processor.loopStartSample : 0);
    startFader.setSliderStyle(juce::Slider::LinearHorizontal);
    startFader.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20);
    startFader.setColour(juce::Slider::backgroundColourId, juce::Colours::darkgrey);
    startFader.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    startFader.setColour(juce::Slider::trackColourId, juce::Colours::lightblue);
    startFader.onValueChange = [this] {
        processor.setLoopStartSample(static_cast<int>(startFader.getValue()));
        repaint();
    };
    addAndMakeVisible(startFader);

    // Initialize End Fader
    endFader.setRange(0, processor.getSampleLength() > 0 ? processor.getSampleLength() - 1 : 1000, 1);
    endFader.setValue(processor.loopEndSample >= 0 ? processor.loopEndSample : processor.getSampleLength() - 1);
    endFader.setSliderStyle(juce::Slider::LinearHorizontal);
    endFader.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20);
    endFader.setColour(juce::Slider::backgroundColourId, juce::Colours::darkgrey);
    endFader.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    endFader.setColour(juce::Slider::trackColourId, juce::Colours::lightblue);
    endFader.onValueChange = [this] {
        processor.setLoopEndSample(static_cast<int>(endFader.getValue()));
        repaint();
    };
    addAndMakeVisible(endFader);

    // Initialize Auto Loop Toggle
    autoLoopToggle.setButtonText("Auto Loop");
    autoLoopToggle.setToggleState(processor.isAutoLoop(), juce::dontSendNotification);
    autoLoopToggle.setColour(juce::ToggleButton::tickColourId, juce::Colours::lightblue);
    autoLoopToggle.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::grey);
    autoLoopToggle.onClick = [this] { processor.setAutoLoop(autoLoopToggle.getToggleState()); };
    addAndMakeVisible(autoLoopToggle);

    // Initialize Load Button
    loadButton.setButtonText("Load Sample");
    loadButton.onClick = [this] {
        juce::FileChooser chooser("Select a sample file...",
                                  juce::File::getSpecialLocation(juce::File::userHomeDirectory),
                                  "*.wav;*.aiff;*.mp3");
        chooser.launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                            [this](const juce::FileChooser& fc) {
                                if (fc.getResults().size() > 0) {
                                    juce::File file = fc.getResults()[0];
                                    processor.loadSample(file);
                                    startFader.setRange(0, processor.getSampleLength() - 1, 1);
                                    endFader.setRange(0, processor.getSampleLength() - 1, 1);
                                    startFader.setValue(processor.loopStartSample);
                                    endFader.setValue(processor.loopEndSample);
                                    repaint();
                                }
                            });
    };
    addAndMakeVisible(loadButton);

    // Set editor size
    setSize(600, 400);
}

ASR10LoopFinderAudioProcessorEditor::~ASR10LoopFinderAudioProcessorEditor()
{
}

//==============================================================================
void ASR10LoopFinderAudioProcessorEditor::paint(juce::Graphics& g)
{
    DBG("Painting editor: " + juce::String(getWidth()) + "x" + juce::String(getHeight()));
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    // Waveform display
    waveformBounds = juce::Rectangle<float>(10, 10, getWidth() - 20, getHeight() - 150);
    g.setColour(juce::Colours::grey);
    g.drawRect(waveformBounds);
    processor.getThumbnail().drawChannels(g, waveformBounds.toNearestInt(), 0.0, processor.getThumbnail().getTotalLength(), 1.0f);

    // Draw loop range
    float startX = waveformBounds.getX() + (waveformBounds.getWidth() * processor.loopStartSample / processor.getSampleLength());
    float endX = waveformBounds.getX() + (waveformBounds.getWidth() * processor.loopEndSample / processor.getSampleLength());
    g.setColour(juce::Colours::red.withAlpha(0.3f));
    g.fillRect(startX, waveformBounds.getY(), endX - startX, waveformBounds.getHeight());
    g.setColour(juce::Colours::red);
    g.drawVerticalLine(static_cast<int>(startX), waveformBounds.getY(), waveformBounds.getBottom());
    g.drawVerticalLine(static_cast<int>(endX), waveformBounds.getY(), waveformBounds.getBottom());

    // Labels
    g.setColour(juce::Colours::white);
    g.drawText("Waveform", waveformBounds.getX(), waveformBounds.getY() - 20, 100, 20, juce::Justification::centred);
    g.drawText("Start Fader", 10, waveformBounds.getBottom() + 10, 100, 20, juce::Justification::centred);
    g.drawText("End Fader", 10, waveformBounds.getBottom() + 60, 100, 20, juce::Justification::centred);
    g.drawText("Auto Loop", 10, waveformBounds.getBottom() + 110, 100, 20, juce::Justification::centred);

    // Debug info
    juce::String debugInfo = "Sample length: " + juce::String(processor.getSampleLength()) +
                             "\nLoop start: " + juce::String(processor.loopStartSample) +
                             "\nLoop end: " + juce::String(processor.loopEndSample);
    g.setFont(15.0f);
    g.drawMultiLineText(debugInfo, 20, waveformBounds.getBottom() + 150, getWidth() - 40);
}

void ASR10LoopFinderAudioProcessorEditor::resized()
{
    DBG("Resizing editor: " + juce::String(getWidth()) + "x" + juce::String(getHeight()));
    waveformBounds = juce::Rectangle<float>(10, 10, getWidth() - 20, getHeight() - 150);
    startFader.setBounds(110, waveformBounds.getBottom() + 10, getWidth() - 130, 20);
    endFader.setBounds(110, waveformBounds.getBottom() + 60, getWidth() - 130, 20);
    autoLoopToggle.setBounds(110, waveformBounds.getBottom() + 110, 100, 20);
    loadButton.setBounds(110, waveformBounds.getBottom() + 140, 100, 20);

    DBG("startFader bounds: " + startFader.getBounds().toString());
    DBG("endFader bounds: " + endFader.getBounds().toString());
    DBG("autoLoopToggle bounds: " + autoLoopToggle.getBounds().toString());
    DBG("loadButton bounds: " + loadButton.getBounds().toString());
}
