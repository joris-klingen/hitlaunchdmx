cmake_minimum_required(VERSION 3.22)

# Platform settings — override at configure time via -D flags.
# Defaults: Monterey-compatible, Intel x86_64.
#   Native arm64 build: cmake -S. -B build_native -DCMAKE_OSX_ARCHITECTURES=arm64
#   Universal build:    cmake -S. -B build_universal "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64"
if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 12.0)
endif()
if(NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
endif()

project(HitLaunchDmx VERSION 0.2.0 LANGUAGES C CXX)

if(NOT APPLE)
    message(FATAL_ERROR "HitLaunchDmx targets macOS only.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Shared engine ────────────────────────────────────────────────────────────
# The rig, palette, recipes, composition (computeDmx), ENTTEC driver and the
# on-screen visualiser are compiled straight from the sibling hitnotedmx repo —
# hitnotedmx is the backbone and the single source of truth for the look, so
# this app never carries a fork of them. hitnotedmx itself is not modified.
set(HITNOTEDMX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../hitnotedmx" CACHE PATH
    "Path to the hitnotedmx checkout whose engine sources this app builds against")
if(NOT EXISTS "${HITNOTEDMX_DIR}/Source/Composition.cpp")
    message(FATAL_ERROR "hitnotedmx engine not found at ${HITNOTEDMX_DIR} — set -DHITNOTEDMX_DIR=...")
endif()
set(HND_SRC "${HITNOTEDMX_DIR}/Source")

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
    Source/LiveEngine.cpp
    Source/Surfaces.cpp
    # shared engine (hitnotedmx)
    ${HND_SRC}/Composition.cpp
    ${HND_SRC}/Recipes.cpp
    ${HND_SRC}/MidiState.cpp
    ${HND_SRC}/EnttecProDmx.cpp
    ${HND_SRC}/DmxVisualizer.cpp
    ${HND_SRC}/TriggerVocabulary.cpp
)

# Own sources first so nothing here is shadowed by a same-named engine header.
target_include_directories(HitLaunchDmx PRIVATE Source ${HND_SRC})

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

# ── Headless engine check (CTest) ────────────────────────────────────────────
# Drives LiveEngine::tick with a synthetic clock — no GUI, no MIDI devices,
# ENTTEC not connected — and asserts the Launchpad/XL state produces the
# expected hitnotedmx output (latching, routes, faders, clock, looks, state).
juce_add_console_app(EngineCheck PRODUCT_NAME "engine-check")
target_sources(EngineCheck PRIVATE
    tools/EngineCheck.cpp
    Source/LiveEngine.cpp
    ${HND_SRC}/Composition.cpp
    ${HND_SRC}/Recipes.cpp
    ${HND_SRC}/MidiState.cpp
    ${HND_SRC}/EnttecProDmx.cpp
)
target_include_directories(EngineCheck PRIVATE Source ${HND_SRC})
target_compile_definitions(EngineCheck PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(EngineCheck PRIVATE
    "-framework IOKit"
    "-framework CoreFoundation"
    juce::juce_events
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
)

enable_testing()
add_test(NAME engine-check COMMAND EngineCheck)
