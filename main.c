#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <string.h>
#include <stdbool.h>

#include "gstream.h" 
#include "v4l2.h"
//TODO create the header
// #include "gstream.h"

int main(int argc,char * argv[]){
    unsigned char *buff[3];
	int fd = open("/dev/video0", O_RDWR);
	int size;
	int index;
	int num_buffs;
    int input;
    query_capablities(fd);

    //control query
    struct img_ctrl ctrls[20];
    int ctrl_count=enum_cntrl(fd,ctrls);
    if(ctrl_count==0){
        printf("No available controls");
    }
    else{
        //control choice
        printf("[0]No\nEnter the control number to change:\nInput:");
        scanf("%d",&input);
        if(input){
            int32_t val;
            uint32_t ctrl_id=ctrls[input].id;
            printf("Enter the value to set: ");
            scanf("%d",&val);
            set_ctrl(fd,ctrl_id,val);
        }
    }

    //format query
    struct pix_formats available[16]={0};
    int fmt_count=enum_formats(fd,available);
    char fmt_name[16];
    int fmt_id;
    if(fmt_count==0) {
        printf("No valid format detected. Exiting...\n");
        return 0;
    }

    //format choice
    printf("[-1]Exit\nInput:");
    scanf("%d",&input);
    if(input==-1) return 0; 

    fmt_id=available[input].id;
    strncpy(fmt_name,available[input].format,16);

    //resolution query
    struct img_res res[16]={0};
    int res_count= enum_resolution(fd,res,fmt_id);
    if(res_count==0) {
        printf("No valid resolution detected. Exiting...\n");
        return 0;
    }

    //resolution choice
    printf("[-1]Exit\nInput:");
    scanf("%d",&input);
    if(input==-1) return 0;

    int height=res[input].height;
    int width=res[input].width;

    set_formats(fd,width,height,fmt_id);

    num_buffs=req_buff(fd,3);

    if(num_buffs<3){
        printf("Buffers allocated lesser than requested : %d\n",num_buffs);
    }

    for(int i=0;i<3;i++){
        size=query_buff(fd,i,&buff[i]);
        printf("Bytes allocated for buff %d : %d\n",i,size);
        queue_buff(fd,i);
    }

    start_streaming(fd);
    
    //gstream pipeline
    struct CustomData data;
    gst_init(&argc, &argv);
    data.pipeline = gst_pipeline_new("v4l2-appsrc-pipeline");
    data.appsrc = gst_element_factory_make("appsrc", "src");
    data.conv = gst_element_factory_make("videoconvert", "conv");
    data.sink = gst_element_factory_make("fpsdisplaysink", "sink");

    if (!data.pipeline || !data.appsrc || !data.conv || !data.sink) {
        g_printerr("Failed to create GStreamer elements\n");
        return -1;
    }
    
    switch(fmt_id){
        case V4L2_PIX_FMT_MJPEG:
            data.pipeline = gst_pipeline_new("mjpeg-appsrc-pipeline");
            data.appsrc  = gst_element_factory_make("appsrc", "src");
            data.jpegdec = gst_element_factory_make("jpegdec", "dec");
            data.conv    = gst_element_factory_make("videoconvert", "conv");
            data.sink    = gst_element_factory_make("autovideosink", "sink");

            if (!data.pipeline || !data.appsrc || !data.jpegdec || !data.conv || !data.sink) {
                g_printerr("Failed to create GStreamer elements\n");
                return -1;
            }

            gst_bin_add_many(GST_BIN(data.pipeline), data.appsrc, data.jpegdec, data.conv, data.sink, NULL);
            gst_element_link_many(data.appsrc,data.jpegdec, data.conv, data.sink, NULL);            
            data.caps = gst_caps_new_simple(
                "image/jpeg",
                "width", G_TYPE_INT, width,
                "height", G_TYPE_INT, height,
                "framerate", GST_TYPE_FRACTION, 30, 1,
                NULL);
            // strcpy(name,"mjepg.jpg");
            break;
        case V4L2_PIX_FMT_YUYV:
            data.pipeline = gst_pipeline_new("v4l2-appsrc-pipeline");
            data.appsrc = gst_element_factory_make("appsrc", "src");
            data.conv = gst_element_factory_make("videoconvert", "conv");
            data.sink = gst_element_factory_make("fpsdisplaysink", "sink");

            gst_bin_add_many(GST_BIN(data.pipeline), data.appsrc, data.conv, data.sink, NULL);
            gst_element_link_many(data.appsrc, data.conv, data.sink, NULL);
            data.caps = gst_caps_new_simple(
                "video/x-raw",
                "format", G_TYPE_STRING, "YUY2",
                "width", G_TYPE_INT, width,
                "height", G_TYPE_INT, height,
                "framerate", GST_TYPE_FRACTION, 10, 1,
                NULL);
            // strcpy(name,"yuv.yuv");
            break;
        default:
            printf("Invalid Format ID \n");
            return 0;
    }

    g_object_set(data.appsrc,
        "caps", data.caps,
        "stream-type", GST_APP_STREAM_TYPE_STREAM,
        "is-live", TRUE,
        "format", GST_FORMAT_TIME,
        "do-timestamp", TRUE,
        NULL);

    gst_caps_unref(data.caps);

    gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    
	int frame_count = 0;

    while (frame_count < 300) {   
        int bytes_deq;
        int index = dequeue_buff(fd, &bytes_deq);

        // printf("Frame %d pushed, size = %d bytes\n", frame_count, bytes_deq);
        GstBuffer *gst_buf = gst_buffer_new_allocate(NULL, bytes_deq, NULL);

        gst_buffer_fill(gst_buf, 0, buff[index], bytes_deq);

        // GST_BUFFER_PTS(gst_buf) =
        //     gst_util_uint64_scale(frame_count, GST_SECOND, 10);
        // GST_BUFFER_DURATION(gst_buf) =
        //     gst_util_uint64_scale(1, GST_SECOND, 10);

        GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(data.appsrc), gst_buf);

        if (ret != GST_FLOW_OK) {
            g_printerr("Failed to push buffer\n");
            break;
        }

        queue_buff(fd, index);

        frame_count++;
    }

	stop_streaming(fd);

    gst_app_src_end_of_stream(GST_APP_SRC(data.appsrc));
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);

	for (int i =0; i < 3; i++) {
		munmap(buff[i], size);
	}
	// close(file);
	close(fd);
    return 0;
}
