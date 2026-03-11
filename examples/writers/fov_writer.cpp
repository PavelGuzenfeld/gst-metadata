#include <gst-metadata-examples/meta_fov.hpp>
#include <gst/base/gstbasetransform.h>

struct GstFovWriter {
    GstBaseTransform parent;
};

struct GstFovWriterClass {
    GstBaseTransformClass parent_class;
};

G_DEFINE_TYPE(GstFovWriter, gst_fov_writer, GST_TYPE_BASE_TRANSFORM)

static GstStaticPadTemplate sink_tmpl = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);
static GstStaticPadTemplate src_tmpl = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

static GstFlowReturn fov_writer_transform_ip([[maybe_unused]] GstBaseTransform* base,
                                              GstBuffer* buf)
{
    gstmeta::examples::FovData fov{
        .horizontal_deg = 82.0F, .vertical_deg = 52.0F, .diagonal_deg = 97.0F};
    gstmeta::examples::FovMeta::add(buf, fov);
    return GST_FLOW_OK;
}

static void gst_fov_writer_class_init(GstFovWriterClass* klass)
{
    auto* ec = GST_ELEMENT_CLASS(klass);
    gst_element_class_set_static_metadata(ec,
        "FOV Metadata Writer", "Filter/Metadata",
        "Attaches field-of-view metadata to buffers", "gst-metadata");
    gst_element_class_add_static_pad_template(ec, &sink_tmpl);
    gst_element_class_add_static_pad_template(ec, &src_tmpl);
    GST_BASE_TRANSFORM_CLASS(klass)->transform_ip = fov_writer_transform_ip;
}

static void gst_fov_writer_init(GstFovWriter* self)
{
    gst_base_transform_set_in_place(GST_BASE_TRANSFORM(self), TRUE);
    gst_base_transform_set_passthrough(GST_BASE_TRANSFORM(self), TRUE);
}

GType gst_fov_writer_get_type(void);
