#pragma once

#include "meta_base.hpp"

#include <cstdint>

namespace gstmeta {

/// IMU sensor payload.
struct ImuData {
    std::uint64_t timestamp_ns{};
    float accel_x{};
    float accel_y{};
    float accel_z{};
    float gyro_x{};
    float gyro_y{};
    float gyro_z{};
    float temperature{};
    float pressure{};
    float humidity{};
    float altitude{};
};

// Unique names — file-scope linkage, used as template args.
inline constexpr char ImuApiName[] = "GstImuMetaAPI";
inline constexpr char ImuInfoName[] = "GstImuMetaInfo";

class ImuMeta : public MetaBase<ImuMeta, ImuData, ImuApiName, ImuInfoName> {
public:
    static constexpr std::uint32_t current_version() { return 1; }
};

} // namespace gstmeta
