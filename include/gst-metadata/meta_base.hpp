#pragma once

#include <gst/gst.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace gstmeta {

/// Header prepended to every metadata payload.
/// Enables forward-compatible readers: a reader compiled against an older
/// struct can detect that the writer used a larger struct and safely read
/// only the fields it knows about.
struct MetaHeader {
    std::uint32_t version;
    std::uint32_t data_size; // sizeof(DataType) at write time
};

/// CRTP base for composable GStreamer metadata.
///
/// Template parameters:
///   Derived       — the concrete metadata class (CRTP)
///   DataType      — the POD payload struct
///   ApiName       — unique API type name string  (e.g. "GstImuMetaAPI")
///   InfoName      — unique info name string       (e.g. "GstImuMetaInfo")
///
/// Each instantiation produces an independent GType, so multiple metadata
/// types coexist on the same GstBuffer and can be queried independently.
template <typename Derived, typename DataType, const char* ApiName, const char* InfoName>
class MetaBase {
public:
    /// The in-buffer layout: GstMeta header + version header + user data.
    struct Storage {
        GstMeta meta;
        MetaHeader header;
        DataType data;
    };

    // -- Primary API --------------------------------------------------------

    /// Attach metadata to a buffer.  Returns the populated Storage pointer.
    static Storage* add(GstBuffer* buffer, const DataType& payload)
    {
        g_return_val_if_fail(buffer != nullptr, nullptr);

        auto* s = reinterpret_cast<Storage*>(
            gst_buffer_add_meta(buffer, info(), nullptr));
        if (s) {
            s->header.version = Derived::current_version();
            s->header.data_size = sizeof(DataType);
            s->data = payload;
        }
        return s;
    }

    /// Retrieve the first metadata of this type from a buffer.
    static std::optional<DataType> get(GstBuffer* buffer)
    {
        g_return_val_if_fail(buffer != nullptr, std::nullopt);

        auto* s = reinterpret_cast<Storage*>(
            gst_buffer_get_meta(buffer, api_type()));
        if (!s)
            return std::nullopt;
        return s->data;
    }

    /// Retrieve a mutable pointer to the first metadata of this type.
    /// Returns nullptr if not present.
    static Storage* get_mut(GstBuffer* buffer)
    {
        g_return_val_if_fail(buffer != nullptr, nullptr);
        return reinterpret_cast<Storage*>(
            gst_buffer_get_meta(buffer, api_type()));
    }

    /// Iterate over all instances of this metadata type on a buffer.
    static void for_each(GstBuffer* buffer,
                         const std::function<void(const DataType&)>& fn)
    {
        g_return_if_fail(buffer != nullptr);

        gpointer state = nullptr;
        GstMeta* m = nullptr;
        while ((m = gst_buffer_iterate_meta_filtered(
                    buffer, &state, api_type())) != nullptr) {
            auto* s = reinterpret_cast<Storage*>(m);
            fn(s->data);
        }
    }

    /// Collect all instances into a vector.
    static std::vector<DataType> get_all(GstBuffer* buffer)
    {
        std::vector<DataType> result;
        for_each(buffer, [&](const DataType& d) { result.push_back(d); });
        return result;
    }

    /// Remove the first instance of this metadata from a buffer.
    static bool remove(GstBuffer* buffer)
    {
        g_return_val_if_fail(buffer != nullptr, false);

        auto* m = gst_buffer_get_meta(buffer, api_type());
        if (!m)
            return false;
        return gst_buffer_remove_meta(buffer, m) == TRUE;
    }

    /// Count how many instances of this metadata type are on the buffer.
    static std::size_t count(GstBuffer* buffer)
    {
        std::size_t n = 0;
        for_each(buffer, [&](const DataType&) { ++n; });
        return n;
    }

    // -- GStreamer type plumbing (public for advanced use) -------------------

    /// Per-type unique GType.  Safe to call from any thread.
    static GType api_type()
    {
        static GType type = 0;
        if (g_once_init_enter(&type)) {
            // No tags → metadata is NOT sensitive to any transform.
            // It survives passthrough elements automatically.
            const gchar* tags[] = {nullptr};
            GType t = gst_meta_api_type_register(ApiName, tags);
            g_once_init_leave(&type, t);
        }
        return type;
    }

    /// Per-type GstMetaInfo.  Safe to call from any thread.
    static const GstMetaInfo* info()
    {
        static const GstMetaInfo* meta_info = nullptr;
        if (g_once_init_enter(&meta_info)) {
            const GstMetaInfo* mi = gst_meta_register(
                api_type(),
                InfoName,
                sizeof(Storage),
                init_cb,
                free_cb,
                transform_cb);
            g_once_init_leave(&meta_info, mi);
        }
        return meta_info;
    }

protected:
    // -- Default version (override in Derived if payload evolves) -----------
    static constexpr std::uint32_t current_version() { return 1; }

private:
    // -- GstMeta callbacks --------------------------------------------------

    static gboolean init_cb(GstMeta* meta, [[maybe_unused]] gpointer params,
                            [[maybe_unused]] GstBuffer* buffer)
    {
        auto* s = reinterpret_cast<Storage*>(meta);
        s->header = {};
        s->data = {};
        return TRUE;
    }

    // GStreamer expects void return for free callback.
    static void free_cb([[maybe_unused]] GstMeta* meta,
                        [[maybe_unused]] GstBuffer* buffer)
    {
        // DataType is POD — nothing to free.
        // If Derived needs cleanup, specialize this.
    }

    static gboolean transform_cb(GstBuffer* transbuf, GstMeta* meta,
                                 [[maybe_unused]] GstBuffer* buffer,
                                 GQuark type,
                                 [[maybe_unused]] gpointer data)
    {
        // Only handle copy transforms.
        if (GST_META_TRANSFORM_IS_COPY(type)) {
            auto* src = reinterpret_cast<Storage*>(meta);
            auto* dst = reinterpret_cast<Storage*>(
                gst_buffer_add_meta(transbuf, info(), nullptr));
            if (dst) {
                dst->header = src->header;
                dst->data = src->data;
            }
            return dst != nullptr;
        }
        // Unknown transform — allow the meta to be dropped rather than crash.
        return FALSE;
    }
};

} // namespace gstmeta
