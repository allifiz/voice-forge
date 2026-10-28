cmake_minimum_required(VERSION 3.20)
project(VoiceForgeLite VERSION 0.1.2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(miniaudio
  GIT_REPOSITORY https://github.com/mackron/miniaudio.git
  GIT_TAG master
  GIT_SHALLOW TRUE
)
FetchContent_Declare(signalsmith_stretch
  GIT_REPOSITORY https://github.com/Signalsmith-Audio/signalsmith-stretch.git
  GIT_TAG main
  GIT_SHALLOW TRUE
)
# Signalsmith Stretch includes "signalsmith-linear/stft.h".
# This SOURCE_DIR gives it the required directory name.
FetchContent_Declare(signalsmith_linear
  GIT_REPOSITORY https://github.com/Signalsmith-Audio/linear.git
  GIT_TAG main
  GIT_SHALLOW TRUE
  SOURCE_DIR "${CMAKE_BINARY_DIR}/_deps/signalsmith-linear"
)

FetchContent_GetProperties(miniaudio)
if(NOT miniaudio_POPULATED)
  FetchContent_Populate(miniaudio)
endif()
FetchContent_GetProperties(signalsmith_stretch)
if(NOT signalsmith_stretch_POPULATED)
  FetchContent_Populate(signalsmith_stretch)
endif()
FetchContent_GetProperties(signalsmith_linear)
if(NOT signalsmith_linear_POPULATED)
  FetchContent_Populate(signalsmith_linear)
endif()

add_executable(voiceforge src/main.cpp)

# Avoid Windows min/max preprocessor macros corrupting std::min/max.
if(WIN32)
  target_compile_definitions(voiceforge PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

target_include_directories(voiceforge PRIVATE
  "${miniaudio_SOURCE_DIR}"
  "${signalsmith_stretch_SOURCE_DIR}"
  "${CMAKE_BINARY_DIR}/_deps"
)

if(MSVC)
  target_compile_options(voiceforge PRIVATE /O2 /W4 /EHsc)
else()
  target_compile_options(voiceforge PRIVATE -O3 -Wall -Wextra -Wpedantic)
  if(UNIX AND NOT APPLE)
    target_link_libraries(voiceforge PRIVATE m dl pthread)
  endif()
endif()
