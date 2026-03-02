#ifndef GSTREAM_APP_H
#define GSTREAM_APP_H

#include <pthread.h>
#include <linux/videodev2.h>
#include "gst/gst.h"
#include "gst/app/gstappsrc.h"
#include "v4l2.h"

struct CustomData{
    GstElement *appsrc, *sink, *pipeline,*conv,*jpegdec;
    GstCaps *caps;
    GstBus *bus;
    GstBuffer *g_buff; 
    GstMessage *msg;
};

int gstream_init(struct CustomData * data);
int gstream_setup(struct CustomData * data,struct StreamInfo * info);
int gstream_deinit(struct CustomData * data);

#endif