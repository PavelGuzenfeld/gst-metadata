#include <gst-metadata-examples/meta_crop.hpp>
#include <gst/base/gstbasetransform.h>

struct GstCropWriter {
    GstBaseTransform parent;
};

struct GstCropWriterClass {
    GstBaseTransformClass parent_class;
};

G_DEFINE_TYPE(GstCropWriter, gst_crop_writer, GST_TYPE_BASE_TRANSFORM)

static GstStaticPadTemplate sink_tmpl = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);
static GstStaticPadTemplate src_tmpl = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

static GstFlowReturn crop_writer_transform_ip([[maybe_unused]] GstBaseTransform* base,
                                               GstBuffer* buf)
{
    gstmeta::examples::CropData crop{.top = 10, .bottom = 10, .left = 20, .right = 20};
    gstmeta::examples::CropMeta::add(buf, crop);
    return GST_FLOW_OK;
}

static void gst_crop_writer_class_init(GstCropWriterClass* klass)
{
    auto* ec = GST_ELEMENT_CLASS(klass);
    gst_element_class_set_static_metadata(ec,
        "Crop Metadata Writer", "Filter/Metadata",
        "Attaches crop metadata to buffers", "gst-metadata");
    gst_element_class_add_static_pad_template(ec, &sink_tmpl);
    gst_element_class_add_static_pad_template(ec, &src_tmpl);
    GST_BASE_TRANSFORM_CLASS(klass)->transform_ip = crop_writer_transform_ip;
}

static void gst_crop_writer_init(GstCropWriter* self)
{
    gst_base_transform_set_in_place(GST_BASE_TRANSFORM(self), TRUE);
    gst_base_transform_set_passthrough(GST_BASE_TRANSFORM(self), TRUE);
}

GType gst_crop_writer_get_type(void);
