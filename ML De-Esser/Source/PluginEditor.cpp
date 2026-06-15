/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MLDeEsserAudioProcessorEditor::MLDeEsserAudioProcessorEditor (MLDeEsserAudioProcessor& p)
    : AudioProcessorEditor (&p),
     #if JucePlugin_Enable_ARA
      AudioProcessorEditorARAExtension (&p),
     #endif
      audioProcessor (p)
{
   #if JucePlugin_Enable_ARA
    // ARA plugins must be resizable for proper view embedding
    setResizable (true, false);
   #endif

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);

    // --- Style & Setup Cutoff Slider ---
    cutoffSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    cutoffSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    addAndMakeVisible(cutoffSlider);

    // Connect Cutoff Slider to APVTS via ID "HPF_CUTOFF"
    cutoffAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "HCF_CUTOFF", cutoffSlider);

    // --- Style & Setup Gain Slider ---
    gainSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    addAndMakeVisible(gainSlider);

    // Connect Gain Slider to APVTS via ID "GAIN_RED"
    gainAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "GAIN_RED", gainSlider);

    // --- Style & Setup Toggle Button ---
    enabledToggle.setButtonText("FX Enabled");
    addAndMakeVisible(enabledToggle);

    // Connect Toggle to APVTS via ID "PROCESS_ON"
    toggleAttachment = std::make_unique<ButtonAttachment>(audioProcessor.getAPVTS(), "PROCESS_ON", enabledToggle);

}

MLDeEsserAudioProcessorEditor::~MLDeEsserAudioProcessorEditor()
{
}

//==============================================================================
void MLDeEsserAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(15.0f));

    g.drawText("HCF Cutoff", 50, 40, 100, 20, juce::Justification::centred);
    g.drawText("Gain Reduction", 250, 40, 100, 20, juce::Justification::centred);
}

void MLDeEsserAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    cutoffSlider.setBounds(50, 70, 100, 100);
    gainSlider.setBounds(250, 70, 100, 100);

    enabledToggle.setBounds(150, 200, 100, 30);

}
