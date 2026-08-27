cmake_minimum_required(VERSION 3.20)
project(search_engine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# GCC builds on macOS default to an older deployment target than the installed
# SDK, and the linker then warns once per object file. Match the SDK instead.
if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
  execute_process(
    COMMAND xcrun --show-sdk-version
    OUTPUT_VARIABLE macos_sdk_version
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  if(macos_sdk_version)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "${macos_sdk_version}")
  endif()
endif()

add_executable(search
  src/main.cpp
  src/corpus.cpp
  src/document.cpp
  src/tokenizer.cpp
  src/normalize.cpp
  src/stopwords.cpp
  src/stemmer.cpp
  src/analyzer.cpp
  src/index.cpp
  src/query.cpp
  src/evaluator.cpp
  src/ranking.cpp
)

target_include_directories(search PRIVATE src)
target_compile_options(search PRIVATE -Wall -Wextra)
