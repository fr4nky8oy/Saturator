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
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      // Build the APVTS. Arguments:
      //   *this        -> the processor that owns these parameters
      //   nullptr      -> no separate UndoManager (we don't need undo)
      //   "PARAMETERS" -> the name of the root state node (used when saving/loading)
      //   createParameterLayout() -> the parameter list we declared above
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // Cache the lock-free pointer to the Drive value ONCE here, while we're on the
    // safe (non-audio) thread. getRawParameterValue does a string lookup internally,
    // which we never want to do inside processBlock — so we do it exactly once.
    // The ID "drive" must match the one we give the parameter in createParameterLayout.
    driveParameter = apvts.getRawParameterValue ("drive");
}

// Destructor. Empty: we allocated nothing that needs manual cleanup.
SaturateAudioProcessor::~SaturateAudioProcessor()
{
}

//==============================================================================
// Build the list of parameters the plugin exposes. Called once, from the
// constructor's initialiser list, to construct the apvts. Returns a
// ParameterLayout — essentially a container we add parameter objects into.
juce::AudioProcessorValueTreeState::ParameterLayout
    SaturateAudioProcessor::createParameterLayout()
{
    // The (initially empty) layout we'll fill and return.
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Add ONE parameter: Drive. We std::make_unique it because the layout takes
    // ownership of a heap-allocated parameter object.
    layout.add (std::make_unique<juce::AudioParameterFloat>(
        // The unique ID used in code/automation. The "1" is a VERSION HINT: if we
        // ever change this parameter's meaning later, bumping the hint tells hosts
        // the automation may differ. Must match the ID we cache in the constructor.
        juce::ParameterID { "drive", 1 },

        // The human-readable name shown in the DAW.
        "Drive",

        // The value range: from 1.0 (no extra drive) up to 25.0 (hard drive).
        // NormalisableRange maps that real range to/from the host's internal 0..1.
        // Linear for now; we can add a skew later to give the low end more resolution.
        juce::NormalisableRange<float> (1.0f, 25.0f),

        // The default value when the plugin first loads: 1.0 = unity, no change.
        1.0f));

    // Hand the finished layout back to the apvts constructor.
    return layout;
}

//==============================================================================
// Called before playback. The host tells us the sample rate (e.g. 48000 Hz) and
// the max block size (samples per processBlock call). We'll use these later to
// prepare the saturator. For now we deliberately ignore them.
void SaturateAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // We don't need the block size for this parameter; silence its unused warning.
    juce::ignoreUnused (samplesPerBlock);

    // Tell the smoother HOW LONG its ramp should take. reset() takes the sample rate
    // and a ramp length in SECONDS; 0.05 = a 50 ms glide. It uses the sample rate to
    // work out how many per-sample steps that is. Re-running here is correct because
    // the host can change the sample rate, and the ramp length must track it.
    driveSmoothed.reset (sampleRate, 0.05);

    // Snap the smoother straight to the current knob value (no ramp on the very first
    // block) so playback doesn't glide up from zero when audio starts. We read the
    // cached atomic pointer; load() gets its current float value.
    driveSmoothed.setCurrentAndTargetValue (driveParameter->load());
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

    // How many samples are in this block.
    const auto numSamples = buffer.getNumSamples();

    // Point the smoother at wherever the Drive knob is RIGHT NOW. If the value moved
    // since last block, the smoother will glide toward it over its 50 ms ramp rather
    // than jumping. One atomic load — cheap and lock-free, safe on the audio thread.
    driveSmoothed.setTargetValue (driveParameter->load());

    // Walk the block one sample at a time (outer loop) so the smoother advances once
    // per sample and the SAME gain is applied to every channel at that instant.
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // getNextValue() returns this sample's gain and advances the ramp by one step.
        const float gain = driveSmoothed.getNextValue();

        // Apply that gain to each channel's sample, in place. For now Drive is just a
        // linear volume multiply — this proves the parameter reaches the audio thread.
        // Issue #2 replaces this multiply with the tanh saturation curve.
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
            buffer.getWritePointer (channel)[sample] *= gain;
    }
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
    // Take a snapshot of the apvts's whole state (a ValueTree) and convert it to XML.
    // copyState() is thread-safe. createXml() may return nullptr, so we guard with if.
    if (auto xml = apvts.copyState().createXml())
        // Serialize that XML into the host's memory block. The DAW saves this with the
        // project, so reopening it restores every parameter exactly as it was.
        copyXmlToBinary (*xml, destData);
}

void SaturateAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Turn the saved binary blob back into XML. Guard against null (corrupt/empty data).
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        // Only load it if it's actually OUR state tree (tag matches), so we don't try
        // to apply some other plugin's data.
        if (xml->hasTagName (apvts.state.getType()))
            // Replace the live parameter state with the loaded one — restoring the knobs.
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
// THE entry point. The host calls this one C-style function to create an instance
// of our plugin. It's how the DAW "boots" the plugin. JUCE requires exactly this.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SaturateAudioProcessor();
}
