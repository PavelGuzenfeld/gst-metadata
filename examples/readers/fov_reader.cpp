#include <gst-metadata-examples/meta_fov.hpp>
#include <gst/base/gstbasetransform.h>

struct GstFovReader {
    GstBaseTransform parent;
};

struct GstFovReaderClass {
    GstBaseTransformClass parent_class;
};

extern "C" GType gst_fov_reader_get_type(void);
G_DEFINE_TYPE(GstFovReader, gst_fov_reader, GST_TYPE_BASE_TRANSFORM)

static GstStaticPadTemplate sink_tmpl = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);
static GstStaticPadTemplate src_tmpl = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

static GstFlowReturn fov_reader_transform_ip([[maybe_unused]] GstBaseTransform* base,
                                              GstBuffer* buf)
{
    auto fov = gstmeta::examples::FovMeta::get(buf);
    if (fov) {
        g_print("[fovreader] h=%.1f° v=%.1f° d=%.1f°\n",
                fov->horizontal_deg, fov->vertical_deg, fov->diagonal_deg);
    } else {
        g_print("[fovreader] no FOV metadata on buffer\n");
    }
    return GST_FLOW_OK;
}

static void gst_fov_reader_class_init(GstFovReaderClass* klass)
{
    auto* ec = GST_ELEMENT_CLASS(klass);
    gst_element_class_set_static_metadata(ec,
        "FOV Metadata Reader", "Filter/Metadata",
        "Reads and prints field-of-view metadata from buffers", "gst-metadata");
    gst_element_class_add_static_pad_template(ec, &sink_tmpl);
    gst_element_class_add_static_pad_template(ec, &src_tmpl);
    GST_BASE_TRANSFORM_CLASS(klass)->transform_ip = fov_reader_transform_ip;
}

static void gst_fov_reader_init(GstFovReader* self)
{
    gst_base_transform_set_in_place(GST_BASE_TRANSFORM(self), TRUE);
    gst_base_transform_set_passthrough(GST_BASE_TRANSFORM(self), TRUE);
}
