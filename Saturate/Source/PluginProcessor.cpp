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
      // ^ note the comma above — it chains on the next thing to initialize:
      // Create the box and hand it our parameter list. The four arguments:
      //   *this        -> the processor that owns these parameters
      //   nullptr      -> no undo system (we don't need one)
      //   "PARAMETERS" -> a name tag for the saved data
      //   createParameterLayout() -> the list we built in Step 1
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // Listen for changes to these three so we can mirror Drive<->Output when Link is on.
    apvts.addParameterListener ("drive",  this);
    apvts.addParameterListener ("output", this);
    apvts.addParameterListener ("link",   this);
}

// Destructor. Stop listening for parameter changes before we're destroyed.
SaturateAudioProcessor::~SaturateAudioProcessor()
{
    apvts.removeParameterListener ("drive",  this);
    apvts.removeParameterListener ("output", this);
    apvts.removeParameterListener ("link",   this);
}

//==============================================================================
// Builds and returns the list of parameters. Right now nothing calls this yet —
// we're just defining the Drive knob here. Step 3 will use it.
juce::AudioProcessorValueTreeState::ParameterLayout
    SaturateAudioProcessor::createParameterLayout()
{
    // Start with an empty list that we'll add our parameter(s) to.
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Create the Drive parameter and put it in the list. The four pieces are:
    layout.add (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "drive", 1 },             // internal ID: "drive"
        "Drive",                                       // label the user sees
        juce::NormalisableRange<float> (1.0f, 25.0f),  // range: 1.0 up to 25.0
        1.0f));                                        // default: 1.0 (no effect)

    // Create the Output parameter (a post-saturation makeup gain) and add it to the list.
    // It's measured in decibels (dB) — how engineers think about level. 0 dB means
    // "leave the level unchanged"; positive = louder, negative = quieter.
    layout.add (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "output", 1 },              // internal ID the code uses: "output"
        "Output",                                        // human-readable label shown in the DAW
        juce::NormalisableRange<float> (-14.0f, 14.0f),  // matches the link compensation range; 0 dB centred
        0.0f));                                          // default: 0 dB = unity = no change

    // Link toggle: when on, Drive and Output mirror each other (handled by listeners).
    layout.add (std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "link", 1 },   // internal ID the code looks it up by
        "Link",                            // label shown in the DAW
        false));                           // default: off

    // Hand the finished list back to whoever asked for it.
    return layout;
}

//==============================================================================
// Measured link compensation. These are the dB to REMOVE from Output at each Drive
// position (0..1 in 0.1 steps), from the K-weighted loudness of the tanh stage
// (tools/measure_link_curve.py). Anchored so Drive 0 -> 0 dB; flattens at the top.
namespace
{
    const float kLinkComp[11] = { 0.0f, -8.6f, -11.2f, -12.4f, -13.2f, -13.6f,
                                  -14.0f, -14.2f, -14.4f, -14.6f, -14.7f };

    // Drive position (0..1) -> compensation dB, by linear interpolation of the table.
    float dispToComp (float disp)
    {
        disp = juce::jlimit (0.0f, 1.0f, disp);
        const float t = disp * 10.0f;            // table index as a float (0..10)
        const int   i = juce::jmin (9, (int) t); // lower table index
        const float f = t - (float) i;           // fraction into the segment
        return kLinkComp[i] + (kLinkComp[i + 1] - kLinkComp[i]) * f;
    }

    // Inverse: compensation dB -> Drive position (0..1). The table decreases, so we
    // find the segment containing db, then interpolate.
    float compToDisp (float db)
    {
        if (db >= kLinkComp[0])  return 0.0f;    // 0 dB or louder -> Drive min
        if (db <= kLinkComp[10]) return 1.0f;    // beyond the table -> Drive max
        for (int i = 0; i < 10; ++i)
            if (db >= kLinkComp[i + 1])          // between table[i] (higher) and table[i+1] (lower)
            {
                const float span = kLinkComp[i + 1] - kLinkComp[i];   // negative
                const float f = (span != 0.0f) ? (db - kLinkComp[i]) / span : 0.0f;
                return (i + f) / 10.0f;
            }
        return 1.0f;
    }
}

//==============================================================================
// Called whenever drive/output/link changes. When Link is on, mirror one knob onto
// the other along the measured compensation curve (Drive up -> Output down).
void SaturateAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    // Ignore the change we cause ourselves while mirroring (stops the endless ping-pong).
    if (linkUpdating.load())
        return;

    // Nothing to do unless Link is engaged.
    const bool link = apvts.getRawParameterValue ("link")->load() > 0.5f;
    if (! link)
        return;

    // Link compensation from the measured curve (kLinkComp table above).
    // driveNorm = (drive-1)/24 is the knob's 0..1 position.
    auto driveToOutputDb = [](float drive){ return dispToComp ((drive - 1.0f) / 24.0f); };
    auto outputDbToDrive = [](float db)   { return 1.0f + compToDisp (db) * 24.0f; };

    // Helper: set a parameter by its REAL value (converted to 0..1, which the host wants).
    auto setParam = [this] (const juce::String& id, float value)
    {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
    };

    linkUpdating.store (true);

    if (parameterID == "link")
    {
        // Just engaged: remember the user's offset from the curve so the pots DON'T jump.
        const float drive  = apvts.getRawParameterValue ("drive") ->load();
        const float output = apvts.getRawParameterValue ("output")->load();
        linkOffset.store (output - driveToOutputDb (drive));
    }
    else if (parameterID == "drive")
        // Drive moved: pull Output along the curve, shifted by the user's offset.
        setParam ("output", driveToOutputDb (newValue) + linkOffset.load());
    else if (parameterID == "output")
        // Output moved: mirror back the other way (inverse curve, minus the offset).
        setParam ("drive", outputDbToDrive (newValue - linkOffset.load()));

    linkUpdating.store (false);
}

//==============================================================================
// Called before playback. The host tells us the sample rate (e.g. 48000 Hz) and
// the max block size (samples per processBlock call). We'll use these later to
// prepare the saturator. For now we deliberately ignore them.
void SaturateAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // We don't need the block size here; silence just that one unused warning.
    juce::ignoreUnused (samplesPerBlock);

    // Tell the smoother how long its glide should take: 0.05 seconds = 50 ms. It needs
    // the sampleRate to turn "50 ms" into a number of per-sample steps.
    driveSmoothed.reset (sampleRate, 0.05);

    // Start the smoother sitting exactly on the current knob value, so audio doesn't
    // glide up from zero when playback begins. (setCurrentAndTargetValue = snap, no ramp.)
    driveSmoothed.setCurrentAndTargetValue (apvts.getRawParameterValue ("drive")->load());

    // Give the Output smoother the same 50 ms glide. It needs sampleRate to turn
    // "50 ms" into a per-sample step count, exactly like the Drive smoother above.
    outputSmoothed.reset (sampleRate, 0.05);

    // Snap the smoother to the current Output value so audio doesn't glide up from
    // silence when playback starts. The parameter is stored in dB, but the smoother
    // (and the audio multiply in piece 4) work in LINEAR gain — so we convert first
    // with Decibels::decibelsToGain (e.g. 0 dB -> 1.0, +6 dB -> ~2.0, -6 dB -> ~0.5).
    outputSmoothed.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("output")->load()));
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

    // Read the current Drive value out of the box. getRawParameterValue gives us a
    // pointer to the live value; load() reads the number. Once per block is plenty.
    const float drive = apvts.getRawParameterValue ("drive")->load();

    // Aim the smoother at the current knob value. Instead of jumping, it will GLIDE
    // from wherever it currently is toward this target over the 50 ms ramp.
    driveSmoothed.setTargetValue (drive);

    // Read the current Output value (in dB), convert it to a linear gain, and aim the
    // smoother at it — same once-per-block pattern as Drive just above. The smoother
    // will then GLIDE toward this gain over the 50 ms ramp instead of jumping.
    const float outputGain = juce::Decibels::decibelsToGain (
        apvts.getRawParameterValue ("output")->load());
    outputSmoothed.setTargetValue (outputGain);

    // We now touch each sample ourselves (the tanh curve, next step, can't use the
    // applyGain helper). Grab the block's dimensions:
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Walk the block one SAMPLE at a time (outer loop). Pull the smoothed Drive once
    // per sample here, so both channels share the same value at that instant and the
    // smoother advances exactly once per sample.
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float d = driveSmoothed.getNextValue();
        const float g = outputSmoothed.getNextValue();   // smoothed Output gain, one step per sample

        // Inner loop: shape each channel's sample, in place. "d * channelData[sample]"
        // is Drive pushing the signal INTO the curve; std::tanh then rounds the peaks.
        // This is the tanh(drive × input) saturation from the README. We THEN multiply
        // by g (the Output makeup gain) to set the final level after the distortion.
        for (int channel = 0; channel < numChannels; ++channel)
        {
            float* channelData = buffer.getWritePointer (channel);
            channelData[sample] = std::tanh (d * channelData[sample]) * g;
        }
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
