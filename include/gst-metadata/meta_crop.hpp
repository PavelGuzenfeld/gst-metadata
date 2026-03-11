#pragma once

#include "meta_base.hpp"

#include <cstdint>

namespace gstmeta {

/// Crop region payload.
struct CropData {
    std::int32_t top{};
    std::int32_t bottom{};
    std::int32_t left{};
    std::int32_t right{};
};

inline constexpr char CropApiName[] = "GstCropMetaAPI";
inline constexpr char CropInfoName[] = "GstCropMetaInfo";

class CropMeta : public MetaBase<CropMeta, CropData, CropApiName, CropInfoName> {
public:
    static constexpr std::uint32_t current_version() { return 1; }
};

} // namespace gstmeta
