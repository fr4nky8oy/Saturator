// PluginEditor.cpp
// The DEFINITIONS for the editor declared in PluginEditor.h.
// This draws an EMPTY window: a dark background with the word "Saturate".
// The knobs come in Phase 2.

// Our own header (the class shape we're defining).
#include "PluginEditor.h"

//==============================================================================
// Constructor. The ": AudioProcessorEditor (p)" part runs the base class's
// constructor with a reference to the processor. The ", processorRef (p)" part
// stores that same reference into our member so we can use it later.
SaturateAudioProcessorEditor::SaturateAudioProcessorEditor (SaturateAudioProcessor& p)
    : AudioProcessorEditor (p), processorRef (p),
      // Connect the knob to the parameter. The three things it needs to know:
      //   processorRef.apvts -> WHICH box holds the parameter (reached via processorRef)
      //   "drive"            -> WHICH parameter, by the ID we gave it in Step 1
      //   driveSlider        -> WHICH on-screen knob to keep in sync
      driveAttachment (processorRef.apvts, "drive", driveSlider)
{
    // Register the knob as a child of this window and make it show on screen.
    addAndMakeVisible (driveSlider);

    // Make it a ROUND knob you turn by dragging up/down or left/right
    // (instead of JUCE's default straight horizontal fader).
    driveSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);

    // Put a small value readout BELOW the knob: not editable-as-readonly = false
    // means you CAN type a value in too. The 70x20 is the readout box size in pixels.
    driveSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 20);

    // Set the window's size in pixels (width, height). JUCE will call resized()
    // right after this. 400x300 is a comfortable placeholder; we'll revisit it
    // when the knobs need room.
    setSize (400, 300);
}

// Destructor. Nothing to clean up manually.
SaturateAudioProcessorEditor::~SaturateAudioProcessorEditor()
{
}

//==============================================================================
// paint(): draw the window. JUCE hands us a Graphics object "g" — think of it as
// the paintbrush/canvas we issue drawing commands to.
void SaturateAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Fill the whole background with the host/look-and-feel's default window colour.
    // getLocalBounds() = this component's rectangle (its own coordinate space).
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // Set the pen colour to white for the text we're about to draw.
    g.setColour (juce::Colours::white);
    // Choose a 24-point font.
    g.setFont (24.0f);
    // Carve a strip off the TOP of the window for the title: full width, 50px tall.
    // removeFromTop returns that top strip. (This is JUCE's standard layout idiom —
    // you slice rectangles off the edges of a bigger rectangle.)
    auto titleArea = getLocalBounds().removeFromTop (50);

    // Draw "Saturate" centred WITHIN that top strip. Because the strip sits at the top
    // of the window, the text lands high up — and "centred" still centres it left-to-
    // right inside the full-width strip. Change the 50 above to nudge it lower/higher.
    g.drawFittedText ("Saturate", titleArea, juce::Justification::centred, 1);
}

//==============================================================================
// resized(): called whenever the window changes size (and once at startup). This
// is where we'll position child components (the knobs) in Phase 2. Empty for now
// because we have no children to lay out yet.
void SaturateAudioProcessorEditor::resized()
{
    // Place the knob: x=150 from the left, y=80 from the top, 100 wide, 120 tall.
    // (Window is 400 wide, so x=150 centres a 100-wide knob. The extra height
    // leaves room for the value readout under the dial.)
    driveSlider.setBounds (150, 80, 100, 120);
}
