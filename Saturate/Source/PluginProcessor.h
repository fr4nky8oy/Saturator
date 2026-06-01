// PluginProcessor.h
// The "brain" of the plugin: the class that actually handles audio.
// In JUCE, every plugin has ONE processor (the audio/DSP side) and ONE editor
// (the GUI side). This file declares the processor; the .cpp defines it.
//
// Right now this is a PASSTHROUGH skeleton — it receives audio and hands it
// straight back out, no processing. We add the saturator DSP later (Phase 2).

// "#pragma once" tells the compiler: only include this header once per build,
// even if several files #include it. Prevents duplicate-definition errors.
// (It's the modern replacement for the old #ifndef/#define include-guard trick.)
#pragma once

// Pull in JUCE's audio-processor module. This is what gives us the
// juce::AudioProcessor base class we inherit from below, plus AudioBuffer,
// MidiBuffer, etc. The angle-bracket path matches the module we linked in CMake.
#include <juce_audio_processors/juce_audio_processors.h>

// Our processor class. It INHERITS from juce::AudioProcessor — JUCE's base class
// that defines everything a plugin host (a DAW) expects a plugin to provide.
// By inheriting, we promise to implement that set of functions (below); the host
// then calls them at the right times (e.g. processBlock on the audio thread).
class SaturateAudioProcessor : public juce::AudioProcessor,
                               private juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    // Constructor: runs once when the plugin is created. We'll use it later to set
    // up the input/output bus layout. Destructor: runs once when it's destroyed.
    // "override" means we're replacing a function the base class declared.
    SaturateAudioProcessor();
    ~SaturateAudioProcessor() override;

    //==============================================================================
    // --- Audio lifecycle: the three functions that actually move audio ---

    // Called by the host BEFORE playback starts (and whenever sample rate or block
    // size changes). This is the real-time-SAFE place to allocate/prepare things,
    // because it's NOT on the audio thread. (Saturator setup will go here later.)
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;

    // Called when playback stops. A spot to free resources if we grabbed any.
    void releaseResources() override;

    // THE audio thread function. The host calls this repeatedly with a small block
    // of samples; we process them in place. This runs under a hard real-time
    // deadline, so: no allocating, no locking, no file/IO here — ever.
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    // --- The editor (GUI) ---

    // The host calls this to create our GUI window. Returns a new editor object.
    juce::AudioProcessorEditor* createEditor() override;
    // Tells the host we DO have a custom editor (true). If false, the host would
    // draw a generic knob panel for us instead.
    bool hasEditor() const override;

    //==============================================================================
    // --- Identity & capabilities: the host queries these to know who we are ---

    // The plugin's name as shown in the DAW.
    const juce::String getName() const override;

    // Whether we accept/produce MIDI. A saturator is audio-only, so these are false.
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    // Whether we're a MIDI effect (like an arpeggiator). We're not.
    bool isMidiEffect() const override;
    // How much latency (in samples) we add. Zero for now.
    double getTailLengthSeconds() const override;

    //==============================================================================
    // --- Preset programs: the old-school numbered-program system ---
    // We don't use programs (we'll use APVTS state later), but the base class
    // REQUIRES these to exist, so we provide minimal stubs in the .cpp.
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    // --- State save/load: how the host stores the plugin's settings in a project ---
    // getStateInformation: write our current state into the given memory block.
    // setStateInformation: restore from that block when the project reopens.
    // Empty for now (no parameters yet); APVTS will fill these in Phase 2.
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // The "box" that holds all our parameters (just Drive for now). It also lets the
    // host automate them and handles saving/loading. It's PUBLIC on purpose: the
    // editor (Step 4) needs to see it so its slider can attach to the Drive parameter.
    juce::AudioProcessorValueTreeState apvts;

private:
    //==============================================================================
    // --- Parameters ---

    // Builds the list of parameters this plugin has (for now, just "Drive") and
    // returns it. We'll call this in Step 3 when we create the parameter store.
    // It's "static" because it needs to run while the object is still being built,
    // before there's a finished object to belong to.
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==============================================================================
    // --- Link coupling ---

    // Called by JUCE whenever drive/output/link changes (we registered for these in the
    // constructor). When Link is on, this mirrors one knob onto the other.
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    // Guard so the mirror doesn't ping-pong: while we're setting one knob from the other,
    // we ignore the callback that our own change triggers.
    std::atomic<bool>  linkUpdating { false };
    // The user's offset from the compensation curve, captured when Link engages, so
    // turning Link on keeps the current pot positions instead of snapping them.
    std::atomic<float> linkOffset   { 0.0f };

    //==============================================================================
    // --- Smoothing ---

    // Glides the Drive gain from its old value to a new one over a short ramp, so
    // moving the knob doesn't click. <Linear> = it walks in equal steps (a straight
    // line) — i.e. linear interpolation between old and new. We set its ramp length
    // in prepareToPlay (5b-2) and use it in processBlock (5b-3).
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> driveSmoothed;

    // The same idea as driveSmoothed, but for the Output gain. Moving the Output knob
    // jumps to a new value; this glides there over a short ramp so we don't hear a click.
    // <Linear> = it steps in a straight line from old value to new. Ramp length is set
    // in prepareToPlay (piece 3) and consumed in processBlock (piece 4).
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputSmoothed;

    //==============================================================================
    // This macro adds standard JUCE safety boilerplate to the class:
    //  - prevents accidental copying of the processor (which would be a bug), and
    //  - adds a leak detector that warns in debug builds if we forget to delete it.
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SaturateAudioProcessor)
};
