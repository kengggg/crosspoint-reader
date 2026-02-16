cmake_minimum_required(VERSION 3.13)
project(crosspoint-simulator C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

# ------------------------------------------------------------------
# Path to the firmware source tree (one level up from simulator/)
# ------------------------------------------------------------------
set(FW ${CMAKE_CURRENT_SOURCE_DIR}/..)

# ------------------------------------------------------------------
# Compiler definitions matching platformio.ini
# ------------------------------------------------------------------
add_definitions(
  -DCROSSPOINT_EMULATED=1
  -DMINIZ_NO_ZLIB_COMPATIBLE_NAMES=1
  -DEINK_DISPLAY_SINGLE_BUFFER_MODE=1
  -DXML_GE=0
  -DXML_CONTEXT_BYTES=1024
  -DUSE_UTF8_LONG_NAMES=1
  -DOMIT_FONTS=0
)
# CROSSPOINT_VERSION needs special quoting to survive CMake → compiler
add_compile_definitions(CROSSPOINT_VERSION="web-simulator")

# ------------------------------------------------------------------
# Include paths — shims FIRST so they override real SDK/Arduino headers
# ------------------------------------------------------------------
include_directories(
  # Simulator shims (searched first — intercepts Arduino.h, etc.)
  ${CMAKE_CURRENT_SOURCE_DIR}/shims/arduino
  ${CMAKE_CURRENT_SOURCE_DIR}/shims/esp
  ${CMAKE_CURRENT_SOURCE_DIR}/shims            # For <freertos/FreeRTOS.h> etc.
  ${CMAKE_CURRENT_SOURCE_DIR}/shims/freertos
  ${CMAKE_CURRENT_SOURCE_DIR}/shims/network
  ${CMAKE_CURRENT_SOURCE_DIR}/shims/sdk
  ${CMAKE_CURRENT_SOURCE_DIR}/shims/thirdparty

  # Firmware library headers
  ${FW}/lib/hal
  ${FW}/lib/GfxRenderer
  ${FW}/lib/EpdFont
  ${FW}/lib/EpdFont/builtinFonts
  ${FW}/lib/Epub
  ${FW}/lib/Epub/Epub
  ${FW}/lib/Epub/Epub/blocks
  ${FW}/lib/Epub/Epub/css
  ${FW}/lib/Epub/Epub/parsers
  ${FW}/lib/Epub/Epub/hyphenation
  ${FW}/lib/Txt
  ${FW}/lib/Xtc
  ${FW}/lib/Xtc/Xtc
  ${FW}/lib/ZipFile
  ${FW}/lib/FsHelpers
  ${FW}/lib/expat
  ${FW}/lib/miniz
  ${FW}/lib/picojpeg
  ${FW}/lib/Utf8
  ${FW}/lib/Serialization
  ${FW}/lib/KOReaderSync
  ${FW}/lib/OpdsParser
  ${FW}/lib/JpegToBmpConverter

  # Firmware src headers
  ${FW}/src
  ${FW}/src/activities
  ${FW}/src/activities/boot_sleep
  ${FW}/src/activities/browser
  ${FW}/src/activities/home
  ${FW}/src/activities/network
  ${FW}/src/activities/reader
  ${FW}/src/activities/settings
  ${FW}/src/activities/util
  ${FW}/src/components
  ${FW}/src/components/themes
  ${FW}/src/components/themes/lyra
  ${FW}/src/network
  ${FW}/src/images
  ${FW}/src/util
)

# ------------------------------------------------------------------
# Source files — firmware .cpp files (excluding HAL .cpp files we replace)
# ------------------------------------------------------------------

# Firmware src/ .cpp files
set(FW_SRC
  ${FW}/src/main.cpp
  ${FW}/src/CrossPointSettings.cpp
  ${FW}/src/CrossPointState.cpp
  ${FW}/src/MappedInputManager.cpp
  ${FW}/src/RecentBooksStore.cpp
  ${FW}/src/WifiCredentialStore.cpp
  ${FW}/src/activities/ActivityWithSubactivity.cpp
  ${FW}/src/activities/boot_sleep/BootActivity.cpp
  ${FW}/src/activities/boot_sleep/SleepActivity.cpp
  ${FW}/src/activities/browser/OpdsBookBrowserActivity.cpp
  ${FW}/src/activities/home/HomeActivity.cpp
  ${FW}/src/activities/home/MyLibraryActivity.cpp
  ${FW}/src/activities/home/RecentBooksActivity.cpp
  ${FW}/src/activities/network/CalibreConnectActivity.cpp
  ${FW}/src/activities/network/CrossPointWebServerActivity.cpp
  ${FW}/src/activities/network/NetworkModeSelectionActivity.cpp
  ${FW}/src/activities/network/WifiSelectionActivity.cpp
  ${FW}/src/activities/reader/EpubReaderActivity.cpp
  ${FW}/src/activities/reader/EpubReaderChapterSelectionActivity.cpp
  ${FW}/src/activities/reader/EpubReaderMenuActivity.cpp
  ${FW}/src/activities/reader/EpubReaderPercentSelectionActivity.cpp
  ${FW}/src/activities/reader/KOReaderSyncActivity.cpp
  ${FW}/src/activities/reader/ReaderActivity.cpp
  ${FW}/src/activities/reader/TxtReaderActivity.cpp
  ${FW}/src/activities/reader/XtcReaderActivity.cpp
  ${FW}/src/activities/reader/XtcReaderChapterSelectionActivity.cpp
  ${FW}/src/activities/settings/ButtonRemapActivity.cpp
  ${FW}/src/activities/settings/CalibreSettingsActivity.cpp
  ${FW}/src/activities/settings/ClearCacheActivity.cpp
  ${FW}/src/activities/settings/KOReaderAuthActivity.cpp
  ${FW}/src/activities/settings/KOReaderSettingsActivity.cpp
  ${FW}/src/activities/settings/OtaUpdateActivity.cpp
  ${FW}/src/activities/settings/SettingsActivity.cpp
  ${FW}/src/activities/util/FullScreenMessageActivity.cpp
  ${FW}/src/activities/util/KeyboardEntryActivity.cpp
  ${FW}/src/components/UITheme.cpp
  ${FW}/src/components/themes/BaseTheme.cpp
  ${FW}/src/components/themes/lyra/LyraTheme.cpp
  # Network .cpp files excluded — heavy ESP32/ArduinoJson dependencies.
  # Stub implementations provided in simulator/shims/network/network_heavy_stubs.cpp
  ${FW}/src/util/ButtonNavigator.cpp
  ${FW}/src/util/StringUtils.cpp
  ${FW}/src/util/UrlUtils.cpp
)

# Firmware lib/ .cpp files
set(FW_LIB
  ${FW}/lib/EpdFont/EpdFont.cpp
  ${FW}/lib/EpdFont/EpdFontFamily.cpp
  ${FW}/lib/GfxRenderer/GfxRenderer.cpp
  ${FW}/lib/GfxRenderer/Bitmap.cpp
  ${FW}/lib/GfxRenderer/BitmapHelpers.cpp
  ${FW}/lib/Epub/Epub.cpp
  ${FW}/lib/Epub/Epub/BookMetadataCache.cpp
  ${FW}/lib/Epub/Epub/Page.cpp
  ${FW}/lib/Epub/Epub/ParsedText.cpp
  ${FW}/lib/Epub/Epub/Section.cpp
  ${FW}/lib/Epub/Epub/blocks/TextBlock.cpp
  ${FW}/lib/Epub/Epub/css/CssParser.cpp
  ${FW}/lib/Epub/Epub/hyphenation/HyphenationCommon.cpp
  ${FW}/lib/Epub/Epub/hyphenation/Hyphenator.cpp
  ${FW}/lib/Epub/Epub/hyphenation/LanguageRegistry.cpp
  ${FW}/lib/Epub/Epub/hyphenation/LiangHyphenation.cpp
  ${FW}/lib/Epub/Epub/htmlEntities.cpp
  ${FW}/lib/Epub/Epub/parsers/ChapterHtmlSlimParser.cpp
  ${FW}/lib/Epub/Epub/parsers/ContainerParser.cpp
  ${FW}/lib/Epub/Epub/parsers/ContentOpfParser.cpp
  ${FW}/lib/Epub/Epub/parsers/TocNavParser.cpp
  ${FW}/lib/Epub/Epub/parsers/TocNcxParser.cpp
  ${FW}/lib/FsHelpers/FsHelpers.cpp
  ${FW}/lib/JpegToBmpConverter/JpegToBmpConverter.cpp
  ${FW}/lib/KOReaderSync/KOReaderCredentialStore.cpp
  ${FW}/lib/KOReaderSync/KOReaderDocumentId.cpp
  # KOReaderSyncClient.cpp excluded — needs HTTPClient/ArduinoJson; stub in network_heavy_stubs.cpp
  ${FW}/lib/KOReaderSync/ProgressMapper.cpp
  ${FW}/lib/OpdsParser/OpdsParser.cpp
  ${FW}/lib/OpdsParser/OpdsStream.cpp
  ${FW}/lib/Txt/Txt.cpp
  ${FW}/lib/Utf8/Utf8.cpp
  ${FW}/lib/Xtc/Xtc.cpp
  ${FW}/lib/Xtc/Xtc/XtcParser.cpp
  ${FW}/lib/ZipFile/ZipFile.cpp
)

# C source files (expat, miniz, picojpeg)
set(FW_C
  ${FW}/lib/expat/xmlparse.c
  ${FW}/lib/expat/xmlrole.c
  ${FW}/lib/expat/xmltok.c
  ${FW}/lib/miniz/miniz.c
  ${FW}/lib/picojpeg/picojpeg.c
)

# Simulator-specific source files
set(EMU_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/main_simulator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shims/arduino/arduino_shim.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shims/freertos/freertos_shim.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shims/network/network_stubs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shims/network/network_heavy_stubs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hal/HalDisplay_Web.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hal/HalGPIO_Web.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hal/HalStorage_Web.cpp
)

# ------------------------------------------------------------------
# Build target
# ------------------------------------------------------------------
add_executable(crosspoint ${FW_SRC} ${FW_LIB} ${FW_C} ${EMU_SRC})

# C files need include paths too
set_source_files_properties(${FW_C} PROPERTIES LANGUAGE C)

# PlatformIO with Arduino framework force-includes Arduino.h in every TU.
# Replicate that so firmware code gets uint8_t, millis, Serial, etc.
target_compile_options(crosspoint PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-include$<SEMICOLON>Arduino.h>
  -Wno-deprecated-declarations
  -Wno-unused-variable
  -Wno-unused-function
  -Wno-sign-compare
  -Wno-unused-but-set-variable
)

# ------------------------------------------------------------------
# Emscripten-specific link flags
# ------------------------------------------------------------------
if(EMSCRIPTEN)
  set_target_properties(crosspoint PROPERTIES
    SUFFIX ".js"
    LINK_FLAGS "\
      -sASYNCIFY=1 \
      -sASYNCIFY_STACK_SIZE=131072 \
      -sALLOW_MEMORY_GROWTH=1 \
      -sINITIAL_MEMORY=33554432 \
      -sEXPORTED_FUNCTIONS=['_main','_simulator_button_down','_simulator_button_up'] \
      -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','HEAPU8','FS'] \
      -sFORCE_FILESYSTEM=1 \
      -sNO_EXIT_RUNTIME=1 \
      -sASSERTIONS=1 \
      -O2 \
    "
  )

  # Copy shell assets to build directory
  add_custom_command(TARGET crosspoint POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_SOURCE_DIR}/shell/index.html
      ${CMAKE_CURRENT_BINARY_DIR}/index.html
    COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_SOURCE_DIR}/shell/shell.js
      ${CMAKE_CURRENT_BINARY_DIR}/shell.js
    COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_SOURCE_DIR}/shell/style.css
      ${CMAKE_CURRENT_BINARY_DIR}/style.css
  )
endif()
