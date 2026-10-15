cmake_minimum_required(VERSION 3.22)
project(NocturneDarkSynth VERSION 1.0.0)

# Get JUCE from https://github.com/juce-framework/JUCE and set JUCE_DIR,
# or place JUCE alongside this project and configure with -DJUCE_DIR=...
find_package(JUCE CONFIG QUIET)
if(NOT JUCE_FOUND)
  if(JUCE_DIR AND EXISTS "${JUCE_DIR}/CMakeLists.txt")
    add_subdirectory("${JUCE_DIR}" JUCE)
  elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
  else()
    message(FATAL_ERROR "JUCE not found. Set JUCE_DIR to a JUCE checkout or place JUCE in ./JUCE.")
  endif()
endif()

juce_add_plugin(NocturneDarkSynth
    COMPANY_NAME "2GEEKS"
    BUNDLE_ID "com.2geeks.nocturnedarksynth"
    PLUGIN_MANUFACTURER_CODE Tgks
    PLUGIN_CODE Nct1
    FORMATS VST3 Standalone
    PRODUCT_NAME "Nocturne Dark Synth"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(NocturneDarkSynth)

target_sources(NocturneDarkSynth PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)
target_compile_features(NocturneDarkSynth PRIVATE cxx_std_17)
target_compile_definitions(NocturneDarkSynth PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)
target_link_libraries(NocturneDarkSynth PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags
)
