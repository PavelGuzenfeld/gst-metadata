#include <gst-metadata/meta_imu.hpp>
#include <gst/base/gstbasetransform.h>

struct GstImuReader {
    GstBaseTransform parent;
};

struct GstImuReaderClass {
    GstBaseTransformClass parent_class;
};

G_DEFINE_TYPE(GstImuReader, gst_imu_reader, GST_TYPE_BASE_TRANSFORM)

static GstStaticPadTemplate sink_tmpl = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);
static GstStaticPadTemplate src_tmpl = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

static GstFlowReturn imu_reader_transform_ip([[maybe_unused]] GstBaseTransform* base,
                                              GstBuffer* buf)
{
    auto imu = gstmeta::ImuMeta::get(buf);
    if (imu) {
        g_print("[imureader] ts=%" G_GUINT64_FORMAT " accel=(%.2f,%.2f,%.2f) temp=%.1f\n",
                imu->timestamp_ns,
                imu->accel_x, imu->accel_y, imu->accel_z,
                imu->temperature);
    } else {
        g_print("[imureader] no IMU metadata on buffer\n");
    }
    return GST_FLOW_OK;
}

static void gst_imu_reader_class_init(GstImuReaderClass* klass)
{
    auto* ec = GST_ELEMENT_CLASS(klass);
    gst_element_class_set_static_metadata(ec,
        "IMU Metadata Reader", "Filter/Metadata",
        "Reads and prints IMU metadata from buffers", "gst-metadata");
    gst_element_class_add_static_pad_template(ec, &sink_tmpl);
    gst_element_class_add_static_pad_template(ec, &src_tmpl);
    GST_BASE_TRANSFORM_CLASS(klass)->transform_ip = imu_reader_transform_ip;
}

static void gst_imu_reader_init(GstImuReader* self)
{
    gst_base_transform_set_in_place(GST_BASE_TRANSFORM(self), TRUE);
    gst_base_transform_set_passthrough(GST_BASE_TRANSFORM(self), TRUE);
}

GType gst_imu_reader_get_type(void);
