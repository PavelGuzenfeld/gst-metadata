#include <gst-metadata-examples/meta_crop.hpp>
#include <gst/base/gstbasetransform.h>

struct GstCropReader {
    GstBaseTransform parent;
};

struct GstCropReaderClass {
    GstBaseTransformClass parent_class;
};

G_DEFINE_TYPE(GstCropReader, gst_crop_reader, GST_TYPE_BASE_TRANSFORM)

static GstStaticPadTemplate sink_tmpl = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);
static GstStaticPadTemplate src_tmpl = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

static GstFlowReturn crop_reader_transform_ip([[maybe_unused]] GstBaseTransform* base,
                                               GstBuffer* buf)
{
    auto crop = gstmeta::examples::CropMeta::get(buf);
    if (crop) {
        g_print("[cropreader] top=%d bottom=%d left=%d right=%d\n",
                crop->top, crop->bottom, crop->left, crop->right);
    } else {
        g_print("[cropreader] no crop metadata on buffer\n");
    }
    return GST_FLOW_OK;
}

static void gst_crop_reader_class_init(GstCropReaderClass* klass)
{
    auto* ec = GST_ELEMENT_CLASS(klass);
    gst_element_class_set_static_metadata(ec,
        "Crop Metadata Reader", "Filter/Metadata",
        "Reads and prints crop metadata from buffers", "gst-metadata");
    gst_element_class_add_static_pad_template(ec, &sink_tmpl);
    gst_element_class_add_static_pad_template(ec, &src_tmpl);
    GST_BASE_TRANSFORM_CLASS(klass)->transform_ip = crop_reader_transform_ip;
}

static void gst_crop_reader_init(GstCropReader* self)
{
    gst_base_transform_set_in_place(GST_BASE_TRANSFORM(self), TRUE);
    gst_base_transform_set_passthrough(GST_BASE_TRANSFORM(self), TRUE);
}

GType gst_crop_reader_get_type(void);
