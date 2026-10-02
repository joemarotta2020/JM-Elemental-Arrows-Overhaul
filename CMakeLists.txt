cmake_minimum_required(VERSION 3.21)

project(
    JM_ElementalArrows
    VERSION 0.1.0
    DESCRIPTION "Native soul-gem instance support for JM Elemental Arrows Overhaul on Skyrim SE 1.5.97"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

include(FetchContent)

# Joe's load order is intentionally Skyrim SE 1.5.97 / SKSE 2.0.20.
set(ENABLE_SKYRIM_SE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_AE OFF CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    CommonLibSSE
    GIT_REPOSITORY https://github.com/CharmedBaryon/CommonLibSSE-NG.git
    GIT_TAG v3.7.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(CommonLibSSE)

add_commonlibsse_plugin(JM_ElementalArrows
    NAME "JM_ElementalArrows"
    VERSION ${PROJECT_VERSION}
    COMPATIBLE_RUNTIMES "1.5.97"
    SOURCES
        src/main.cpp
)

target_precompile_headers(JM_ElementalArrows PRIVATE src/PCH.h)
target_include_directories(JM_ElementalArrows PRIVATE src)
target_compile_definitions(JM_ElementalArrows PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)

install(TARGETS JM_ElementalArrows DESTINATION SKSE/Plugins)
