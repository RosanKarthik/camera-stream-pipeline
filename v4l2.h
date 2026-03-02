#ifndef V4L2_APP_H
#define V4L2_APP_H

#include <stdint.h>

struct StreamInfo{
    char fmt_name[16];
    int fmt_id;
    int height;
    int width;
};

struct pix_formats{
    uint8_t format[32];
    uint32_t id;
};

struct img_res{
    int width;
    int height;
};

struct img_ctrl{
    uint32_t id;
    char name[64];
    uint8_t is_init;
};

void query_capablities(int fd);

int enum_formats(int fd, struct pix_formats *available);

int enum_resolution(int fd, struct img_res *res, int format);

int enum_cntrl(int fd,struct img_ctrl * available);

int get_ctrl(int fd, uint32_t ctrl_id);

int set_ctrl(int fd, uint32_t ctrl_id,int32_t val);

void set_formats(int fd, int width, int height, int pixformat);

int req_buff(int fd, int count);

int query_buff(int fd, int index, unsigned char **buffer);

int queue_buff(int fd, int index);

int dequeue_buff(int fd, int *bytes_deq);

int start_streaming(int fd);

int stop_streaming(int fd);

#endif