#ifndef STREAM_THREAD_H
#define STREAM_THREAD_H

#include <pthread.h>
#include "gstream.h"
#include "v4l2.h"

struct StreamState{
    int fd;
    struct CustomData * g_data;
    unsigned char ** buff;
    int is_streaming;
    int snap;
    struct StreamInfo * info;
    pthread_mutex_t lock;
};


void* stream_thread(void* arg);

#endif