// KnobLookAndFeel.cpp
// The DEFINITIONS for the custom look declared in KnobLookAndFeel.h.
// It draws a rotary slider by copying one frame out of a vertical filmstrip image.

// Our own header (the class shape).
#include "KnobLookAndFeel.h"
// The generated header holding our embedded assets (BinaryData::knob_filmstrip_png).
#include "BinaryData.h"

//==============================================================================
// Constructor: decode the embedded filmstrip PNG into our Image, once. ImageCache
// keeps a single shared decoded copy, so this is cheap.
KnobLookAndFeel::KnobLookAndFeel()
{
    filmstrip = juce::ImageCache::getFromMemory (BinaryData::knob_filmstrip_png,
                                                 BinaryData::knob_filmstrip_pngSize);
}

//==============================================================================
// JUCE calls this to paint a rotary slider. We ignore the arc angles and instead
// pick the filmstrip frame that matches the knob's 0..1 position, then draw it.
void KnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPosProportional,
                                        float rotaryStartAngle, float rotaryEndAngle,
                                        juce::Slider& slider)
{
    // We don't use these (we draw frames, not an arc), so silence "unused" warnings.
    juce::ignoreUnused (rotaryStartAngle, rotaryEndAngle, slider);

    // Each frame is square, so its size = the strip's width. Number of frames =
    // how many of those squares fit in the strip's height (here 11328 / 236 = 48).
    const int frameSize = filmstrip.getWidth();
    const int numFrames = filmstrip.getHeight() / frameSize;

    // Turn the 0..1 knob position into a frame index (0 .. numFrames-1). round()
    // picks the nearest frame; jlimit clamps so we never index past the last frame.
    int frameIndex = juce::roundToInt (sliderPosProportional * (numFrames - 1));
    frameIndex = juce::jlimit (0, numFrames - 1, frameIndex);

    // Draw that one frame, scaled into the knob's rectangle.
    //   dest rect:   x, y, width, height                  (where the knob sits on screen)
    //   source rect: 0, frameIndex*frameSize, frameSize, frameSize  (the chosen frame)
    g.drawImage (filmstrip,
                 x, y, width, height,
                 0, frameIndex * frameSize, frameSize, frameSize);
}
