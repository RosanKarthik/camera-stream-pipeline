#ifndef STREAM_THREAD_H
#define STREAM_THREAD_H

#include <pthread.h>
#include "gstream.h"
#include "v4l2.h"

struct StreamState{
    int fd;
    struct CustomData * g_data;
    unsigned char ** buff;
    volatile int is_streaming;
    volatile int snap;
    struct StreamInfo * info;
    pthread_mutex_t lock;
};


void* stream_thread(void* arg);

#endif