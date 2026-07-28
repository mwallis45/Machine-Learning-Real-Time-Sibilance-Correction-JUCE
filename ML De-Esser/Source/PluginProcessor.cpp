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
    suspendProcessing(true);

    if (neuralModel != nullptr) {
        neuralModel->reset();
    }
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

    smoothedGain.reset(256); // Set to 256 samples (5 ms)
    smoothedGain.setCurrentAndTargetValue(1.0f);

    // NN Pre processing

    inputFifoBuffer.setSize(getTotalNumOutputChannels(), modelWindowSize * 2); // Set buffer to be twice the size so we can have the delayed audio aswell
    inputFifoBuffer.clear();
    fifoWriteIndex = 0;
    samplesSinceLastHop = 0;
    std::fill(inputAllocationBlock.begin(), inputAllocationBlock.end(), 0.0f); // std::fill(modelInputVector.begin(), modelInputVector.end(), 0.0f);

    setLatencySamples(delayLengthSamples);

    updateSmoothingCoefficients(sampleRate, 2.0f, 35.0f);

    if (neuralModel != nullptr) {
        neuralModel->reset();
    }

    sibilantSoundDetected = false;

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
    // Helper lambda to safely parse binary JSON data
    auto parseBinaryJson = [](const char* data, int size) -> nlohmann::json {
        if (data == nullptr || size <= 0) {
            jassertfalse;
            return nullptr;
        }

        try {
            return nlohmann::json::parse(data, data + size);
        }
        catch (const nlohmann::json::parse_error& e) {
            jassertfalse; // Traceable in debug builds
            DBG("Failed to parse model JSON: " << e.what());
            return nullptr;
        }
        };

    // 2 class
    const char* activeData = BinaryData::two_model_json;
    int activeSize = BinaryData::two_model_jsonSize;

    // 3 class
    /*const char* activeData = BinaryData::three_model_json;
    int activeSize = BinaryData::three_model_jsonSize;*/

    // 5 class
    /*const char* activeData = BinaryData::five_model_json;
    int activeSize = BinaryData::five_model_jsonSize;*/

    // 9 class
    /*const char* activeData = BinaryData::nine_model_json;
    int activeSize = BinaryData::nine_model_jsonSize;*/

    // 51 class
    /*const char* activeData = BinaryData::all_model_json;
    int activeSize = BinaryData::all_model_jsonSize;*/

    // csd 44100
    /*const char* activeData = BinaryData::csd_high_model_json;
    int activeSize = BinaryData::csd_high_model_jsonSize;*/

    // csd 16000
    /*const char* activeData = BinaryData::csd_low_model_json;
    int activeSize = BinaryData::csd_low_model_jsonSize;*/

    nlohmann::json state_dict = parseBinaryJson(activeData, activeSize);

    if (state_dict.is_null()) {
        return;
    }

    using namespace RTNeural::torch_helpers;
    loadDense<float>(state_dict, "fc1.", neuralModel->get<0>());
    loadDense<float>(state_dict, "fc2.", neuralModel->get<2>());
    loadDense<float>(state_dict, "fc3.", neuralModel->get<4>());
}

bool MLDeEsserAudioProcessor::runInference(const float* sampleBlock)
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

    bool sibilantSoundDetected = (bestClass == 1); // 2 class
    // bool sibilantSoundDetected = (currentClass == 1 || currentClass == 2); // 3 class
    // bool sibilantSoundDetected = (currentClass == 3 || currentClass == 4); // 5 class
    // bool sibilantSoundDetected = (currentClass >= 1 && currentClass <= 8); // 9 class
    // bool sibilantSoundDetected = (currentClass == 20 || currentClass == 27 || currentClass == 31 || currentClass == 40 || currentClass == 41 || currentClass == 42 || currentClass == 43 || currentClass == 49); // 51 class
    return sibilantSoundDetected;
}

void MLDeEsserAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    bool isPluginEnabled = enabledParameter->load() > 0.5f;
    int numSamples = buffer.getNumSamples();
    *highCutFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(getSampleRate(), cutoffParameter->load());

    for (int sample = 0; sample < numSamples; ++sample) {
        for (int channel = 0; channel < totalNumInputChannels; ++channel) {
            float incomingSample = buffer.getSample(channel, sample); // Get sample from the first channel
            inputFifoBuffer.setSample(channel, fifoWriteIndex, incomingSample); // Write the sample to the FIFO index
        }
        fifoWriteIndex = (fifoWriteIndex + 1) % (modelWindowSize * 2); // Increase and mod for 1024 samples
        samplesSinceLastHop++;

        if (samplesSinceLastHop >= hopSize) { // Process chunk every 128 samples
            samplesSinceLastHop = 0; 

            int readIdx = (fifoWriteIndex + modelWindowSize) % (modelWindowSize * 2); // start 512 samples back | Look at old 384 samples with new 128 samples
            for (int i = 0; i < modelWindowSize; ++i) {
                inputAllocationBlock[i] = inputFifoBuffer.getSample(0, readIdx);
                readIdx = (readIdx + 1) % (modelWindowSize * 2); // Needs to allow for the full buffer
            }
            sibilantSoundDetected = runInference(inputAllocationBlock.data());

        }

        float targetGain = 1.0f;
        if (sibilantSoundDetected && isPluginEnabled) {
            targetGain = juce::Decibels::decibelsToGain(-gainParamater->load());
        }
        smoothedGain.setTargetValue(targetGain);

        if (targetGain < currentGain) {
            currentGain += attackCoeff * (targetGain - currentGain);
        } else {
            currentGain += releaseCoeff * (targetGain - currentGain);
        }
        
        float currentGain = smoothedGain.getNextValue();
        for (int channel = 0; channel < totalNumInputChannels; ++channel) {
            int readDelayIdx = (fifoWriteIndex + modelWindowSize) % (modelWindowSize * 2);
            float delayedSample = inputFifoBuffer.getSample(channel, readDelayIdx);

            delayedSample *= currentGain;
            buffer.setSample(channel, sample, delayedSample);
        }
    }
   /* if (sibilantSoundDetected && isPluginEnabled) {
        juce::dsp::AudioBlock<float> audioBlock(buffer);
        juce::dsp::ProcessContextReplacing<float> context(audioBlock);
        highCutFilter.process(context);
    }*/
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
