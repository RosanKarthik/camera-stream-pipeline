#include <stdio.h>
#include "thread.h"

void* stream_thread(void* arg) {
    struct StreamState *state = (struct StreamState *)arg;
    gst_element_set_state(state->g_data->pipeline, GST_STATE_PLAYING);
    while (state->is_streaming) {
        //v4l2 parts   
        int bytes_deq;
        int index = dequeue_buff(state->fd, &bytes_deq);

        // printf("Frame %d pushed, size = %d bytes\n", frame_count, bytes_deq);
        state->g_data->g_buff= gst_buffer_new_allocate(NULL, bytes_deq, NULL);

        gst_buffer_fill(state->g_data->g_buff, 0, state->buff[index], bytes_deq);

        // GST_BUFFER_PTS(gst_buf) =
        //     gst_util_uint64_scale(frame_count, GST_SECOND, 10);
        // GST_BUFFER_DURATION(gst_buf) =
        //     gst_util_uint64_scale(1, GST_SECOND, 10);

        GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(state->g_data->appsrc), state->g_data->g_buff);

        if (ret != GST_FLOW_OK) {
            g_printerr("Failed to push buffer\n");
            break;
        }

        //v4l2 parts
        queue_buff(state->fd, index);

    }
    printf("\n[Thread] Streaming stopped.\n");
    return NULL;
}