//this program runs the main loop of the streaming app
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

#define MAX_NUM_FMTS 16
#define MAX_NUM_RES 16

int main(int argc,char * argv[]){
    int fd= openDev();
    if(fd==-1){
        return EXIT_FAILURE;
    }
    unsigned char *buff[NUM_BUFFS];
    int buff_size[NUM_BUFFS];
    int index;
    int num_buffs;
    int input;
    int res_count=0;
    int fmt_count=0;

    struct StreamInfo info;
    struct pix_formats available[MAX_NUM_FMTS]={0};
    struct img_res res[MAX_NUM_RES]={0};
    struct CustomData data={0};
    pthread_t g_pipeline;
    struct StreamState state={0};
    
    gst_init(&argc, &argv);
    pthread_mutex_init(&state.lock, NULL);
    query_capablities(fd); 
    
    while(1){
        printf("-----------------------------------------------------------------------------\n");
        printf("[1]Start Streaming\t[2]Controls\n[3]Stop Streaming\t[4]Take a Snap\n[-1]Quit\n");
        printf("-----------------------------------------------------------------------------\n");
        printf("Choice: ");
        if(validate_inp(&input)==EXIT_FAILURE) continue;
        printf("-----------------------------------------------------------------------------\n");
        switch(input){
            case 1:

                if(state.is_streaming){
                    printf("Stream is already on...\nPlease turn off if you want to change formats/resolution.\n");
                    continue;
                }
                //format query
                fmt_count=enum_formats(fd,available);
                if(fmt_count==0) {
                    printf("No valid format detected.Device unable to stream\n");
                    break;
                }

                //format choice
                printf("[-1]Exit\n");
                printf("-----------------------------------------------------------------------------\n");
                printf("Input:");
                if(validate_inp(&input)==EXIT_FAILURE) continue;
                printf("-----------------------------------------------------------------------------\n");
                if(input==-1) continue; 

                if(input < 0 || input >= fmt_count) {
                    printf("Invalid format selection.\n");
                    continue;
                }

                info.fmt_id=available[input].id;
                strncpy(info.fmt_name,available[input].format,FMT_NAME_LEN);

                //resolution query
                res_count= enum_resolution(fd,res,info.fmt_id);
                if(res_count==0 || res_count == EXIT_FAILURE) {
                    sleep(1);
                    break;
                }

                //resolution choice
                printf("[-1]Exit\n");
                printf("-----------------------------------------------------------------------------\n");
                printf("Input:");
                if(validate_inp(&input)==EXIT_FAILURE) continue;
                printf("-----------------------------------------------------------------------------\n");
                if(input==-1) continue;;

                if(input < 0 || input >= res_count) {
                    printf("Invalid resolution selection.\n");
                    continue;
                }

                info.height=res[input].height;
                info.width=res[input].width;

                if(set_formats(fd,info.width,info.height,info.fmt_id)==-1){
                    sleep(1);
                    break;
                }

                //request v4l2 to allocate n buffs
                num_buffs=req_buff(fd,NUM_BUFFS);

                if(num_buffs<NUM_BUFFS){
                    printf("[alert]Buffers allocated lesser than requested : %d\n",num_buffs);
                }

                for(int i=0;i<num_buffs;i++){
                    buff_size[i]=query_buff(fd,i,&buff[i]);
                    if(buff_size[i]==0){
                        printf("[debug]Memory not allocated for buffer %d!\n",i);
                    }
                    else {
                        printf("[debug]Bytes allocated for buff %d : %d\n",i,buff_size[i]);
                    }
                    queue_buff(fd,i);
                }

                //v4l2 start streaming
                if(start_streaming(fd)==-1){
                    printf("Error starting stream.Try again.\n");
                    for (int i = 0; i < NUM_BUFFS; i++) {
                        munmap(buff[i], buff_size[i]);
                    }
                    req_buff(fd, 0);
                    continue;
                }

                //gstream pipeline start

                if(gstream_setup(&data,&info)==EXIT_FAILURE){
                    printf("Gstream pipeline setup failed.Try again.\n");
                    stop_streaming(fd);
                    for (int i = 0; i < NUM_BUFFS; i++) {
                        munmap(buff[i], buff_size[i]);
                    }
                    req_buff(fd, 0);
                    continue;
                }
        
                //set current stream state to struct
                pthread_mutex_lock(&state.lock);
                state.is_streaming=1;
                state.fd=fd;
                state.g_data=&data;
                state.buff=buff;
                state.info=&info;
                pthread_mutex_unlock(&state.lock);

                //create a parallel thread to process streaming
                pthread_create(&g_pipeline,NULL,stream_thread,&state);

                break;

            case 2:
                if(ctrl_handler(fd)==-1){
                    continue;
                }
                break;

            case 3:
                if(!state.is_streaming)
                {
                    printf("Please start the stream first.\n");
                    continue;
                }

                pthread_mutex_lock(&state.lock);
                state.is_streaming=0;
                pthread_mutex_unlock(&state.lock);

                //wait for the thread to join after exc current iteration 
                pthread_join(g_pipeline,NULL);
                stop_streaming(fd);     
                for (int i =0; i < NUM_BUFFS; i++) {
                    munmap(buff[i], buff_size[i]);
                }
                gstream_deinit(&data);
                //Requesting 0 buffs flushes the buff memory internally
                req_buff(fd, 0);
                printf("You can now press -1 to quit the application\n");
                break;

            case 4:
                if(!state.is_streaming)
                {
                    printf("Please start the stream first.\n");
                    continue;
                }
                pthread_mutex_lock(&state.lock);
                state.snap=1;
                pthread_mutex_unlock(&state.lock);
                break;

            case -1:
                if(state.is_streaming){
                    printf("Turn off streaming before quiting.\n");
                    continue;
                }
                pthread_mutex_destroy(&state.lock);
                close(fd);
                return EXIT_SUCCESS;
            default:
                printf("Enter valid input!\n");
                sleep(1);
                continue;
        }
    }
    //breaks out of the loop in case of early exit.
    return EXIT_SUCCESS;
}
