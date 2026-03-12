# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2025-12-01

### Added

- CRTP base class `gstmeta::MetaBase` for defining custom GStreamer buffer metadata types.
- `MetaHeader` with version and data_size fields for forward-compatible versioning.
- Full static API: `add`, `get`, `get_mut`, `for_each`, `get_all`, `remove`, `count`.
- Thread-safe GType registration via `g_once_init_enter`/`g_once_init_leave`.
- Automatic metadata survival through passthrough elements (identity, videoconvert, videoscale, tee, etc.).
- Three example metadata types: ImuMeta, CropMeta, FovMeta.
- Example GStreamer writer and reader plugins.
- Unit tests: core composition, edge cases (null buffers, empty buffers, read-only memory, large instance counts), and fuzz/stress tests (10,000 random operations, float edge values).
- Docker-based integration tests: 13 pipeline scenarios covering attachment, survival, fan-out, deep chains, and data integrity.
- CI with Debug/Release matrix, ASan+UBSan, TSan, cppcheck, clang-tidy.
- Tag-triggered release workflow with source tarballs and GitHub Releases.
- CMake install targets with `find_package(gst-metadata)` support.
- FetchContent integration via `gstmeta::gstmeta` alias target.
- MIT license.

[0.1.0]: https://github.com/PavelGuzenfeld/gst-metadata/releases/tag/v0.1.0
