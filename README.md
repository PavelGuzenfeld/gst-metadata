# gst-metadata

Composable, type-safe, header-only GStreamer buffer metadata using C++20 CRTP templates.

[![CI](https://github.com/PavelGuzenfeld/gst-metadata/actions/workflows/ci.yml/badge.svg)](https://github.com/PavelGuzenfeld/gst-metadata/actions/workflows/ci.yml)
[![Release](https://github.com/PavelGuzenfeld/gst-metadata/releases/latest/badge.svg)](https://github.com/PavelGuzenfeld/gst-metadata/releases/latest)

## Overview

`gst-metadata` provides a single CRTP base class, `gstmeta::MetaBase`, that lets you define custom GStreamer buffer metadata types in a few lines of code. Each metadata type gets its own `GType`, so multiple metadata types coexist on the same `GstBuffer` and can be queried independently. Metadata survives passthrough elements (`identity`, `videoconvert`, `videoscale`, `tee`, etc.) automatically.

## Key Features

- **Header-only** -- drop `include/gst-metadata/` into your project or use CMake FetchContent.
- **Composable** -- attach multiple independent metadata types to a single buffer.
- **Type-safe** -- each metadata type is a distinct C++ type with its own GType registration.
- **Forward-compatible versioning** -- a `MetaHeader` stores version and data size so readers compiled against older structs can safely read buffers from newer writers.
- **Zero-copy** -- metadata is allocated inline in the GStreamer buffer metadata system; no heap allocations.

## Quick Start

Define a custom metadata type in three steps:

```cpp
#include <gst-metadata/meta_base.hpp>

// 1. Define your POD payload
struct ImuData {
    std::uint64_t timestamp_ns{};
    float accel_x{}, accel_y{}, accel_z{};
    float gyro_x{}, gyro_y{}, gyro_z{};
};

// 2. Declare unique GType name strings
inline constexpr char ImuApiName[]  = "GstImuMetaAPI";
inline constexpr char ImuInfoName[] = "GstImuMetaInfo";

// 3. Derive from MetaBase via CRTP
class ImuMeta : public gstmeta::MetaBase<ImuMeta, ImuData, ImuApiName, ImuInfoName> {
public:
    static constexpr std::uint32_t current_version() { return 1; }
};
```

Then use it in your GStreamer element:

```cpp
// Write
ImuMeta::add(buffer, ImuData{.timestamp_ns = 42, .accel_z = -9.81f});

// Read
auto imu = ImuMeta::get(buffer);  // std::optional<ImuData>

// Iterate all instances
ImuMeta::for_each(buffer, [](const ImuData& d) { /* ... */ });
```

## Integration

### CMake FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(gst-metadata
    GIT_REPOSITORY https://github.com/PavelGuzenfeld/gst-metadata.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(gst-metadata)

target_link_libraries(my_target PRIVATE gstmeta::gstmeta)
```

### System Install

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release \
    -DGST_METADATA_BUILD_EXAMPLES=OFF \
    -DGST_METADATA_BUILD_TESTS=OFF
cmake --build build
sudo cmake --install build
```

Then in your project:

```cmake
find_package(gst-metadata REQUIRED)
target_link_libraries(my_target PRIVATE gstmeta::gstmeta)
```

## Building

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug \
    -DGST_METADATA_BUILD_EXAMPLES=ON \
    -DGST_METADATA_BUILD_TESTS=ON
cmake --build build -j$(nproc)
```

### Dependencies

- CMake 3.16+
- GStreamer 1.0 development packages:
  ```bash
  sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
      gstreamer1.0-tools gstreamer1.0-plugins-base gstreamer1.0-plugins-good
  ```

## Testing

### Unit Tests

Three test suites covering core functionality, edge cases, and fuzz/stress:

```bash
ctest --test-dir build --output-on-failure
```

### Sanitizer Builds

Built-in CMake options for AddressSanitizer + UndefinedBehaviorSanitizer and ThreadSanitizer:

```bash
# ASan + UBSan
cmake -B build-asan -DCMAKE_BUILD_TYPE=Debug \
    -DGST_METADATA_BUILD_TESTS=ON -DGST_METADATA_SANITIZER_ASAN=ON
cmake --build build-asan -j$(nproc)
ctest --test-dir build-asan --output-on-failure

# TSan
cmake -B build-tsan -DCMAKE_BUILD_TYPE=Debug \
    -DGST_METADATA_BUILD_TESTS=ON -DGST_METADATA_SANITIZER_TSAN=ON
cmake --build build-tsan -j$(nproc)
ctest --test-dir build-tsan --output-on-failure
```

### Integration Tests (Docker)

The integration tests run GStreamer pipelines end-to-end in a Docker container:

```bash
docker build --target test -t gst-metadata-test .
docker run --rm gst-metadata-test
```

This builds the library and example plugins in a multi-stage Docker build, runs unit tests with sanitizers, then exercises 13 pipeline scenarios covering metadata attachment, compartmentalization, survival through transforms, tee fan-out, and data integrity.

## Example Pipeline

```bash
GST_PLUGIN_PATH=build/examples gst-launch-1.0 \
    videotestsrc num-buffers=5 ! imuwriter ! cropwriter ! fovwriter ! \
    imureader ! cropreader ! fovreader ! fakesink
```

## API Reference

All methods are static on your derived class (e.g., `ImuMeta::add(...)`) via `MetaBase`.

| Method | Description |
|---|---|
| `add(GstBuffer*, const DataType&)` | Attach metadata to a buffer. Returns `Storage*`. |
| `get(GstBuffer*)` | Retrieve the first instance. Returns `std::optional<DataType>`. |
| `get_mut(GstBuffer*)` | Retrieve a mutable `Storage*` pointer, or `nullptr`. |
| `for_each(GstBuffer*, fn)` | Iterate all instances of this type on the buffer. |
| `get_all(GstBuffer*)` | Collect all instances into a `std::vector<DataType>`. |
| `remove(GstBuffer*)` | Remove the first instance. Returns `bool`. |
| `count(GstBuffer*)` | Count how many instances are attached. |
| `api_type()` | Get the `GType` for this metadata API. |
| `info()` | Get the `GstMetaInfo*` for this metadata type. |

## License

LGPL -- see source files for details.
