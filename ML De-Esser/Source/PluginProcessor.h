/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "RTNeural/RTNeural.h"
#include <cmath>


template <typename T, int size>
class GeluActivationT
{
public:
    static constexpr auto in_size = size;
    static constexpr auto out_size = size;

    GeluActivationT() = default;

    std::string getName() const noexcept { return "gelu"; }
    constexpr bool isActivation() const noexcept { return true; }
    RTNEURAL_REALTIME void reset() {}

    RTNEURAL_REALTIME inline void forward(const T(&ins)[size]) noexcept
    {
        constexpr T sqrt_2_over_pi = (T)0.7978845608; // sqrt(2 / pi)

        for (int i = 0; i < size; ++i)
        {
            const T x = ins[i];
            // Fast Tanh Gelu Approximation Formula
            const T intermediate = sqrt_2_over_pi * (x + (T)0.044715 * x * x * x);
            outs[i] = (T)0.5 * x * ((T)1.0 + std::tanh(intermediate));
        }
    }

    // Allocate internal output storage matching RTNeural's memory alignments
    T outs alignas(RTNEURAL_DEFAULT_ALIGNMENT)[size];
};


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

    using FilterType = juce::dsp::IIR::Filter<float>;
    using FilterCoefs = juce::dsp::IIR::Coefficients<float>;

    juce::LinearSmoothedValue<float> smoothedGain{ 1.0f };

    // NN privates

    static constexpr int modelWindowSize = 512;
    static constexpr int hopSize = 128;

    

    juce::AudioBuffer<float> inputFifoBuffer; // juce::AudioDataConverters::GenericBuffer<float> fifoBuffer;
    int fifoWriteIndex = 0; // int fifoIndex = 0;
    int samplesSinceLastHop = 0;

    std::vector<float> inputAllocationBlock; // std::array<float, modelWindowSize> modelInputVector;

    static constexpr int delayLengthSamples = 512;
    juce::AudioBuffer<float> delayLineBuffer;
    int delayWriteIndex = 0;

    static constexpr int input_dim = 512;
    static constexpr int hidden_dim = 512;
    static constexpr int num_classes = 51;

    using GeluHidden = GeluActivationT<float, hidden_dim>;
    using GeluClasses = GeluActivationT<float, num_classes>;

    using ModelType = RTNeural::ModelT<float, input_dim, num_classes,
        RTNeural::DenseT<float, input_dim, hidden_dim>,
        GeluHidden,
        RTNeural::DenseT<float, hidden_dim, num_classes>,
        GeluClasses,
        RTNeural::DenseT<float, num_classes, num_classes>,
        RTNeural::SoftmaxActivationT<float, num_classes>
    >;

    std::unique_ptr<ModelType> neuralModel;

    std::atomic<int> predictedClassIndex{ 0 };

    void loadModelWeights();
    void runInference(const float* sampleBlock);

    juce::dsp::ProcessorDuplicator<FilterType, FilterCoefs> highCutFilter;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MLDeEsserAudioProcessor)
};
