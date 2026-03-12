cmake_minimum_required(VERSION 3.16)
project(gst-metadata
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Dependencies -----------------------------------------------------------
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED IMPORTED_TARGET
    gstreamer-1.0
    gstreamer-base-1.0
)

# --- Header-only library target ---------------------------------------------
# The core library is header-only (CRTP templates).
# Consumers link this target to get include paths + GStreamer deps.
add_library(gst-metadata INTERFACE)
target_include_directories(gst-metadata INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(gst-metadata INTERFACE PkgConfig::GST)

# Alias for use from parent CMake projects (e.g. FetchContent)
add_library(gstmeta::gstmeta ALIAS gst-metadata)

# --- Sanitizers -------------------------------------------------------------
option(GST_METADATA_SANITIZER_ASAN "Enable AddressSanitizer + UndefinedBehaviorSanitizer" OFF)
option(GST_METADATA_SANITIZER_TSAN "Enable ThreadSanitizer" OFF)

if(GST_METADATA_SANITIZER_ASAN AND GST_METADATA_SANITIZER_TSAN)
    message(FATAL_ERROR "ASan and TSan cannot be used together")
endif()

if(GST_METADATA_SANITIZER_ASAN)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

if(GST_METADATA_SANITIZER_TSAN)
    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
    add_link_options(-fsanitize=thread)
endif()

# --- Example GStreamer plugins ----------------------------------------------
option(GST_METADATA_BUILD_EXAMPLES "Build example GStreamer plugins" ON)
if(GST_METADATA_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# --- Tests ------------------------------------------------------------------
option(GST_METADATA_BUILD_TESTS "Build tests" ON)
if(GST_METADATA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# --- Install ----------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS gst-metadata
    EXPORT gst-metadata-targets
)
install(DIRECTORY include/gst-metadata/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gst-metadata
)
install(EXPORT gst-metadata-targets
    FILE gst-metadata-targets.cmake
    NAMESPACE gstmeta::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/gst-metadata
)

# --- pkg-config .pc file ----------------------------------------------------
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/gst-metadata.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/gst-metadata.pc
    @ONLY
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/gst-metadata.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
