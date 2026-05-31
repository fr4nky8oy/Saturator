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
class SaturateAudioProcessor : public juce::AudioProcessor
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
    // --- Parameters (APVTS) ---

    // A small factory that BUILDS the list of parameters this plugin owns and hands
    // it back as a ParameterLayout. We keep it as a separate static function (rather
    // than inlining it) so the constructor's initialiser list can call it cleanly to
    // construct the apvts below. "static" = it belongs to the class, not to any one
    // instance, so it can run before the object is fully built.
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // THE central parameter store. It owns our parameters, exposes them to the host
    // for automation, gives the audio thread lock-free atomic access to their values,
    // and handles saving/loading their state. It's PUBLIC so the editor (GUI) can
    // attach sliders to it directly. The args are wired up in the .cpp constructor.
    juce::AudioProcessorValueTreeState apvts;

private:
    //==============================================================================
    // --- Cached audio-thread state for the Drive parameter ---

    // A direct, lock-free pointer to the Drive parameter's current value. The APVTS
    // hands this out via getRawParameterValue(); we cache it ONCE in the constructor
    // so processBlock can read the knob with a single atomic load — no string lookup,
    // no locking — which is exactly what the real-time audio thread needs.
    std::atomic<float>* driveParameter { nullptr };

    // Smooths the Drive value over a short ramp so moving the knob doesn't cause an
    // instant jump in gain (which you'd hear as a click/"zipper"). The <Linear> type
    // means it ramps in equal steps per sample. We configure its ramp in prepareToPlay
    // and read one smoothed step per sample in processBlock.
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> driveSmoothed;

    //==============================================================================
    // This macro adds standard JUCE safety boilerplate to the class:
    //  - prevents accidental copying of the processor (which would be a bug), and
    //  - adds a leak detector that warns in debug builds if we forget to delete it.
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SaturateAudioProcessor)
};
