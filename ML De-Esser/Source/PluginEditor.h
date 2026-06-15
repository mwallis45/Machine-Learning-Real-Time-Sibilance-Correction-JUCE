/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class MLDeEsserAudioProcessorEditor  : public juce::AudioProcessorEditor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorEditorARAExtension
                            #endif
{
public:
    MLDeEsserAudioProcessorEditor (MLDeEsserAudioProcessor&);
    ~MLDeEsserAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    MLDeEsserAudioProcessor& audioProcessor;

    juce::Slider cutoffSlider;
    juce::Slider gainSlider;
    juce::ToggleButton enabledToggle;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> cutoffAttachment;
    std::unique_ptr<SliderAttachment> gainAttachment;
    std::unique_ptr<ButtonAttachment> toggleAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MLDeEsserAudioProcessorEditor)
};
