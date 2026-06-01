# The minimum CMake version we require. JUCE 8 needs at least 3.22, so we ask for it.
# If someone has an older CMake, they get a clear error instead of a confusing one later.
cmake_minimum_required(VERSION 3.22)

# Declare the project: its name, a version number, and the languages we use.
# "Saturate" becomes the value of ${PROJECT_NAME}, which we reuse everywhere below.
# C is listed alongside CXX because some JUCE dependencies have a few C files.
project(Saturate VERSION 1.0.0 LANGUAGES C CXX)

# Tell the compiler to use the C++17 standard (JUCE 8's minimum).
set(CMAKE_CXX_STANDARD 17)
# Make C++17 a hard requirement — don't silently fall back to an older standard.
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Emit a compile_commands.json file. Editors/clangd read it for autocomplete and
# error-checking. Pure convenience; it doesn't affect the build itself.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Where JUCE lives on this machine. We DEFAULT to your real install (~/JUCE), unlike
# the old template which hard-coded /Applications/JUCE and broke. It's a CACHE PATH,
# so you can still override it at configure time with -DJUCE_PATH=/some/other/path.
set(JUCE_PATH "$ENV{HOME}/JUCE" CACHE PATH "Path to the JUCE framework")

# Pull JUCE into our build. We point at the JUCE ROOT folder (~/JUCE), NOT ~/JUCE/modules.
# The root CMakeLists.txt is what defines JUCE's helper commands like juce_add_plugin()
# and the juce::juce_* targets we link below. (Pointing at /modules loads no CMake at all,
# since there's no CMakeLists.txt in there — that's a classic "Unknown command" mistake.)
# The second argument is just where CMake puts JUCE's own build files, kept out of our tree.
add_subdirectory(${JUCE_PATH} ${CMAKE_BINARY_DIR}/JUCE)

# Define our plugin target and all its metadata in one call. juce_add_plugin is a
# JUCE helper that generates VST3/AU/Standalone wrappers around our shared code.
juce_add_plugin(${PROJECT_NAME}
    # The company/author name. Shows up in the DAW's plugin browser.
    COMPANY_NAME "Franky"

    # A 4-character code that identifies YOU as the manufacturer. Must be exactly 4
    # chars and contain at least one uppercase letter (an old Audio Unit rule).
    # "Frky" = Franky.
    PLUGIN_MANUFACTURER_CODE Frky

    # A 4-character code that identifies THIS plugin. Also exactly 4 chars, one
    # uppercase. The (manufacturer code + plugin code) pair must be unique per plugin
    # so hosts don't confuse two of your products. "Satr" = Saturate.
    PLUGIN_CODE Satr

    # Which plugin formats to build. VST3 (cross-DAW), AU (Apple's format for Logic/
    # GarageBand), and Standalone (a normal app, easiest for quick testing).
    FORMATS VST3 AU Standalone

    # The name the user actually sees for the plugin in their DAW.
    PRODUCT_NAME "Saturate"

    # We're an audio EFFECT, not an instrument, so this is FALSE. (TRUE would make it
    # a synth that generates sound from MIDI.)
    IS_SYNTH FALSE

    # A saturator processes incoming audio; it doesn't need MIDI notes, so FALSE.
    NEEDS_MIDI_INPUT FALSE
    # We don't send MIDI out either.
    NEEDS_MIDI_OUTPUT FALSE
    # We're not a MIDI-only effect (like an arpeggiator), we touch audio. FALSE.
    IS_MIDI_EFFECT FALSE

    # After a successful build, copy the plugin into your user plugin folders so it
    # shows up in your DAW immediately. (This is the DEV convenience copy to ~/Library.
    # The Phase-4 installer will target the system /Library instead — different thing.)
    COPY_PLUGIN_AFTER_BUILD TRUE
    # Where the .vst3 gets copied.
    VST3_COPY_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/VST3"
    # Where the .component (AU) gets copied.
    AU_COPY_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/Components"
)

# Bundle our UI assets into a compiled library JUCE calls "binary data". At build
# time JUCE reads each file's raw bytes and generates C++ source (a big byte array)
# that gets compiled in — so the images and font ship INSIDE the .vst3/.component,
# with nothing to load from disk at runtime. We reach them later as BinaryData::...
# (This is the CMake equivalent of Projucer's "Binary Resource" tick.)
juce_add_binary_data(SaturateData
    SOURCES
        Resources/background.png        # the hero faceplate (2000x2000)
        Resources/knob_filmstrip.png    # 48-frame knob rotation, reused for both knobs
        Resources/VT323-Regular.ttf)    # retro font for the live number readouts

# List the .cpp files that make up our plugin. CMake compiles these into the target.
# Headers (.h) don't go here — they're pulled in via #include. Right now it's just
# the two skeleton files we're about to write next.
target_sources(${PROJECT_NAME}
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Compile-time switches passed to JUCE. These turn OFF things we don't want.
target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        # We have no web UI, so disable JUCE's web browser component.
        JUCE_WEB_BROWSER=0
        # Disable libcurl networking — we don't make network calls.
        JUCE_USE_CURL=0
        # Tell JUCE this VST3 does NOT need to replace an old VST2 of the same plugin.
        # JUCE defaults this flag ON, which forces a build #error to make you consciously
        # decide about VST2<->VST3 parameter-automation compatibility. We have no VST2
        # (the SDK is deprecated), so we set it to 0 to opt out and clear the error.
        JUCE_VST3_CAN_REPLACE_VST2=0
        # Hide the JUCE startup splash screen.
        JUCE_DISPLAY_SPLASH_SCREEN=0
        # Don't phone home with usage stats.
        JUCE_REPORT_APP_USAGE=0
)

# Link the JUCE modules our code depends on. Each juce::juce_* is one module (a
# library). We link only what a basic audio plugin needs — we can add more later
# (e.g. juce_product_unlocking in Phase 5a for licensing).
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        SaturateData                   # our embedded assets (background, knob filmstrip, font)
        juce::juce_audio_basics        # buffers, sample maths
        juce::juce_audio_plugin_client # the VST3/AU/Standalone wrapper glue
        juce::juce_audio_processors    # AudioProcessor base class
        juce::juce_audio_utils         # AudioProcessorEditor + helper widgets
        juce::juce_core                # strings, files, core utilities
        juce::juce_dsp                 # DSP helpers we'll use for the saturator later
    PUBLIC
        juce::juce_recommended_config_flags   # JUCE's sensible default build flags
        juce::juce_recommended_lto_flags      # link-time optimisation in Release
        juce::juce_recommended_warning_flags  # a strong, helpful warning set
)
