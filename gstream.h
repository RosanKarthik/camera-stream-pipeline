#ifndef GSTREAM_APP_H
#define GSTREAM_APP_H

#include "gst/gst.h"
#include "gst/app/gstappsrc.h"

struct CustomData{
    GstElement *appsrc, *sink, *pipeline,*conv,*jpegdec;
    GstCaps *caps;
    GstBus *bus;
    GstBuffer *g_buff; 
    GstMessage *msg;
};

int gstream_init(struct CustomData * data);
int gstream_setup(struct CustomData * data,int fmt_id,int width,int height);
int gstream_deinit(struct CustomData * data);

#endif