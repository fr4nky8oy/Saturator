// PluginProcessor.cpp
// The DEFINITIONS for everything declared in PluginProcessor.h.
// (.h = the promises / the menu; .cpp = the actual code / the kitchen.)
// Still a PASSTHROUGH: audio comes in, goes straight out, no DSP yet.

// Bring in our own header so the compiler knows the class shape we're defining.
#include "PluginProcessor.h"
// Bring in the editor header so createEditor() can build our GUI object.
#include "PluginEditor.h"

//==============================================================================
// Constructor. The ": AudioProcessor(...)" part runs the base class's constructor
// FIRST, and we use it to declare our audio "buses": stereo in, stereo out.
// BusesProperties() describes the plugin's input/output channel layout to the host.
SaturateAudioProcessor::SaturateAudioProcessor()
    : AudioProcessor (BusesProperties()
                        // We take a stereo input ("Input", 2 channels), enabled by default.
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        // We produce a stereo output ("Output", 2 channels), enabled.
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    // Body is empty for now — nothing to set up until we add parameters/DSP.
}

// Destructor. Empty: we allocated nothing that needs manual cleanup.
SaturateAudioProcessor::~SaturateAudioProcessor()
{
}

//==============================================================================
// Called before playback. The host tells us the sample rate (e.g. 48000 Hz) and
// the max block size (samples per processBlock call). We'll use these later to
// prepare the saturator. For now we deliberately ignore them.
void SaturateAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // juce::ignoreUnused stops the compiler warning "unused parameter" without us
    // having to delete the parameter names (we want them visible for when we DO
    // use them). Purely cosmetic; generates no code.
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

// Called when playback stops. Nothing to release yet.
void SaturateAudioProcessor::releaseResources()
{
}

//==============================================================================
// THE audio thread. Runs under a hard real-time deadline.
// buffer = the block of samples to process, in place. midiMessages = MIDI for
// this block (unused — we're audio-only).
void SaturateAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& midiMessages)
{
    // We don't use MIDI; silence the unused-parameter warning.
    juce::ignoreUnused (midiMessages);

    // ScopedNoDenormals disables "denormal" floating-point numbers for this scope.
    // Denormals are tiny near-zero values that are EXTREMELY slow on CPUs and can
    // spike audio-thread time. Standard, essential first line of every processBlock.
    juce::ScopedNoDenormals noDenormals;

    // How many input vs output channels the host gave us this block.
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Safety housekeeping: if there are more OUTPUT channels than INPUT channels,
    // the extra output channels contain stale/garbage memory. We clear them so we
    // don't emit noise. (With our stereo-in/stereo-out setup this loop does nothing,
    // but it's the correct, host-proof pattern.)
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // PASSTHROUGH: we intentionally do NOT touch the audio samples. The buffer
    // already holds the input, and since we leave it unchanged, the host reads the
    // same samples back as our output. The saturator DSP will replace this comment
    // block in Phase 2.
}

//==============================================================================
// Build and return our GUI. "new" hands ownership to the host, which deletes it
// when the window closes. We pass *this so the editor can talk to the processor.
juce::AudioProcessorEditor* SaturateAudioProcessor::createEditor()
{
    return new SaturateAudioProcessorEditor (*this);
}

// Yes, we provide our own editor (so the host shows our window, not a generic one).
bool SaturateAudioProcessor::hasEditor() const
{
    return true;
}

//==============================================================================
// --- Identity & capabilities: simple answers to the host's questions ---

// JucePlugin_Name is a macro the CMake juce_add_plugin step defines for us from
// PRODUCT_NAME ("Saturate") — so the name lives in one place (the build config).
const juce::String SaturateAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

// We're an audio-only effect: no MIDI in, no MIDI out, not a MIDI effect.
bool SaturateAudioProcessor::acceptsMidi() const   { return false; }
bool SaturateAudioProcessor::producesMidi() const  { return false; }
bool SaturateAudioProcessor::isMidiEffect() const  { return false; }

// We add no audible "tail" (like a reverb would). Zero seconds.
double SaturateAudioProcessor::getTailLengthSeconds() const { return 0.0; }

//==============================================================================
// --- Program stubs: required by the base class, but we don't use programs. ---
// Returning 1 (not 0) because some hosts misbehave with zero programs.
int SaturateAudioProcessor::getNumPrograms()                { return 1; }
int SaturateAudioProcessor::getCurrentProgram()             { return 0; }
void SaturateAudioProcessor::setCurrentProgram (int index)  { juce::ignoreUnused (index); }

const juce::String SaturateAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {}; // an empty string
}

void SaturateAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
// --- State save/load: empty for now (no parameters). APVTS fills these later. ---
void SaturateAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused (destData);
}

void SaturateAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// THE entry point. The host calls this one C-style function to create an instance
// of our plugin. It's how the DAW "boots" the plugin. JUCE requires exactly this.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SaturateAudioProcessor();
}
