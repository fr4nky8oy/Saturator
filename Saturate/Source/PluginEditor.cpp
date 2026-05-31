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
    : AudioProcessorEditor (p), processorRef (p)
{
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
    // Draw "Saturate" centred within the whole window area. Justification::centred
    // centres it both horizontally and vertically. The "1" is the max number of lines.
    g.drawFittedText ("Saturate", getLocalBounds(), juce::Justification::centred, 1);
}

//==============================================================================
// resized(): called whenever the window changes size (and once at startup). This
// is where we'll position child components (the knobs) in Phase 2. Empty for now
// because we have no children to lay out yet.
void SaturateAudioProcessorEditor::resized()
{
}
