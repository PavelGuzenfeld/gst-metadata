#include <gst/gst.h>

extern "C" {
GType gst_imu_writer_get_type(void);
GType gst_crop_writer_get_type(void);
GType gst_fov_writer_get_type(void);
}

static gboolean plugin_init(GstPlugin* plugin)
{
    gboolean ok = TRUE;
    ok &= gst_element_register(plugin, "imuwriter", GST_RANK_NONE, gst_imu_writer_get_type());
    ok &= gst_element_register(plugin, "cropwriter", GST_RANK_NONE, gst_crop_writer_get_type());
    ok &= gst_element_register(plugin, "fovwriter", GST_RANK_NONE, gst_fov_writer_get_type());
    return ok;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR,
                  metawriters, "Metadata writer elements",
                  plugin_init, "0.1", "MIT", "gst-metadata", "https://github.com/PavelGuzenfeld/gst-metadata")
