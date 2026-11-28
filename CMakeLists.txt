cmake_minimum_required(VERSION 3.22)
project(DioCHORDX VERSION 0.1.0 LANGUAGES C CXX)
option(DIOCHORD_BUILD_PLUGIN "Build native JUCE VST3 plugin" ON)

enable_testing()
add_executable(diochord_harmony_tests tests/HarmonyTests.cpp)
target_include_directories(diochord_harmony_tests PRIVATE Source)
target_compile_features(diochord_harmony_tests PRIVATE cxx_std_17)
add_test(NAME harmony_engine COMMAND diochord_harmony_tests)

if(DIOCHORD_BUILD_PLUGIN)
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 9.0.6
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)

    juce_add_plugin(DioCHORDXPlugin
        COMPANY_NAME "Dioalex Music Tools"
        BUNDLE_ID "com.dioalexmusictools.diochordx"
        PLUGIN_MANUFACTURER_CODE DlAx
        PLUGIN_CODE DcX1
        FORMATS VST3
        PRODUCT_NAME "DioCHORD X"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT TRUE
        IS_MIDI_EFFECT FALSE
        VST3_CATEGORIES Instrument Synth
        COPY_PLUGIN_AFTER_BUILD FALSE)

    juce_generate_juce_header(DioCHORDXPlugin)
    target_sources(DioCHORDXPlugin PRIVATE
        Source/Harmony.h
        Source/PluginProcessor.h
        Source/PluginProcessor.cpp
        Source/PluginEditor.h
        Source/PluginEditor.cpp)
    target_compile_features(DioCHORDXPlugin PRIVATE cxx_std_17)
    target_compile_definitions(DioCHORDXPlugin PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
    target_link_libraries(DioCHORDXPlugin PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
        PUBLIC juce::juce_recommended_config_flags
               juce::juce_recommended_warning_flags)
endif()
