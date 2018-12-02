cmake_minimum_required(VERSION 3.10)

message("Generator: ${CMAKE_GENERATOR}")
if(${CMAKE_GENERATOR} STREQUAL "Xcode")
    set(IsXcode "1")
elseif(${CMAKE_GENERATOR} MATCHES "^Visual Studio .*")
    set(IsMSVC "1")
endif()

# GeneratorがVisual Studioの場合、Win64指定されているかどうかでプラットフォーム指定を分岐
# Xcodeでは常にx64を指定する
if(IsMSVC)
    if(${CMAKE_GENERATOR} MATCHES "Win64$")
        set(PLATFORM "x64")
        set(Isx64 "1")
    else()
        set(PLATFORM "x86")
        set(Isx86 "1")
    endif()
elseif(IsXcode)
    set(Isx64 "1")
    set(PLATFORM "x64")
endif()


if(NOT DEFINED CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

project("TimeStretchTest")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Werror=return-type")

string(TOLOWER ${CMAKE_BUILD_TYPE} LOWER_CONFIG_NAME)
string(TOUPPER ${CMAKE_BUILD_TYPE} UPPER_CONFIG_NAME)
string(SUBSTRING ${UPPER_CONFIG_NAME} 0 1 TMP1)
string(SUBSTRING ${LOWER_CONFIG_NAME} 1 -1 TMP2)
string(CONCAT CAPITAL_CONFIG_NAME ${TMP1} ${TMP2})
set(BUILD_DIR_NAME "build_${LOWER_CONFIG_NAME}")

if(${LOWER_CONFIG_NAME} STREQUAL "debug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_DEBUG")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_NDEBUG")
endif()

include_directories(
  "./src"
  "./ext/soundtouch/${BUILD_DIR_NAME}/install/include"
  "./ext/rubberband/rubberband"
  "./ext/AudioFile"
  )

set(SOURCE_DIR_LIST "./src" "./ext/AudioFile")
set(TARGET_EXTENSION_LIST "*.c" "*.cpp" "*.h" "*.hpp")
set(EXCLUDE_FILENAME_PATTERN_LIST "AudioFileTest")
set(BASE_DIR "./src")
get_filename_component(BASE_DIR "${BASE_DIR}" ABSOLUTE)

if(IsXcode)
  list(APPEND TARGET_EXTENSION_LIST "*.mm")
endif()

foreach(DIR ${SOURCE_DIR_LIST})
  set(PATTERNS "")
  foreach(EXT ${TARGET_EXTENSION_LIST})
    list(APPEND PATTERNS "${DIR}/${EXT}")
  endforeach()

  file(GLOB_RECURSE FILES ${PATTERNS})

  # Define SourceGroup reflecting filesystem hierarchy.
  foreach(FILE_PATH ${FILES})
    get_filename_component(ABS_PATH "${FILE_PATH}" ABSOLUTE)
    set(EXCLUDED 0)
    foreach(ENAME ${EXCLUDE_FILENAME_PATTERN_LIST})
      if(ABS_PATH MATCHES "${ENAME}")
        set(EXCLUDED 1)
      endif()
    endforeach()
    if(EXCLUDED)
      continue()
    endif()

    get_filename_component(DIR "${ABS_PATH}" DIRECTORY)
    file(RELATIVE_PATH GROUP_NAME "${BASE_DIR}" "${DIR}")
    string(REPLACE "/" "\\" GROUP_NAME "${GROUP_NAME}")
    source_group("${GROUP_NAME}" FILES "${ABS_PATH}")
    list(APPEND SOURCE_FILES "${ABS_PATH}")
  endforeach()
endforeach()

message("Files ${SOURCE_FILES}")

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

find_library(SOUNDTOUCH_LIB "soundtouch" PATHS "./ext/soundtouch/${BUILD_DIR_NAME}/install/lib/")
find_library(RUBBERBAND_LIB "librubberband.a" PATHS "./ext/rubberband/lib")

message("RubberBand: ${RUBBERBAND_LIB}")

target_link_libraries(
  ${PROJECT_NAME}
  ${SOUNDTOUCH_LIB}
  ${RUBBERBAND_LIB}
  "-framework Accelerate"
  )
