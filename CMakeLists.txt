cmake_minimum_required(VERSION 3.22)
project(DC23 VERSION 1.0.0 LANGUAGES C CXX)

option(DC23_BUILD_PLUGIN "Build the JUCE plug-in" ON)

enable_testing()
add_executable(dc23_dsp_tests tests/DSPTests.cpp)
target_include_directories(dc23_dsp_tests PRIVATE Source)
target_compile_features(dc23_dsp_tests PRIVATE cxx_std_17)
add_test(NAME dc23_dsp_core COMMAND dc23_dsp_tests)

if(DC23_BUILD_PLUGIN)
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 9.0.6
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)

    juce_add_plugin(DC23Plugin
        COMPANY_NAME "Dioalex Music Tools"
        BUNDLE_ID "com.dioalexmusictools.dc23"
        PLUGIN_MANUFACTURER_CODE DlAx
        PLUGIN_CODE Dc23
        FORMATS VST3
        PRODUCT_NAME "DC-23"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES Fx)

    juce_generate_juce_header(DC23Plugin)

    target_sources(DC23Plugin PRIVATE
        Source/PluginProcessor.cpp Source/PluginProcessor.h
        Source/PluginEditor.cpp Source/PluginEditor.h
        Source/DSP.h Source/FactoryPresets.h Source/Timing.h)
    target_compile_features(DC23Plugin PRIVATE cxx_std_17)
    target_compile_definitions(DC23Plugin PUBLIC
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
    target_link_libraries(DC23Plugin PRIVATE
        juce::juce_audio_utils
        PUBLIC juce::juce_recommended_config_flags
               juce::juce_recommended_warning_flags)
endif()
