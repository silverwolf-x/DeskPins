cmake_minimum_required(VERSION 3.21)

set(DP_VERSION "1.33.0" CACHE STRING "DeskPins version (major.minor.patch)")
if(NOT DP_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "DP_VERSION must be major.minor.patch, got '${DP_VERSION}'")
endif()
project(DeskPins VERSION ${DP_VERSION} LANGUAGES CXX RC)

if(NOT MSVC)
    message(FATAL_ERROR "DeskPins is built with MSVC only")
endif()

# static CRT: the exe has no runtime dependencies besides Windows itself
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# optimize Release builds for size
string(REPLACE "/O2" "/O1" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

configure_file(src/dp_version.h.in ${CMAKE_CURRENT_BINARY_DIR}/dp_version.h @ONLY)

add_executable(DeskPins WIN32
    src/app.cpp
    src/apputils.cpp
    src/deskpins.cpp
    src/help.cpp
    src/mainwnd.cpp
    src/optautopin.cpp
    src/opthotkeys.cpp
    src/options.cpp
    src/optionsdlg.cpp
    src/optlang.cpp
    src/optpins.cpp
    src/pinlayerwnd.cpp
    src/pinshape.cpp
    src/pinwnd.cpp
    src/trayicon.cpp
    src/util.cpp
    src/deskpins.rc
)

target_compile_features(DeskPins PRIVATE cxx_std_23)
target_include_directories(DeskPins PRIVATE src ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(DeskPins PRIVATE UNICODE _UNICODE)
target_compile_options(DeskPins PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:/W3 /utf-8 /permissive- /EHsc>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:Release>>:/GL /Gy>
)
target_link_options(DeskPins PRIVATE
    $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
)
target_link_libraries(DeskPins PRIVATE comctl32 shlwapi dwmapi)
