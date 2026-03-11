#pragma once

#include <gst-metadata/meta_base.hpp>

namespace gstmeta::examples {

struct FovData {
    float horizontal_deg{};
    float vertical_deg{};
    float diagonal_deg{};
};

inline constexpr char FovApiName[] = "GstFovMetaAPI";
inline constexpr char FovInfoName[] = "GstFovMetaInfo";

class FovMeta : public MetaBase<FovMeta, FovData, FovApiName, FovInfoName> {
public:
    static constexpr std::uint32_t current_version() { return 1; }
};

} // namespace gstmeta::examples
