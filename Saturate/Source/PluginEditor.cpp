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
      // Build each knob's look with its own strip: Drive uses knob_L, Output uses knob_R.
      driveLook  (BinaryData::knob_L_png, BinaryData::knob_L_pngSize),
      outputLook (BinaryData::knob_R_png, BinaryData::knob_R_pngSize),
      // Connect each knob to its parameter (which box, which param ID, which slider).
      driveAttachment (processorRef.apvts, "drive", driveSlider),
      outputAttachment (processorRef.apvts, "output", outputSlider)
{
    // Decode the embedded faceplate PNG into our Image, once. ImageCache keeps a single
    // shared copy in memory, so this is cheap and safe to call here. The two arguments
    // are the byte array and its size, both from BinaryData (our juce_add_binary_data).
    backgroundImage = juce::ImageCache::getFromMemory (BinaryData::background_png,
                                                       BinaryData::background_pngSize);

    // Load the VT323 font from the embedded bytes, and start the 30 Hz refresh so the
    // live value numbers keep up with the knobs (and host automation).
    numberTypeface = juce::Typeface::createSystemTypefaceFor (BinaryData::VT323Regular_ttf,
                                                              BinaryData::VT323Regular_ttfSize);
    startTimerHz (30);

    // Decode the Link toggle images (lit / unlit) once, ready to draw in paint().
    buttonOn  = juce::ImageCache::getFromMemory (BinaryData::button_on_png,  BinaryData::button_on_pngSize);
    buttonOff = juce::ImageCache::getFromMemory (BinaryData::button_off_png, BinaryData::button_off_pngSize);

    // --- Drive knob ---
    // Register it as a child of this window and make it show on screen.
    addAndMakeVisible (driveSlider);
    // Make it a ROUND knob you turn by dragging up/down or left/right.
    driveSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    // No built-in value box — our retro VT323 readouts (Step 6) show the numbers instead.
    driveSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    // Draw it with the Drive (knob_L) filmstrip look instead of the default dial.
    driveSlider.setLookAndFeel (&driveLook);

    // --- Output knob --- (same setup, same look, bound to the "output" parameter)
    addAndMakeVisible (outputSlider);
    outputSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    outputSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    outputSlider.setLookAndFeel (&outputLook);

    // Size the window to the artwork's shape. The faceplate is square (2000x2000), so
    // we use a square 600x600 on screen — big enough to read, small enough to fit. The
    // background image gets scaled to fill this in paint().
    setSize (600, 600);
}

// Destructor. Nothing to clean up manually.
SaturateAudioProcessorEditor::~SaturateAudioProcessorEditor()
{
    // CRITICAL: detach the custom look before this editor (and its knobLookAndFeel
    // member) are destroyed, so the slider isn't left pointing at freed memory.
    driveSlider.setLookAndFeel (nullptr);
    outputSlider.setLookAndFeel (nullptr);
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

    // --- live value numbers (VT323, amber) drawn into the empty baked boxes ---
    const float w = (float) getWidth();
    const float h = (float) getHeight();
    g.setColour (juce::Colour (0xfff0d8c0));                  // warm cream, matching baked labels
    g.setFont (juce::Font (juce::FontOptions (numberTypeface).withHeight (h * 0.032f)));

    // read the current parameter values straight from the processor
    const float drive  = processorRef.apvts.getRawParameterValue ("drive")->load();
    const float output = processorRef.apvts.getRawParameterValue ("output")->load();

    // box size to draw the text in (a bit wider than the baked number, for headroom)
    const int bw = juce::roundToInt (0.10f * w);
    const int bh = juce::roundToInt (0.05f * h);

    // Drive number box (centre 0.3896, 0.6396)
    // Show Drive normalized 0.00..1.00 for the user; the engine value stays 1..25.
    g.drawText (juce::String ((drive - 1.0f) / 24.0f, 2),
                juce::roundToInt (0.3896f * w - bw * 0.5f),
                juce::roundToInt (0.6396f * h - bh * 0.5f), bw, bh,
                juce::Justification::centred);

    // Output number box (centre 0.6106, 0.6396)
    g.drawText (juce::String (output, 1),
                juce::roundToInt (0.6106f * w - bw * 0.5f),
                juce::roundToInt (0.6396f * h - bh * 0.5f), bw, bh,
                juce::Justification::centred);

    // Link button: lit image when link is on, unlit when off (drawn over the baked socket).
    const bool linkOn = processorRef.apvts.getRawParameterValue ("link")->load() > 0.5f;
    g.drawImage (linkOn ? buttonOn : buttonOff, linkButtonArea().toFloat());
}

//==============================================================================
// resized(): called whenever the window changes size (and once at startup). This
// is where we'll position child components (the knobs) in Phase 2. Empty for now
// because we have no children to lay out yet.
void SaturateAudioProcessorEditor::timerCallback()
{
    repaint();   // redraw so the value numbers track the current knob / host automation
}

// The Link button's on-screen rectangle, from the Blender coords (centre 0.5, 0.5086;
// square side 0.0766). Computed from the live window size so it scales with the window.
juce::Rectangle<int> SaturateAudioProcessorEditor::linkButtonArea() const
{
    const float w = (float) getWidth(), h = (float) getHeight();
    return { juce::roundToInt (0.4617f * w), juce::roundToInt (0.4703f * h),   // top-left
             juce::roundToInt (0.0766f * w), juce::roundToInt (0.0766f * h) }; // square size
}

void SaturateAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    // A click inside the Link button flips the link parameter on/off.
    if (linkButtonArea().contains (e.getPosition()))
    {
        auto* p = processorRef.apvts.getParameter ("link");
        const bool on = p->getValue() > 0.5f;        // current state (normalized 0/1)
        p->setValueNotifyingHost (on ? 0.0f : 1.0f); // flip it
    }
}

void SaturateAudioProcessorEditor::resized()
{
    // Window dimensions right now (square: 600x600, but we read them so it stays
    // correct if the size ever changes).
    const float w = (float) getWidth();
    const float h = (float) getHeight();

    // The new per-knob strips are cropped tight and CENTRED on the knob (square crop =
    // knob's tallest dim 0.1638 * 1.25 padding = 0.2048). So the box side is 0.2048 of
    // the window, and we place it centred on each socket (no per-frame offset needed).
    const float S = 0.2048f * w;           // square box side, as a fraction of the window

    // --- Drive knob --- socket centre (0.3809, 0.4418); box top-left = centre - S/2.
    driveSlider.setBounds (juce::roundToInt (0.3809f * w - 0.5f * S),
                           juce::roundToInt (0.4418f * h - 0.5f * S),
                           juce::roundToInt (S), juce::roundToInt (S));

    // --- Output knob --- socket centre (0.6191, 0.4418).
    outputSlider.setBounds (juce::roundToInt (0.6191f * w - 0.5f * S),
                            juce::roundToInt (0.4418f * h - 0.5f * S),
                            juce::roundToInt (S), juce::roundToInt (S));
}
