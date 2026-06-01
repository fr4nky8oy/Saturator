// PluginEditor.cpp
// The DEFINITIONS for the editor declared in PluginEditor.h.
// This draws an EMPTY window: a dark background with the word "Saturate".
// The knobs come in Phase 2.

// Our own header (the class shape we're defining).
#include "PluginEditor.h"
// The generated header holding our embedded assets (BinaryData::background_png etc.).
#include "BinaryData.h"

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
    // Decode the embedded faceplate PNG into our Image, once. ImageCache keeps a single
    // shared copy in memory, so this is cheap and safe to call here. The two arguments
    // are the byte array and its size, both from BinaryData (our juce_add_binary_data).
    backgroundImage = juce::ImageCache::getFromMemory (BinaryData::background_png,
                                                       BinaryData::background_pngSize);

    // Register the knob as a child of this window and make it show on screen.
    addAndMakeVisible (driveSlider);

    // Make it a ROUND knob you turn by dragging up/down or left/right
    // (instead of JUCE's default straight horizontal fader).
    driveSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);

    // Put a small value readout BELOW the knob: not editable-as-readonly = false
    // means you CAN type a value in too. The 70x20 is the readout box size in pixels.
    driveSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 20);

    // Size the window to the artwork's shape. The faceplate is square (2000x2000), so
    // we use a square 600x600 on screen — big enough to read, small enough to fit. The
    // background image gets scaled to fill this in paint().
    setSize (600, 600);
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
    // Draw the faceplate to fill the whole window. drawImage stretches the source
    // image into the destination rectangle we give it. getLocalBounds() is this
    // window's rectangle; .toFloat() because drawImage wants float coordinates.
    // (No grey fill or title text needed — the artwork already has them baked in.)
    g.drawImage (backgroundImage, getLocalBounds().toFloat());
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
