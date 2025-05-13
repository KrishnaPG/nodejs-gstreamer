#include <gst/gst.h>
#include <iostream>

void check_hardware_acceleration()
{
    GstRegistry* registry = gst_registry_get();
    GstPluginFeature* feature;

    feature = gst_registry_lookup_feature(registry, "vaapidecode");
    if (feature)
    {
        std::cout << "Hardware acceleration (VAAPI) is available." << std::endl;
        gst_object_unref(feature);
    }
    else
    {
        std::cout << "VAAPI not found, falling back to software decoding." << std::endl;
    }

    feature = gst_registry_lookup_feature(registry, "nvv4l2decoder");
    if (feature)
    {
        std::cout << "NVIDIA V4L2 decoder available." << std::endl;
        gst_object_unref(feature);
    }
}