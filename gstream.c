#include <stdio.h>
#include <stdlib.h>
// #include <stdbool.h">

#include <pthread.h>
#include <linux/videodev2.h>
#include "gstream.h"

// #include "v4l2.h"

int gstream_init(struct CustomData * data){
    data->pipeline = gst_pipeline_new("v4l2-appsrc-pipeline");
    data->appsrc = gst_element_factory_make("appsrc", "src");
    data->conv = gst_element_factory_make("videoconvert", "conv");
    data->sink = gst_element_factory_make("fpsdisplaysink", "sink");

    if (!data->pipeline || !data->appsrc || !data->conv || !data->sink) {
        g_printerr("Failed to create GStreamer elements\n");
        return -1;
    }
    return 0;
}

int gstream_deinit(struct CustomData * data){
    gst_app_src_end_of_stream(GST_APP_SRC(data->appsrc));
    gst_element_set_state(data->pipeline, GST_STATE_NULL);
    gst_object_unref(data->pipeline);
    return 0;
}

int gstream_setup(struct CustomData * data,int fmt_id,int width,int height){
    switch(fmt_id){
        case V4L2_PIX_FMT_MJPEG:
            data->pipeline = gst_pipeline_new("mjpeg-appsrc-pipeline");
            data->appsrc  = gst_element_factory_make("appsrc", "src");
            data->jpegdec = gst_element_factory_make("jpegdec", "dec");
            data->conv    = gst_element_factory_make("videoconvert", "conv");
            data->sink    = gst_element_factory_make("autovideosink", "sink");

            if (!data->pipeline || !data->appsrc || !data->jpegdec || !data->conv || !data->sink) {
                g_printerr("Failed to create GStreamer elements\n");
                return -1;
            }

            gst_bin_add_many(GST_BIN(data->pipeline), data->appsrc, data->jpegdec, data->conv, data->sink, NULL);
            gst_element_link_many(data->appsrc,data->jpegdec, data->conv, data->sink, NULL);            
            data->caps = gst_caps_new_simple(
                "image/jpeg",
                "width", G_TYPE_INT, width,
                "height", G_TYPE_INT, height,
                "framerate", GST_TYPE_FRACTION, 30, 1,
                NULL);
            // strcpy(name,"mjepg.jpg");
            break;
        case V4L2_PIX_FMT_YUYV:
            data->pipeline = gst_pipeline_new("v4l2-appsrc-pipeline");
            data->appsrc = gst_element_factory_make("appsrc", "src");
            data->conv = gst_element_factory_make("videoconvert", "conv");
            data->sink = gst_element_factory_make("fpsdisplaysink", "sink");

            if (!data->pipeline || !data->appsrc || !data->conv || !data->sink) {
                g_printerr("Failed to create GStreamer elements\n");
                return -1;
            }

            gst_bin_add_many(GST_BIN(data->pipeline), data->appsrc, data->conv, data->sink, NULL);
            gst_element_link_many(data->appsrc, data->conv, data->sink, NULL);
            data->caps = gst_caps_new_simple(
                "video/x-raw",
                "format", G_TYPE_STRING, "YUY2",
                "width", G_TYPE_INT, width,
                "height", G_TYPE_INT, height,
                "framerate", GST_TYPE_FRACTION, 10, 1,
                NULL);
            // strcpy(name,"yuv.yuv");
            break;
        default:
            printf("Invalid Format ID \n");
    }
    g_object_set(data->appsrc,
        "caps", data->caps,
        "stream-type", GST_APP_STREAM_TYPE_STREAM,
        "is-live", TRUE,
        "format", GST_FORMAT_TIME,
        "do-timestamp", TRUE,
        NULL);
    gst_caps_unref(data->caps);
    return 0;
}