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
#include "thread.h"

//TODO create the header
// #include "gstream.h"

int main(int argc,char * argv[]){
    unsigned char *buff[3];
	int fd = open("/dev/video0", O_RDWR);
	int buff_size;
	int index;
	int num_buffs;
    int input;
    query_capablities(fd);

    //control query
    printf("Do you want to change any controls?:\n[0]No [1]Yes: ");
    scanf("%d",&input);
    if(input){
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
                get_ctrl(fd,ctrl_id);
                printf("Enter the value to set: ");
                scanf("%d",&val);
                set_ctrl(fd,ctrl_id,val);
            }
            //TODO ADD GET CNTRL CHOICE
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
        buff_size=query_buff(fd,i,&buff[i]);
        printf("Bytes allocated for buff %d : %d\n",i,buff_size);
        queue_buff(fd,i);
    }

    //v4l2 start streaming
    start_streaming(fd);

    //gstream pipeline start
    struct CustomData data={0};
    gst_init(&argc, &argv);
    if(!gstream_init(&data)){
        printf("Gstream pipeline initialized\n");
    }

    if(gstream_setup(&data,fmt_id,width,height)){
        printf("Gstream setup success\n");
    }
    struct StreamState state;
    state.is_streaming=1;
    state.fd=fd;
    state.g_data=&data;
    state.buff=buff;

    pthread_t g_pipeline;

    pthread_create(&g_pipeline,NULL,stream_thread,&state);

    state.is_streaming=0;

    pthread_join(g_pipeline,NULL);
   
    //v4l2 buff deinit
    for (int i =0; i < 3; i++) {
		munmap(buff[i], buff_size);
	}
    stop_streaming(fd);
	// close(file);
	close(fd);
    return 0;
}
