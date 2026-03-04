#ifndef STREAM_THREAD_H
#define STREAM_THREAD_H

#include <pthread.h>
#include "gstream.h"
#include "v4l2.h"

struct StreamState{
    int fd;
    struct CustomData * g_data;         //struct containing the pipeline elements of the gstream
    unsigned char ** buff;              //v2l4 allocated buff pointer
    volatile int is_streaming;          //flag to indicate stream status
    volatile int snap;                  //flag to indicate snap event
    struct StreamInfo * info;           //struct containing the currenst stream information
    pthread_mutex_t lock;
};


void* stream_thread(void* arg);

#endif