/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DelayXAudioProcessor::DelayXAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

DelayXAudioProcessor::~DelayXAudioProcessor()
{
}

//==============================================================================
const juce::String DelayXAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DelayXAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DelayXAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DelayXAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DelayXAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DelayXAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DelayXAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DelayXAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DelayXAudioProcessor::getProgramName (int index)
{
    return {};
}

void DelayXAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void DelayXAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mSampleRate = sampleRate;
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    const int numOutputChannels = getTotalNumOutputChannels();
    const int delayBufferSize = sampleRate* 2;
    mDelayBufferLength = delayBufferSize;
    mDelayBuffer.setSize(numOutputChannels, delayBufferSize);
    mWetBuffer.setSize(numOutputChannels, samplesPerBlock);
    mDelayBuffer.clear();
    mReadPosition = (int)(mWritePosition - (delayLength * getSampleRate()) + delayBufferSize) % delayBufferSize;
}

void DelayXAudioProcessor::releaseResources()
{ 
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DelayXAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void DelayXAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();
    

    int dpr, dpw;

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        float* delayData = mDelayBuffer.getWritePointer(juce::jmin(channel, mDelayBuffer.getNumChannels() - 1));

        dpr = mReadPosition;
        dpw = mWritePosition;

        for (int i = 0; i < numSamples; ++i) {
            const float in = channelData[i];
            float out = 0.0;

            out = (dryMix * in + wetMix * delayData[dpr]);

            delayData[dpw] = in + (delayData[dpr] * feedback);

            if (++dpr >= mDelayBufferLength)
                dpr = 0;
            if (++dpw >= mDelayBufferLength)
                dpw = 0;

            channelData[i] = out;
        }
    }

    mReadPosition = dpr;
    mWritePosition = dpw;
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
}
void DelayXAudioProcessor::fillDelayBuffer(int channel,const int bufferLength,const int delayBufferLength,const float* bufferData,const float *delayBufferData)
{
    //Main Buffer to Delay Buffer
    if (delayBufferLength >= bufferLength + mWritePosition)
    {
        mDelayBuffer.copyFromWithRamp(channel, mWritePosition, bufferData, bufferLength, 0.8, 0.8);
        //mDelayBuffer.copyFromWithRamp()
    }
    else
    {
        const int bufferRemaining = delayBufferLength - mWritePosition;
        mDelayBuffer.copyFromWithRamp(channel, mWritePosition, bufferData, bufferRemaining, 0.8, 0.8);
        mDelayBuffer.copyFromWithRamp(channel, 0, bufferData+bufferRemaining, bufferLength - bufferRemaining, 0.8, 0.8);
    }


}

void DelayXAudioProcessor::getDelayBuffer(juce::AudioBuffer<float>& buffer,int channel, const int bufferLength, const int delayBufferLength, const float* wetBufferData, const float* delayBufferData)
{
     int delayTime = 250;
     const int readPosition=static_cast<int>(delayBufferLength+mWritePosition-(mSampleRate*delayTime/1000))%delayBufferLength;

     if (delayBufferLength > bufferLength + readPosition)
     {
         buffer.copyFrom(channel, 0, delayBufferData+readPosition, bufferLength);
     }
     else
     {
         const int bufferRemaining = delayBufferLength - readPosition;
         mWetBuffer.copyFrom(channel, 0, delayBufferData +readPosition, bufferRemaining);
         mWetBuffer.copyFrom(channel, bufferRemaining, delayBufferData, bufferLength - bufferRemaining);
     }
     buffer.addFrom(channel,0,wetBufferData,bufferLength);
}
void DelayXAudioProcessor::feedbackDelay( int channel, const int bufferLength, const int delayBufferLength,const float* wetBufferData)
{
    if (delayBufferLength > bufferLength + mWritePosition)
    {
        mDelayBuffer.addFromWithRamp(channel, mWritePosition, wetBufferData, bufferLength, 0.8, 0.8);
    }
    else
    {
        const int bufferRemaining = delayBufferLength - mWritePosition;
        mDelayBuffer.addFromWithRamp(channel, bufferRemaining, wetBufferData, bufferRemaining, 0.8, 0.8);
        mDelayBuffer.addFromWithRamp(channel, 0, wetBufferData+bufferRemaining,bufferLength-bufferRemaining, 0.8, 0.8);
    }
}
//==============================================================================
bool DelayXAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DelayXAudioProcessor::createEditor()
{
    return new DelayXAudioProcessorEditor (*this);
}

//==============================================================================
void DelayXAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void DelayXAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DelayXAudioProcessor();
}
