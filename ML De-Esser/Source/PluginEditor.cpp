/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

/*
    AI decleration for this code:

    AI was not used in the making of this code
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

    // High Cut
    cutoffSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    cutoffSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    addAndMakeVisible(cutoffSlider);

    cutoffAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "HCF_CUTOFF", cutoffSlider);

    // Gain Slider
    gainSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    addAndMakeVisible(gainSlider);

    gainAttachment = std::make_unique<SliderAttachment>(audioProcessor.getAPVTS(), "GAIN_RED", gainSlider);

    // Enable Toggle
    enabledToggle.setButtonText("FX Enabled");
    addAndMakeVisible(enabledToggle);

    toggleAttachment = std::make_unique<ButtonAttachment>(audioProcessor.getAPVTS(), "PROCESS_ON", enabledToggle);

}

MLDeEsserAudioProcessorEditor::~MLDeEsserAudioProcessorEditor()
{
}

//==============================================================================
void MLDeEsserAudioProcessorEditor::paint (juce::Graphics& g)
{
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
