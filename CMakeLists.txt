cmake_minimum_required(VERSION 3.22)

# Platform settings — override at configure time via -D flags.
# Defaults: Monterey-compatible, Intel x86_64.
#   Native arm64 build: cmake -S. -B build_native -DCMAKE_OSX_ARCHITECTURES=arm64
if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 12.0)
endif()
if(NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
endif()

project(HitLaunchDmx VERSION 0.1.0 LANGUAGES C CXX)

if(NOT APPLE)
    message(FATAL_ERROR "HitLaunchDmx targets macOS only.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(HitLaunchDmx
    BUNDLE_ID                   "com.hitdmx.HitLaunchDmx"
    PRODUCT_NAME                "HitLaunchDmx"
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    ICON_BIG                    "${CMAKE_CURRENT_SOURCE_DIR}/Assets/icon_pink.png"
    ICON_SMALL                  "${CMAKE_CURRENT_SOURCE_DIR}/Assets/icon_pink.png"
)

target_sources(HitLaunchDmx PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/SceneEngine.cpp
    Source/DmxVisualizer.cpp
    Source/EnttecProDmx.cpp
    Source/MidiLog.cpp
)

target_include_directories(HitLaunchDmx PRIVATE Source)

target_compile_definitions(HitLaunchDmx PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(HitLaunchDmx PRIVATE
    "-framework IOKit"
    "-framework CoreFoundation"
    juce::juce_audio_utils
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)
