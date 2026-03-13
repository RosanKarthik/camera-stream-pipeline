//program to run the snap/stream

#include <stdio.h>
#include "thread.h"
#include <time.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <fcntl.h>

void* stream_thread(void* arg) {
    struct StreamState *state = (struct StreamState *)arg;
    //local variables to read when mutex locked
    int l_is_streaming=1,l_snap=0;
    gst_element_set_state(state->g_data->pipeline, GST_STATE_PLAYING);
    
    while (l_is_streaming) {
        pthread_mutex_lock(&state->lock);
        l_is_streaming=state->is_streaming;
        l_snap=state->snap;
        state->snap=0;
        pthread_mutex_unlock(&state->lock);

        if(l_is_streaming==0){
            break;
        }
        
        //v4l2 parts   
        int bytes_deq;
        int index = dequeue_buff(state->fd, &bytes_deq);
        //printf("[debug]DQ Buff idx:%d\n",index);
        if(index==-1){
            //debug print
            printf("[Thread]v4l2 buffer not allocated...Skipping...\n");
            continue;
        }
        
        if(l_snap){
            // l_snap=0;
            char filename[256]={0};
            char extension[8]={0};
            time_t now = time(NULL);
            struct tm *t = localtime(&now);
            char time[100];

            strftime(time, sizeof(time), "%d-%m-%Y_%H-%M-%S", t); 

            if(state->info->fmt_id==V4L2_PIX_FMT_YUYV) strcpy(extension,"yuv");
            else if(state->info->fmt_id==V4L2_PIX_FMT_MJPEG) strcpy(extension,"jpg");

            sprintf(filename, "%s_%s.%s", state->info->fmt_name,time,extension);
            FILE *file = fopen(filename, "wb");
            if (file != NULL) {
                fwrite(state->buff[index], 1, bytes_deq, file);
                fclose(file);
            } else {
                printf("[Error] Failed to save snapshot");
            }
        }

        // printf("Frame %d pushed, size = %d bytes\n", frame_count, bytes_deq);
        state->g_data->g_buff= gst_buffer_new_allocate(NULL, bytes_deq, NULL);

        gst_buffer_fill(state->g_data->g_buff, 0, state->buff[index], bytes_deq);

        GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(state->g_data->appsrc), state->g_data->g_buff);

        if (ret != GST_FLOW_OK) {
            //[imp bug fix] queue the buff even if skipping due to gstream err 
            queue_buff(state->fd, index);
            g_printerr("[Thread]Failed to push buffer\n");
            break;
        }

        //v4l2 parts
        queue_buff(state->fd, index);
        //printf("[debug]DQ Buff idx:%d\n",index);
    }
    gst_element_set_state(state->g_data->pipeline, GST_STATE_NULL);
    printf("\n[Thread] Streaming stopped.\n");
    return NULL;
}
