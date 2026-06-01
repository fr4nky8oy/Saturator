// KnobLookAndFeel.h
// A custom LookAndFeel that draws a rotary Slider as a frame from a vertical
// "filmstrip" image (many rotation snapshots stacked top-to-bottom), instead of
// JUCE's default dial. This is what turns a plain Slider into our Blender pot.

// Include-once guard.
#pragma once

// We need JUCE's GUI types: LookAndFeel_V4 (the base we customise), Graphics,
// Slider, Image. This module pulls them in.
#include <juce_gui_basics/juce_gui_basics.h>

// Our custom look. We inherit from LookAndFeel_V4 so we get all of JUCE's normal
// drawing for free and override ONLY the rotary-knob function below.
class KnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // Constructor: loads the filmstrip image once (defined in the .cpp).
    KnobLookAndFeel();

    // The one function we override. JUCE calls this to paint a rotary slider. We get
    // the canvas (g), the knob's rectangle (x,y,width,height), and its position as a
    // 0..1 proportion (sliderPosProportional). The rotaryStart/EndAngle params we
    // don't need (we're drawing frames, not arcs), but the signature must match.
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override;

private:
    // The vertical filmstrip (48 stacked square frames), loaded from BinaryData.
    juce::Image filmstrip;
};
