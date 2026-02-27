#ifndef GSTREAM_APP_H
#define GSTREAM_APP_H
#include "gst/gst.h"
#include "gst/app/gstappsrc.h"

struct CustomData{
    GstElement *appsrc, *sink, *pipeline,*conv,*jpegdec;
    GstCaps *caps;
    GstBus *bus;
    GstBuffer *buff; 
    GstMessage *msg;
};
#endif