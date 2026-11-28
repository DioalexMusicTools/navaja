cmake_minimum_required(VERSION 3.22)
project(DioVERB VERSION 0.1.0 LANGUAGES C CXX)

option(DIOVERB_BUILD_PLUGIN "Build JUCE VST3" ON)
enable_testing()

add_executable(dioverb_dsp_tests tests/ReverseTests.cpp)
target_include_directories(dioverb_dsp_tests PRIVATE Source)
target_compile_features(dioverb_dsp_tests PRIVATE cxx_std_17)
add_test(NAME dioverb_dsp_engine COMMAND dioverb_dsp_tests)

if(DIOVERB_BUILD_PLUGIN)
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 9.0.6
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)

    juce_add_plugin(DioVERBPlugin
        COMPANY_NAME "Dioalex Music Tools"
        BUNDLE_ID "com.dioalexmusictools.dioverb"
        PLUGIN_MANUFACTURER_CODE DlAx
        PLUGIN_CODE DiVr
        FORMATS VST3
        PRODUCT_NAME "DioVERB"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES Fx)

    juce_generate_juce_header(DioVERBPlugin)
    target_sources(DioVERBPlugin PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/ReverseEngine.h)
    target_compile_features(DioVERBPlugin PRIVATE cxx_std_17)
    target_compile_definitions(DioVERBPlugin PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
    target_link_libraries(DioVERBPlugin PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
        PUBLIC juce::juce_recommended_config_flags
               juce::juce_recommended_warning_flags)
endif()
