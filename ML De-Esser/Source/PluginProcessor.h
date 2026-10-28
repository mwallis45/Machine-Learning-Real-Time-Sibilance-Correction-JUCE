/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

/*
    AI decleration for this code:

    Gemini was used in the PluginProcessor.cpp file which would influence the items
    in this file. One main point that was gemini made is the RTNural parts as I didn't
    have any experience with you this.
*/

#pragma once

#include <JuceHeader.h>
#include "RTNeural/RTNeural.h"
#include <cmath>

//==============================================================================
/**
*/
class MLDeEsserAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    MLDeEsserAudioProcessor();
    ~MLDeEsserAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

private:

    // Main DSP privates

    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float>* cutoffParameter = nullptr;
    std::atomic<float>* gainParamater = nullptr;
    std::atomic<float>* enabledParameter = nullptr;

    float highCutCutoff{ 10000.0f };
    float gainReduction{ 0.0f };
    bool isProcessingEnabled{ true };

    float currentGain = 1.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    void updateSmoothingCoefficients(double sampleRate, float attackMs, float releaseMs)
    {
        // Exponential time constant formula: alpha = 1 - exp(-1 / (fs * time_in_seconds))
        attackCoeff = 1.0f - std::exp(-1.0f / (static_cast<float>(sampleRate) * (attackMs / 1000.0f)));
        releaseCoeff = 1.0f - std::exp(-1.0f / (static_cast<float>(sampleRate) * (releaseMs / 1000.0f)));
    }

    using FilterType = juce::dsp::IIR::Filter<float>;
    using FilterCoefs = juce::dsp::IIR::Coefficients<float>;

    juce::LinearSmoothedValue<float> smoothedGain{ 1.0f };
    juce::LinearSmoothedValue<float> smoothedCutoff{ 1.0f };

    bool sibilantSoundDetected = false;

    // NN privates

    static constexpr int modelWindowSize = 512;
    static constexpr int hopSize = 128;

    juce::AudioBuffer<float> inputFifoBuffer;
    int fifoWriteIndex = 0;
    int samplesSinceLastHop = 0;

    std::vector<float> inputAllocationBlock;

    static constexpr int delayLengthSamples = 512;
    juce::AudioBuffer<float> delayLineBuffer;
    int delayWriteIndex = 0;

    static constexpr int input_dim = 512;
    static constexpr int hidden_dim = 512;
    static constexpr int num_classes = 51;

    using ModelType = RTNeural::ModelT<float, input_dim, num_classes,
        RTNeural::DenseT<float, input_dim, hidden_dim>,
        RTNeural::ReLuActivationT<float, hidden_dim>,
        RTNeural::DenseT<float, hidden_dim, num_classes>,
        RTNeural::ReLuActivationT<float, num_classes>,
        RTNeural::DenseT<float, num_classes, num_classes>,
        RTNeural::SoftmaxActivationT<float, num_classes>
    >;

    std::unique_ptr<ModelType> neuralModel;

    std::atomic<int> predictedClassIndex{ 0 };

    void loadModelWeights();
    bool runInference(const float* sampleBlock);

    std::vector<FilterType> highCutFilters;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MLDeEsserAudioProcessor)
};
