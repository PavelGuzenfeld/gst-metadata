#include <gst-metadata-examples/meta_imu.hpp>
#include <gst/base/gstbasetransform.h>

#include <cstdint>

// -- GObject boilerplate ----------------------------------------------------

struct GstImuWriter {
    GstBaseTransform parent;
    std::uint64_t frame_count;
};

struct GstImuWriterClass {
    GstBaseTransformClass parent_class;
};

G_DEFINE_TYPE(GstImuWriter, gst_imu_writer, GST_TYPE_BASE_TRANSFORM)

static GstStaticPadTemplate sink_tmpl = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);
static GstStaticPadTemplate src_tmpl = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

static GstFlowReturn imu_writer_transform_ip(GstBaseTransform* base,
                                              GstBuffer* buf)
{
    auto* self = reinterpret_cast<GstImuWriter*>(base);

    gstmeta::examples::ImuData imu{};
    imu.timestamp_ns = GST_BUFFER_PTS(buf);
    imu.accel_x = 0.0F;
    imu.accel_y = 0.0F;
    imu.accel_z = -9.81F;
    imu.temperature = 25.0F + static_cast<float>(self->frame_count) * 0.1F;

    gstmeta::examples::ImuMeta::add(buf, imu);
    self->frame_count++;
    return GST_FLOW_OK;
}

static void gst_imu_writer_class_init(GstImuWriterClass* klass)
{
    auto* element_class = GST_ELEMENT_CLASS(klass);
    gst_element_class_set_static_metadata(element_class,
        "IMU Metadata Writer", "Filter/Metadata",
        "Attaches IMU metadata to buffers",
        "gst-metadata");

    gst_element_class_add_static_pad_template(element_class, &sink_tmpl);
    gst_element_class_add_static_pad_template(element_class, &src_tmpl);

    auto* bt = GST_BASE_TRANSFORM_CLASS(klass);
    bt->transform_ip = imu_writer_transform_ip;
}

static void gst_imu_writer_init(GstImuWriter* self)
{
    gst_base_transform_set_in_place(GST_BASE_TRANSFORM(self), TRUE);
    gst_base_transform_set_passthrough(GST_BASE_TRANSFORM(self), TRUE);
    self->frame_count = 0;
}

GType gst_imu_writer_get_type(void);
