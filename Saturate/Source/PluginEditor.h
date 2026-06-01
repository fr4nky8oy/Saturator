// PluginEditor.h
// The GUI side of the plugin. The processor (PluginProcessor) handles AUDIO;
// the editor handles what the user SEES and clicks. One processor can have its
// editor opened and closed many times — they're separate objects on purpose.
//
// Right now this is an EMPTY window: a grey rectangle with a bit of text. The
// four knobs (Drive/Output/Mix/Tone) arrive in Phase 2.

// Include-once guard (same reason as in PluginProcessor.h).
#pragma once

// We need the processor's declaration because the editor holds a reference to it
// (so the GUI can read/write the processor's state later).
#include "PluginProcessor.h"

// We inherit from juce::AudioProcessorEditor — JUCE's base class for a plugin GUI.
// That base class is itself a juce::Component (JUCE's "anything drawable" type),
// which is why we can override paint() and resized() below.
class SaturateAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    //==============================================================================
    // Constructor takes a reference to the processor that owns this editor. The "&"
    // means we get a reference (an alias to the real processor), not a copy.
    // Destructor cleans up when the window closes.
    explicit SaturateAudioProcessorEditor (SaturateAudioProcessor&);
    ~SaturateAudioProcessorEditor() override;

    //==============================================================================
    // paint(): JUCE calls this whenever the window needs to be (re)drawn. We draw
    // the background and any text/graphics here using the supplied Graphics context.
    void paint (juce::Graphics&) override;

    // resized(): JUCE calls this whenever the window's size changes (including once
    // at startup). This is where we'll position child components (the knobs) later.
    void resized() override;

private:
    //==============================================================================
    // A reference back to the processor this editor belongs to. We store it so the
    // GUI can later read parameter values / attach knobs to them. "processorRef"
    // is the conventional name. It's a reference, so it always points at the one
    // real processor — never a copy.
    SaturateAudioProcessor& processorRef;

    // The faceplate artwork, loaded once from the embedded BinaryData and drawn in
    // paint(). juce::Image is JUCE's in-memory picture type. We keep it as a member
    // so we load it a single time (in the constructor) rather than every repaint.
    juce::Image backgroundImage;

    // The on-screen knob for the Drive parameter. juce::Slider is JUCE's component
    // for a knob/fader. This just creates the object; we make it visible (4b),
    // position it (4c), and connect it to the parameter (4d) next.
    juce::Slider driveSlider;

    // The connector that keeps driveSlider and the "drive" parameter in sync (both
    // ways), automatically. Declared AFTER driveSlider so it's destroyed first when
    // the window closes. Like apvts, it has no empty form — we build it in the
    // constructor (next part), which clears the "must initialize" error.
    juce::AudioProcessorValueTreeState::SliderAttachment driveAttachment;

    // Same safety macro as the processor: no accidental copies, plus a debug-build
    // leak detector.
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SaturateAudioProcessorEditor)
};
