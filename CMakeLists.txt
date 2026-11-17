cmake_minimum_required(VERSION 3.22)

# Famous Beats Vocal Presets — one-click vocal chain for macOS
# Vocal Cleaner (RNNoise) -> EQ -> Compressor -> Saturation -> Tune
# (pitch correction) -> Reverb -> Delay -> Output.
# Build on a Mac (see README.md) or let GitHub Actions build it for you.

# Target machine: macOS 10.15 (Catalina) and newer, Intel and Apple Silicon.
# These MUST be set before project() — the Xcode generator ignores them
# when set after project() (that mistake shipped an arm64-only build of
# the Vocal Cleaner the first time). The GitHub workflow also passes them
# on the cmake command line, which is the belt-and-braces guarantee.
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build universal binary")

project(FamousBeatsVocalPresets VERSION 1.0.0)

# ---- JUCE (downloaded automatically at configure time, not bundled) ----
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.8
)
FetchContent_MakeAvailable(JUCE)

# ---- RNNoise (C library, sources included in this project) ----
# Classic RNNoise 0.1 (xiph/rnnoise, master branch) plus the "sh" model
# (GregorR/rnnoise-models, somnolent-hogwash-2018-09-01) embedded below.
add_library(rnnoise STATIC
    Source/RNNoise/denoise.c
    Source/RNNoise/rnn.c
    Source/RNNoise/rnn_data.c
    Source/RNNoise/rnn_reader.c
    Source/RNNoise/pitch.c
    Source/RNNoise/celt_lpc.c
    Source/RNNoise/kiss_fft.c
)
target_include_directories(rnnoise PUBLIC Source/RNNoise)
# The plugin is a shared library (.vst3/.component), so this static
# library must be position-independent code (required on Linux; macOS
# builds PIC by default, but this keeps every platform the same).
set_property(TARGET rnnoise PROPERTY POSITION_INDEPENDENT_CODE ON)
# gnu99 (C extensions on) so M_PI is available in math.h on every platform.
set_property(TARGET rnnoise PROPERTY C_STANDARD 99)
set_property(TARGET rnnoise PROPERTY C_EXTENSIONS ON)
if(APPLE)
    # Silence the "shorten-64-to-32" style warnings from the old C code.
    target_compile_options(rnnoise PRIVATE -Wno-shorten-64-to-32)
endif()

# ---- The plugin ----
juce_add_plugin(FamousBeatsVocalPresets
    COMPANY_NAME "Famous Beats Studio"
    PRODUCT_NAME "Famous Beats Vocal Presets"
    BUNDLE_ID com.famousbeatsstudio.vocalpresets
    PLUGIN_MANUFACTURER_CODE FbSt
    PLUGIN_CODE FVPS
    FORMATS VST3 AU
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx
    AU_MAIN_TYPE kAudioUnitType_Effect
)

# Generate JuceHeader.h (needed because the sources #include <JuceHeader.h>).
juce_generate_juce_header(FamousBeatsVocalPresets)

# Embed the trained RNNoise model file into the plugin binary.
juce_add_binary_data(FamousBeatsVocalPresetsData
    SOURCES Source/RNNoise/sh.rnnn
)

target_sources(FamousBeatsVocalPresets PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(FamousBeatsVocalPresets
    PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(FamousBeatsVocalPresets
    PRIVATE
    FamousBeatsVocalPresetsData
    rnnoise
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# ---- Test harness (Linux only, off by default) ----
# Builds test/vocal_presets_test.cpp against the plugin target and feeds
# generated audio through the full chain: noise reduction, tuner pitch
# snap, silence, polyphonic input, clipping/NaN checks.
# Enable with: cmake -B build -DFBS_BUILD_TESTS=ON
option(FBS_BUILD_TESTS "Build the vocal presets test harness" OFF)
if(FBS_BUILD_TESTS)
    add_executable(vocal_presets_test test/vocal_presets_test.cpp)
    target_link_libraries(vocal_presets_test PRIVATE
        FamousBeatsVocalPresets
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_dsp)
    # The test includes PluginProcessor.cpp's BinaryData.h (generated)
    # and rnnoise.h (from the rnnoise static lib, whose include dir is
    # private to the plugin target).
    target_include_directories(vocal_presets_test PRIVATE
        ${CMAKE_SOURCE_DIR}/Source/RNNoise
        ${CMAKE_BINARY_DIR}/FamousBeatsVocalPresets_artefacts/JuceLibraryCode)
endif()
