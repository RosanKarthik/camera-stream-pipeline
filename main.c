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

int main(int argc,char * argv[]){
    unsigned char *buff[8];
    int fd = open("/dev/video0", O_RDWR);
    if(fd==-1){
        printf("Unable to connect to device.\nExiting....");
        return -1;
    }
    int buff_size;
    int index;
    int num_buffs;
    int input;
    int ctrl_count=0;
    int res_count=0;
    int fmt_count=0;
    int32_t ctrl_val;
    uint32_t ctrl_id;

    struct StreamInfo info;
    struct pix_formats available[16]={0};
    struct img_res res[16]={0};
    struct img_ctrl ctrls[20]={0};
    struct CustomData data={0};
    pthread_t g_pipeline;
    struct StreamState state;
    
    gst_init(&argc, &argv);
    pthread_mutex_init(&state.lock, NULL);

    while(1){
        query_capablities(fd);  
        printf("-----------------------------------------------------------------------------\n");
        printf("[1]Start Streaming\t[2]Query Controls\t[3]Get/Set Controls\n[4]Stop Streaming\t[5]Take a Snap\t\t[0]Quit\n");
        printf("-----------------------------------------------------------------------------\n");
        printf("Choice: ");
        scanf("%d",&input);
        printf("-----------------------------------------------------------------------------\n");
        switch(input){
            case 1:
                //format query
                if(state.is_streaming){
                    printf("Stream is already on...\nPlease turn off if you want to change formats/resolution.\n");
                    continue;
                }
                
                fmt_count=enum_formats(fd,available);
                if(fmt_count==0) {
                    printf("No valid format detected. Exiting...\n");
                    return 0;
                }

                //format choice
                printf("[-1]Exit\n");
                printf("-----------------------------------------------------------------------------\n");
                printf("Input:");
                scanf("%d",&input);
                printf("-----------------------------------------------------------------------------\n");
                if(input==-1) return 0; 

                info.fmt_id=available[input].id;
                strncpy(info.fmt_name,available[input].format,16);

                //resolution query
                res_count= enum_resolution(fd,res,info.fmt_id);
                if(res_count==0) {
                    printf("No valid resolution detected. Exiting...\n");
                    return 0;
                }

                //resolution choice
                printf("[-1]Exit\n");
                printf("-----------------------------------------------------------------------------\n");
                printf("Input:");
                scanf("%d",&input);
                printf("-----------------------------------------------------------------------------\n");
                if(input==-1) return 0;

                info.height=res[input].height;
                info.width=res[input].width;

                set_formats(fd,info.width,info.height,info.fmt_id);

                num_buffs=req_buff(fd,8);

                if(num_buffs<8){
                    printf("[debug]Buffers allocated lesser than requested : %d\n",num_buffs);
                }

                for(int i=0;i<8;i++){
                    buff_size=query_buff(fd,i,&buff[i]);
                    printf("[debug]Bytes allocated for buff %d : %d\n",i,buff_size);
                    queue_buff(fd,i);
                }

                //v4l2 start streaming
                start_streaming(fd);

                //gstream pipeline start

                if(!gstream_setup(&data,&info)){
                    printf("Gstream setup success\n");
                }
        
                state.is_streaming=1;
                state.fd=fd;
                state.g_data=&data;
                state.buff=buff;
                state.info=&info;

                pthread_create(&g_pipeline,NULL,stream_thread,&state);

                break;
            case 2:
                ctrl_count=enum_cntrl(fd,ctrls);
                if(ctrl_count==0){
                    printf("No available controls\n");
                }
                break;
            case 3:
                if(ctrl_count==0){
                    printf("Please query controls first.\n");
                    continue;
                }
                printf("Do you want to set or the current value of a control?:\n[0]Set [1]Get: ");
                scanf("%d",&input);
                //set
                if(!input){
                    printf("[0]Exit\nEnter the control number to change:\nInput:");
                    scanf("%d",&input);
                    if(input){
                        ctrl_id=ctrls[input].id;
                        get_ctrl(fd,ctrl_id);
                        printf("Enter the value to set: ");
                        scanf("%d",&ctrl_val);
                        set_ctrl(fd,ctrl_id,ctrl_val);
                        break;
                    }
                    else{
                        break;
                    }
                }
                //get
                else{
                    printf("[0]Exit\nEnter the control number to get:\nInput:");
                    scanf("%d",&input);
                    if(input){
                        uint32_t ctrl_id=ctrls[input].id;
                        get_ctrl(fd,ctrl_id);
                        break;
                    }
                    else{
                        break;
                    }
                }
            case 4:
                pthread_mutex_lock(&state.lock);
                state.is_streaming=0;
                pthread_mutex_unlock(&state.lock);
                pthread_join(g_pipeline,NULL);
                stop_streaming(fd);     
                for (int i =0; i < 8; i++) {
                    munmap(buff[i], buff_size);
                }
                req_buff(fd, 0);
                gstream_deinit(&data);
                pthread_mutex_destroy(&state.lock);
                printf("You can now press 0 to quit the application\n");
                break;

            case 5:
                if(!state.is_streaming)
                {
                    printf("Please start the stream first.\n");
                    continue;
                }
                pthread_mutex_lock(&state.lock);
                state.snap=1;
                pthread_mutex_unlock(&state.lock);
                break;
            case 0:
                if(state.is_streaming){
                    printf("Turn off streaming before quiting.\n");
                    continue;
                }
                close(fd);
                return 0;
            default:
                printf("Enter valid input!\n");
                sleep(1);
                continue;
        }
    }
    return 0;
}
