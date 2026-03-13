//this program runs the main loop of the streaming app
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

/*
func name: validate_inp
args:
    int * input: pointer to store the validated input to
desc:  
    this function gets an input and validates if it is only integers
returns:
    0 on success
    -1 on fail
*/
int validate_inp(int * input){
    if (scanf("%d", input) != 1) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
        printf("Invalid input! Please enter a number.\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

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
    int ctrl_count=0;
    int res_count=0;
    int fmt_count=0;
    int32_t ctrl_val;
    uint32_t ctrl_id;
    int selected_ctrl=0;

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
        printf("[1]Start Streaming\t[2]Controls\n[3]Stop Streaming\t[4]Take a Snap\n[-1]Quit\n");
        printf("-----------------------------------------------------------------------------\n");
        printf("Choice: ");
        if(validate_inp(&input)==EXIT_FAILURE) continue;
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

                info.fmt_id=available[input].id;
                strncpy(info.fmt_name,available[input].format,16);

                //resolution query
                res_count= enum_resolution(fd,res,info.fmt_id);
                if(res_count==0 || res_count == EXIT_FAILURE) {
                    sleep(1);
                    return EXIT_FAILURE;
                }

                //resolution choice
                printf("[-1]Exit\n");
                printf("-----------------------------------------------------------------------------\n");
                printf("Input:");
                if(validate_inp(&input)==EXIT_FAILURE) continue;
                printf("-----------------------------------------------------------------------------\n");
                if(input==-1) continue;;

                info.height=res[input].height;
                info.width=res[input].width;

                if(set_formats(fd,info.width,info.height,info.fmt_id)){
                    sleep(1);
                    continue;
                }

                //request v4l2 to allocate n buffs
                num_buffs=req_buff(fd,NUM_BUFFS);

                if(num_buffs<NUM_BUFFS){
                    printf("[debug]Buffers allocated lesser than requested : %d\n",num_buffs);
                }

                for(int i=0;i<NUM_BUFFS;i++){
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
                start_streaming(fd);

                //gstream pipeline start

                if(gstream_setup(&data,&info)==EXIT_FAILURE){
                    printf("Gstream pipeline setup failed\n");
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
                ctrl_count=enum_cntrl(fd,ctrls);
                if(ctrl_count==0){
                    printf("No valid controls found.\n");
                    sleep(1);
                    continue;
                }
                while(1){
                    printf("Controls are:\n");
                    for(int i=0;i<ctrl_count;i++){
                        fprintf(stdout,"[%d]%s\n",i,ctrls[i].name);
                    }
                    printf("[-1]Back\n");
                    printf("Choose the control you wish to view: ");
                    int selected_ctrl;
                    if(validate_inp(&selected_ctrl)==EXIT_FAILURE){continue;}
                    if(selected_ctrl==-1){break;}
                    printf("-----------------------------------------------------------------------------\n");  
                    
                    if(ctrls[selected_ctrl].flags&V4L2_CTRL_FLAG_DISABLED){
                        printf("[Warning]Control flag disabled for %s.\n",ctrls[selected_ctrl].name);
                        sleep(1);
                        continue;
                    }
                    if(ctrls[selected_ctrl].flags&V4L2_CTRL_FLAG_READ_ONLY){
                        printf("[WARNING]Control is read only and cannot be modified.\n");
                        sleep(1);
                        continue;
                    } 
                    printf("%s\n",ctrls[selected_ctrl].name);
                    if(ctrls[selected_ctrl].type==V4L2_CTRL_TYPE_MENU){
                        struct  v4l2_querymenu menu={0};
                        menu.id=ctrls[selected_ctrl].id;
                        for(int i=ctrls[selected_ctrl].min;i<=ctrls[selected_ctrl].max;i++){
                            menu.index=i;
                            if(ioctl(fd,VIDIOC_QUERYMENU,&menu)!=-1){
                                printf("\t[MenuItem] [%d]: %s\n", menu.index, menu.name);
                            }
                        }
                    }
                    else{
                        printf("\tMin:%d Max:%d Step:%d Def:%d\n",ctrls[selected_ctrl].min,ctrls[selected_ctrl].max,ctrls[selected_ctrl].step,ctrls[selected_ctrl].def);
                    }
                    printf("-----------------------------------------------------------------------------\n");
                    while(1){
                        printf("Do you want to set or get the current value of %s?:\n[1]Set [2]Get [-1]Back: ",ctrls[selected_ctrl].name);
                        if(validate_inp(&input)==EXIT_FAILURE) continue;
                        printf("-----------------------------------------------------------------------------\n");
                        //set
                        if(input==-1){
                            break;
                        }
                        else if(input==1){
                            ctrl_id=ctrls[selected_ctrl].id;
                            get_ctrl(fd,ctrl_id);
                            printf("-----------------------------------------------------------------------------\n");
                            printf("Enter the value to set : ");
                            if(validate_inp(&ctrl_val)==EXIT_FAILURE) continue;
                            printf("-----------------------------------------------------------------------------\n");
                            set_ctrl(fd,ctrl_id,ctrl_val);
                            printf("-----------------------------------------------------------------------------\n");
                        }
                        //get
                        else if(input==2){
                            uint32_t ctrl_id=ctrls[selected_ctrl].id;
                            get_ctrl(fd,ctrl_id);
                            printf("-----------------------------------------------------------------------------\n");
                        }
                    }
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
    return EXIT_SUCCESS;
}
