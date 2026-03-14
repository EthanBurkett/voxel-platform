cmake_minimum_required(VERSION 3.24)

project(MinecraftConsoles LANGUAGES C CXX RC ASM_MASM)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
  message(FATAL_ERROR "This CMake build currently supports Windows only.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Use a 64-bit generator/toolchain (x64).")
endif()

# ---------------------------------------------------------------------------
# Node.js paths — must be defined before ClientSources.cmake is included
# because that file references NODE_ROOT in the sources list.
#
# IMPORTANT: This project must be configured and built with the *same* Visual
# Studio (and toolset) used to build Node.js. Node 24+ is built with VS 2026
# (e.g. vcbuild). If you see LNK2019 for __std_rotate, __std_find_*_trivial_*,
# __std_unique_4, or __std_find_last_not_ch_pos_1, you are building
# MinecraftConsoles with an older VS (e.g. 2022) while Node libs are from VS
# 2026. Fix: open "x64 Native Tools Command Prompt for VS 2026", then run
# CMake configure + build from that shell so this project uses the same toolset.
# ---------------------------------------------------------------------------
set(NODE_ROOT
  "C:/Users/Ethan/Documents/Projects/MCCE-Modding/node-main"
  CACHE PATH "Root of the Node.js source tree")

set(NODE_LIB_DIR "${NODE_ROOT}/out/Release/lib"
  CACHE PATH "Node.js MSVC Release .lib directory")

# ---------------------------------------------------------------------------
# Node libs are built with /MT (no debug CRT). Force the whole project to
# match so we don't get LNK2038 CRT mismatch errors.
# ---------------------------------------------------------------------------
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

function(configure_msvc_target target)
  target_compile_options(${target} PRIVATE
    $<$<AND:$<NOT:$<CONFIG:Release>>,$<COMPILE_LANGUAGE:C,CXX>>:/W3>
    $<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:C,CXX>>:/W0>
    $<$<COMPILE_LANGUAGE:C,CXX>:/MP>
    $<$<COMPILE_LANGUAGE:C,CXX>:/FS>
    $<$<COMPILE_LANGUAGE:CXX>:/EHsc>
    # Note: /GL removed — conflicts with Node's libs at link time (/LTCG also removed below)
    $<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:C,CXX>>:/O2 /Oi /GT /GF>
  )
endfunction()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/WorldSources.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ClientSources.cmake")

list(TRANSFORM MINECRAFT_WORLD_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.World/")
list(TRANSFORM MINECRAFT_CLIENT_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/")

# Append extra sources AFTER the transform so they don't get the prefix mangled.
list(APPEND MINECRAFT_CLIENT_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/Xbox/MinecraftWindows.rc"
  "${NODE_ROOT}/src/node_snapshot_stub.cc"
)

# ---------------------------------------------------------------------------
# MinecraftWorld static lib
# ---------------------------------------------------------------------------
add_library(MinecraftWorld STATIC ${MINECRAFT_WORLD_SOURCES})
target_include_directories(MinecraftWorld PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.World"
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.World/x64headers"
)
target_compile_definitions(MinecraftWorld PRIVATE
  $<$<CONFIG:Debug>:_LARGE_WORLDS;_DEBUG_MENUS_ENABLED;_DEBUG;_LIB;_CRT_NON_CONFORMING_SWPRINTFS;_CRT_SECURE_NO_WARNINGS;_WINDOWS64>
  $<$<NOT:$<CONFIG:Debug>>:_LARGE_WORLDS;_DEBUG_MENUS_ENABLED;_LIB;_CRT_NON_CONFORMING_SWPRINTFS;_CRT_SECURE_NO_WARNINGS;_WINDOWS64>
)
if(MSVC)
  configure_msvc_target(MinecraftWorld)
endif()

# ---------------------------------------------------------------------------
# MinecraftClient executable
# ---------------------------------------------------------------------------
add_executable(MinecraftClient WIN32 ${MINECRAFT_CLIENT_SOURCES})

target_include_directories(MinecraftClient PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client"
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/Windows64/Iggy/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/Xbox/Sentient/Include"
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.World/x64headers"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/"
  # ModLoader headers
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/ModLoader"
  # Node.js headers
  "${NODE_ROOT}/src"
  "${NODE_ROOT}/deps/v8/include"
  "${NODE_ROOT}/deps/uv/include"
)

target_compile_definitions(MinecraftClient PRIVATE
  # Suppress iterator debug level mismatch with Node's /MT libs
  _ITERATOR_DEBUG_LEVEL=0
  $<$<CONFIG:Debug>:_LARGE_WORLDS;_DEBUG_MENUS_ENABLED;_DEBUG;_CRT_NON_CONFORMING_SWPRINTFS;_CRT_SECURE_NO_WARNINGS;_WINDOWS64>
  $<$<NOT:$<CONFIG:Debug>>:_LARGE_WORLDS;_DEBUG_MENUS_ENABLED;_CRT_NON_CONFORMING_SWPRINTFS;_CRT_SECURE_NO_WARNINGS;_WINDOWS64>
)

# ModLoader files need C++20 and /Zc:__cplusplus for Node/V8 headers.
# Set per-file so MC's own files stay on C++17.
set_source_files_properties(
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/ModLoader/engine/event_bus.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/ModLoader/engine/game_bridge.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/ModLoader/engine/module_registry.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/ModLoader/engine/mod_objects.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/ModLoader/engine/node_host.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/ModLoader/engine/type_generator.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/ModLoader/modules/engine_module.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/ModLoader/modules/mc_module.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/Windows64/Windows64_Minecraft.cpp"
  "${NODE_ROOT}/src/node_snapshot_stub.cc"
  PROPERTIES COMPILE_OPTIONS "/Zc:__cplusplus;/std:c++20;/DNOMINMAX;/DWIN32_LEAN_AND_MEAN"
)

if(MSVC)
  configure_msvc_target(MinecraftClient)
  target_link_options(MinecraftClient PRIVATE
    # /LTCG removed — Node libs are not built with /GL, mixing causes LNK1218
    $<$<CONFIG:Release>:/INCREMENTAL:NO>
    # Node requires these or symbols get stripped from the static libs
    "/WHOLEARCHIVE:libnode"
    "/WHOLEARCHIVE:v8_base_without_compiler"
  )
endif()

set_target_properties(MinecraftClient PROPERTIES
  VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:MinecraftClient>"
)

# ---------------------------------------------------------------------------
# Link libraries
# ---------------------------------------------------------------------------
target_link_directories(MinecraftClient PRIVATE "${NODE_LIB_DIR}")

target_link_libraries(MinecraftClient PRIVATE
  MinecraftWorld

  # MC platform libs
  d3d11
  XInput9_1_0
  wsock32
  legacy_stdio_definitions
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/Windows64/Iggy/lib/iggy_w64.lib"
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/Windows64/Iggy/lib/iggyperfmon_w64.lib"
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/Windows64/Iggy/lib/iggyexpruntime_w64.lib"
  $<$<CONFIG:Debug>:
    "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/Windows64/4JLibs/libs/4J_Input_d.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/Windows64/4JLibs/libs/4J_Storage_d.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/Windows64/4JLibs/libs/4J_Render_PC_d.lib"
  >
  $<$<NOT:$<CONFIG:Debug>>:
    "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/Windows64/4JLibs/libs/4J_Input.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/Windows64/4JLibs/libs/4J_Storage.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/Windows64/4JLibs/libs/4J_Render_PC.lib"
  >

  # Node.js + V8 libs
  libnode
  v8_base_without_compiler
  v8_compiler
  v8_init
  v8_initializers
  v8_initializers_slow
  v8_libbase
  v8_libplatform
  v8_snapshot
  v8_zlib
  libuv
  uvwasi
  nghttp2
  cares
  openssl
  ncrypto
  zlib
  zlib_adler32_simd
  zlib_data_chunk_simd
  brotli
  zstd
  llhttp
  histogram
  nbytes
  abseil
  ada
  simdjson
  simdutf
  highway
  icudata
  icui18n
  icuucx
  icutools
  sqlite
  merve
  torque_base
  crdtp

  # Windows system libs required by Node
  Dbghelp
  winmm
  Ws2_32
  crypt32
  userenv
)

if(EXISTS "${NODE_LIB_DIR}/liblief.lib")
  target_link_libraries(MinecraftClient PRIVATE liblief)
endif()

# ---------------------------------------------------------------------------
# Asset copying
# ---------------------------------------------------------------------------
if(CMAKE_HOST_WIN32)
  message(STATUS "Starting redist copy...")
  execute_process(
    COMMAND robocopy.exe
            "${CMAKE_CURRENT_SOURCE_DIR}/x64/Release"
            "${CMAKE_CURRENT_BINARY_DIR}"
            /S /MT /R:0 /W:0 /NP
  )
  message(STATUS "Starting asset copy...")
  execute_process(
    COMMAND robocopy.exe
            "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client"
            "${CMAKE_CURRENT_BINARY_DIR}"
            /S /MT /R:0 /W:0 /NP
            /XF "*.cpp" "*.c" "*.h" "*.hpp" "*.asm"
                "*.xml" "*.lang" "*.vcxproj" "*.vcxproj.*" "*.sln"
                "*.docx" "*.xls"
                "*.bat" "*.cmd" "*.ps1" "*.py"
                "*Test*"
            /XD "Durango*" "Orbis*" "PS*" "Xbox"
  )
  message(STATUS "Patching Windows64Media...")
  execute_process(
    COMMAND robocopy.exe
            "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/DurangoMedia"
            "${CMAKE_CURRENT_BINARY_DIR}/Windows64Media"
            /S /MT /R:0 /W:0 /NP
            /XF "*.h" "*.xml" "*.lang" "*.bat"
  )
elseif(CMAKE_HOST_UNIX)
  message(STATUS "Starting redist copy...")
  execute_process(
    COMMAND rsync -av "${CMAKE_CURRENT_SOURCE_DIR}/x64/Release/" "${CMAKE_CURRENT_BINARY_DIR}/"
  )
  message(STATUS "Starting asset copy...")
  execute_process(
    COMMAND rsync -av
            "--exclude=*.cpp" "--exclude=*.c" "--exclude=*.h" "--exclude=*.hpp" "--exclude=*.asm"
            "--exclude=*.xml" "--exclude=*.lang" "--exclude=*.vcxproj" "--exclude=*.vcxproj.*" "--exclude=*.sln"
            "--exclude=*.docx" "--exclude=*.xls"
            "--exclude=*.bat" "--exclude=*.cmd" "--exclude=*.ps1" "--exclude=*.py"
            "--exclude=*Test*"
            "--exclude=Durango*" "--exclude=Orbis*" "--exclude=PS*" "--exclude=Xbox"
            "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/" "${CMAKE_CURRENT_BINARY_DIR}/"
  )
  message(STATUS "Patching Windows64Media...")
  execute_process(
    COMMAND rsync -av
            "--exclude=*.h" "--exclude=*.xml" "--exclude=*.lang" "--exclude=*.bat"
            "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/DurangoMedia/" "${CMAKE_CURRENT_BINARY_DIR}/Windows64Media/"
  )
else()
  message(FATAL_ERROR "Redist and asset copying is only supported on Windows (Robocopy) and Unix systems (rsync).")
endif()

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT MinecraftClient)