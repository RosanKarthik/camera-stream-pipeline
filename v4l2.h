#ifndef V4L2_APP_H
#define V4L2_APP_H

#include <stdint.h>

struct pix_formats{
    uint8_t format[32];
    uint32_t id;
};

struct img_res{
    int width;
    int height;
};

void query_capablities(int fd);

int enum_formats(int fd, struct pix_formats *available);

int enum_resolution(int fd, struct img_res *res, int format);

int enum_cntrl(int fd);

void set_formats(int fd, int width, int height, int pixformat);

int req_buff(int fd, int count);

int query_buff(int fd, int index, unsigned char **buffer);

int queue_buff(int fd, int index);

int dequeue_buff(int fd, int *bytes_deq);

int start_streaming(int fd);

int stop_streaming(int fd);

#endif