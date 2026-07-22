/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("HCF_CUTOFF", 1), "HCF Cutoff",
        juce::NormalisableRange<float>(1000.0f, 20000.0f, 1.0f, 0.3f), 10000.0f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("GAIN_RED", 1), "Gain Reduction", 0.0f, 48.0f, 0.0f
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("PROCESS_ON", 1), "Processing Enabled", true
    ));

    return layout;
}

//==============================================================================
MLDeEsserAudioProcessor::MLDeEsserAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
    apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    cutoffParameter = apvts.getRawParameterValue("HCF_CUTOFF");
    gainParamater = apvts.getRawParameterValue("GAIN_RED");
    enabledParameter = apvts.getRawParameterValue("PROCESS_ON");

    neuralModel = std::make_unique<ModelType>();
    inputAllocationBlock.resize(input_dim, 0.0f);

    loadModelWeights();
}

MLDeEsserAudioProcessor::~MLDeEsserAudioProcessor()
{
}

//==============================================================================
const juce::String MLDeEsserAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MLDeEsserAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MLDeEsserAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MLDeEsserAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MLDeEsserAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MLDeEsserAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int MLDeEsserAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MLDeEsserAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String MLDeEsserAudioProcessor::getProgramName (int index)
{
    return {};
}

void MLDeEsserAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void MLDeEsserAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    // DSP Pre processing

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    highCutFilter.prepare(spec);

    *highCutFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 10000.0f);

    smoothedGain.reset(50); // Set to 50 samples
    smoothedGain.setCurrentAndTargetValue(1.0f);

    // NN Pre processing

    inputFifoBuffer.setSize(1, modelWindowSize);
    inputFifoBuffer.clear();
    fifoWriteIndex = 0;
    samplesSinceLastHop = 0;
    std::fill(inputAllocationBlock.begin(), inputAllocationBlock.end(), 0.0f); // std::fill(modelInputVector.begin(), modelInputVector.end(), 0.0f);

    delayLineBuffer.setSize(getTotalNumOutputChannels(), delayLengthSamples / 2);
    delayLineBuffer.clear();
    delayWriteIndex = 0;

    setLatencySamples(delayLengthSamples);

    if (neuralModel != nullptr) {
        neuralModel->reset();
    }

    // classifier.reset();

}

void MLDeEsserAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MLDeEsserAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif


void MLDeEsserAudioProcessor::loadModelWeights()
{

    if (BinaryData::model_json != nullptr && BinaryData::model_jsonSize > 0)
    {
        nlohmann::json state_dict = nullptr;

        // 2 class
        if (true) {
            nlohmann::json state_dict = nlohmann::json::parse(
                BinaryData::two_model_json,
                BinaryData::two_model_json + BinaryData::two_model_jsonSize
            );
        }

        // 3 class
        if (true) {
            nlohmann::json state_dict = nlohmann::json::parse(
                BinaryData::three_model_json,
                BinaryData::three_model_json + BinaryData::three_model_jsonSize
            );
        }

        // 5 class
        if (true) {
            nlohmann::json state_dict = nlohmann::json::parse(
                BinaryData::five_model_json,
                BinaryData::five_model_json + BinaryData::five_model_jsonSize
            );
        }

        // 9 class
        if (true) {
            nlohmann::json state_dict = nlohmann::json::parse(
                BinaryData::nine_model_json,
                BinaryData::nine_model_json + BinaryData::nine_model_jsonSize
            );
        }

        // 51 class
        if (true) {
            nlohmann::json state_dict = nlohmann::json::parse(
                BinaryData::all_model_json,
                BinaryData::all_model_json + BinaryData::all_model_jsonSize
            );
        }

        // csd 44100
        if (true) {
            nlohmann::json state_dict = nlohmann::json::parse(
                BinaryData::csd_high_model_json,
                BinaryData::csd_high_model_json + BinaryData::csd_high_model_jsonSize
            );
        }

        // csd 16000
        if (true) {
            nlohmann::json state_dict = nlohmann::json::parse(
                BinaryData::csd_low_model_json,
                BinaryData::csd_low_model_json + BinaryData::csd_low_model_jsonSize
            );
        }

        using namespace RTNeural::torch_helpers;
        loadDense<float>(state_dict, "fc1.", neuralModel->get<0>());
        loadDense<float>(state_dict, "fc2.", neuralModel->get<2>());
        loadDense<float>(state_dict, "fc3.", neuralModel->get<4>());
    }
    else
    {
        jassertfalse; // Execution drops here if the file wasn't bundled properly
    }
}

void MLDeEsserAudioProcessor::runInference(const float* sampleBlock)
{

    neuralModel->forward(sampleBlock);
    const float* outputProbs = neuralModel->getOutputs();
    int bestClass = 0;
    float maxProb = -1.0f;

    for (int i = 0; i < num_classes; ++i)
    {
        if (outputProbs[i] > maxProb)
        {
            maxProb = outputProbs[i];
            bestClass = i;
        }
    }
    predictedClassIndex.store(bestClass, std::memory_order_relaxed);

}


void MLDeEsserAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.

    int numSamples = buffer.getNumSamples();

    for (int sample = 0; sample < numSamples; ++sample) {
        float incomingSample = buffer.getSample(0, sample);

        inputFifoBuffer.setSample(0, fifoWriteIndex, incomingSample);
        fifoWriteIndex = (fifoWriteIndex + 1) % modelWindowSize;

        samplesSinceLastHop++;

        if (samplesSinceLastHop >= hopSize) {
            samplesSinceLastHop = 0;

            int readIdx = fifoWriteIndex;
            for (int i = 0; i < modelWindowSize; ++i) {
                inputAllocationBlock[i] = inputFifoBuffer.getSample(0, readIdx);
                readIdx = (readIdx + 1) % modelWindowSize;
            }

            runInference(inputAllocationBlock.data());

        }

        int currentClass = predictedClassIndex.load(std::memory_order_relaxed);
        bool isSibilantSound = (currentClass == 20 || currentClass == 27 || currentClass == 31 || currentClass == 40 || currentClass == 41 || currentClass == 42 || currentClass == 43 || currentClass == 49);

        bool isPluginEnabled = (enabledParameter->load() > 0.5f);
        bool shouldProcess = isPluginEnabled && isSibilantSound; // bool shouldProcess = (enabledParameter->load() > 0.5f);

        float targetGain = 1.0f;
        if (shouldProcess) {
            targetGain = juce::Decibels::decibelsToGain(-gainParamater->load());
        }
        smoothedGain.setTargetValue(targetGain);
        float currentGain = smoothedGain.getNextValue();

        for (int channel = 0; channel < totalNumInputChannels; ++channel) {
            float liveSample = buffer.getSample(channel, sample);
            float delayedSample = delayLineBuffer.getSample(channel, delayWriteIndex);
            delayLineBuffer.setSample(channel, delayWriteIndex, liveSample);


            delayedSample *= currentGain;

            buffer.setSample(channel, sample, delayedSample);

        }

        delayWriteIndex = (delayWriteIndex + 1) % delayLengthSamples;

    }

    bool lastSampleActive = (enabledParameter->load() > 0.5f) &&
        [this]() {
        int c = predictedClassIndex.load(std::memory_order_relaxed);
        return (c == 20 || c == 27 || c == 31 || c == 40 || c == 41 || c == 42 || c == 43 || c == 49); // Must match your target classes above
        }();

    if (lastSampleActive) {
        *highCutFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(getSampleRate(), cutoffParameter->load());
        juce::dsp::AudioBlock<float> audioBlock(buffer);
        juce::dsp::ProcessContextReplacing<float> context(audioBlock);
        highCutFilter.process(context);
    }

}

//==============================================================================
bool MLDeEsserAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* MLDeEsserAudioProcessor::createEditor()
{
    return new MLDeEsserAudioProcessorEditor (*this);
}

//==============================================================================
void MLDeEsserAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void MLDeEsserAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MLDeEsserAudioProcessor();
}
