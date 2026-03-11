#include <gst/gst.h>

extern "C" {
GType gst_imu_reader_get_type(void);
GType gst_crop_reader_get_type(void);
GType gst_fov_reader_get_type(void);
}

static gboolean plugin_init(GstPlugin* plugin)
{
    gboolean ok = TRUE;
    ok &= gst_element_register(plugin, "imureader", GST_RANK_NONE, gst_imu_reader_get_type());
    ok &= gst_element_register(plugin, "cropreader", GST_RANK_NONE, gst_crop_reader_get_type());
    ok &= gst_element_register(plugin, "fovreader", GST_RANK_NONE, gst_fov_reader_get_type());
    return ok;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR,
                  metareaders, "Metadata reader elements",
                  plugin_init, "0.1", "LGPL", "gst-metadata", "https://github.com/PavelGuzenfeld/gst-metadata")
