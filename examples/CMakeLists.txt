# Example GStreamer plugin elements that demonstrate composable metadata.
#
# Produces two plugins:
#   libgstmetawriters.so  — elements: imuwriter, cropwriter, fovwriter
#   libgstmetareaders.so  — elements: imureader, cropreader, fovreader
#
# Usage:
#   GST_PLUGIN_PATH=build/examples gst-launch-1.0 \
#     videotestsrc num-buffers=5 ! imuwriter ! cropwriter ! fovwriter ! \
#     imureader ! cropreader ! fovreader ! fakesink

# --- Writers plugin ---------------------------------------------------------
add_library(gstmetawriters SHARED
    writers/imu_writer.cpp
    writers/crop_writer.cpp
    writers/fov_writer.cpp
    writers/writers_plugin.cpp
)
target_link_libraries(gstmetawriters PRIVATE gst-metadata)
set_target_properties(gstmetawriters PROPERTIES PREFIX "libgst" OUTPUT_NAME "metawriters")

# --- Readers plugin ---------------------------------------------------------
add_library(gstmetareaders SHARED
    readers/imu_reader.cpp
    readers/crop_reader.cpp
    readers/fov_reader.cpp
    readers/readers_plugin.cpp
)
target_link_libraries(gstmetareaders PRIVATE gst-metadata)
set_target_properties(gstmetareaders PROPERTIES PREFIX "libgst" OUTPUT_NAME "metareaders")
