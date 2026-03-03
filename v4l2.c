//program to capture a picture from the webcam according to the users parameters/formats

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <string.h>

#include "v4l2.h"

/*
func name:  query_capablities
args:
    int fd: file descriptor to the camera driver
desc:  
    this function checks if out needed formats/modes are supported by the driver
returns:
    NULL
*/

void query_capablities(int fd){
    struct v4l2_capability vcap;
    if (-1 == ioctl(fd, VIDIOC_QUERYCAP, &vcap)) {
		printf("[v4l2]Query capabilites\n");
	}

	// if (!(vcap.capabilities & V4L2_CAP_READWRITE)) {
	// 	printf("Device does not support read i/o\n");
	// }

	if (!(vcap.capabilities & V4L2_CAP_STREAMING)) {
		printf("[v4l2]Device does not support streaming i/o\n");
	}

    if(!(vcap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
        printf("[v4l2]Not Compatible\n");
    }
    return;
}

/*
func name:  enum_formats
args:
    int fd: file descriptor     to the camera driver
    struct pix_formats: structure to store all the formats names and ids supported 
desc:  
    this function lists all the formats available and stores the names,ids to the struct passed
returns:
    0 on success 
    -1 on error
*/
int enum_formats(int fd,struct pix_formats * available){
    struct v4l2_fmtdesc fmt={0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.index=0;
    printf("Available formats:\n");
    while(ioctl(fd,VIDIOC_ENUM_FMT,&fmt)!=-1){
        printf("[%d]%s\n",fmt.index,fmt.description);
        strncpy(available[fmt.index].format,(char *)fmt.description,sizeof(fmt.description));
        available[fmt.index].id=fmt.pixelformat;
        fmt.index++;
    }
    if(fmt.index==0) {
        printf("[v4l2]Error querying formats\n");
        return -1;
    }    
    return fmt.index;
}

/*
func name:  enum_reolution
args:
    int fd: file descriptor to the camera driver
    struct img_res: structure to store all the resolutions supported 
desc:  
    this function lists all the resolutions available and stores to the struct passed
returns:
    no of supported resolutions on success 
    -1 on error
*/
int enum_resolution(int fd,struct img_res * res, int format){
    struct v4l2_frmsizeenum frame={0};
    frame.pixel_format = format;
    frame.index=0;
    printf("Available resolutions:\n");
    printf("-----------------------------------------------------------------------------\n");
    while(ioctl(fd,VIDIOC_ENUM_FRAMESIZES,&frame)!=-1){
        if (frame.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
            printf("[%d]%d x %d\n",frame.index,frame.discrete.width,frame.discrete.height);
            res[frame.index].height=frame.discrete.height;
            res[frame.index].width=frame.discrete.width;
        }
        else {
            printf("  [%d] Unsupported resolution type (not discrete).\n", frame.index);
        }
        frame.index++;
    }
    if(frame.index==0) {
        printf("[v4l2]Error querying resolutions\n");
        return -1;
    }    
    return frame.index;
}

/*
func name:  enum_cntrl
args:
    int fd: file descriptor to the camera driver
    struct available: structure to store all the image control names and ids supported 
desc:  
    this function lists all the controls available and stores the names,ids to the struct passed
returns:
    no of supported controls on success 
*/
int enum_cntrl(int fd,struct img_ctrl * available){
    int count=0;
    struct v4l2_queryctrl ctrl={0};
    ctrl.id=V4L2_CTRL_FLAG_NEXT_CTRL;
    printf("Available controls:\n");
    printf("-----------------------------------------------------------------------------\n");
    while(ioctl(fd,VIDIOC_QUERYCTRL,&ctrl)!=-1){
        if(ctrl.flags&V4L2_CTRL_FLAG_DISABLED){
            printf("[v4l2]Control flag disabled for %s. Skipping..\n",ctrl.name);
            ctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
            // count++;
            continue;
        }
        if(ctrl.flags&V4L2_CTRL_FLAG_READ_ONLY){
            printf("[WARNING]Control [%d] is read only and cannot be modified.\n",count);
        } 
        printf("[%d]%s\n",count,ctrl.name);
        if(ctrl.type==V4L2_CTRL_TYPE_MENU){
            struct  v4l2_querymenu menu={0};
            menu.id=ctrl.id;
            for(int i=ctrl.minimum;i<=ctrl.maximum;i++){
                menu.index=i;
                if(ioctl(fd,VIDIOC_QUERYMENU,&menu)!=-1){
                    printf("\t[MenuItem] [%d]: %s\n", menu.index, menu.name);
                }
            }
        }
        else{
            printf("\tMin:%d Max:%d Step:%d Def:%d\n",ctrl.minimum,ctrl.maximum,ctrl.step,ctrl.default_value);
        }
        available[count].id=ctrl.id;
        strncpy(available[count].name,(char *)ctrl.name,sizeof(ctrl.name));
        ctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
        count++;
    }
    return count;
}
/*
func name:  set_control
args:
    int fd: file descriptor to the camera driver
    uint32_t ctrl_id: id of the control to set
    int32_t val: value to set the control to
desc:  
    this function sets the value to the control corresponding to the id passed
returns:
    0 on success 
    -1 on error
*/

int set_ctrl(int fd,uint32_t ctrl_id,int32_t val){
    struct v4l2_control vctrls; 
    vctrls.id=ctrl_id;
    printf("[debug]fmt_id:%d\n",vctrls.id);
    vctrls.value=val;
    if(ioctl(fd,VIDIOC_S_CTRL,&vctrls)==-1){
        printf("[v4l2]Eror setting controls\n");
        return -1;
    }
    printf("Successfully set cntrl to %d \n",vctrls.value);
    return 0;
}   

/*
func name:  get_control
args:
    int fd: file descriptor to the camera driver
    uint32_t ctrl_id: id of the control to get
desc:  
    this function gets the value of the control corresponding to the id passed
returns:
    0 on success 
    -1 on error
*/
int get_ctrl(int fd,uint32_t ctrl_id){
    struct v4l2_control vctrls;
    vctrls.id=ctrl_id;
    printf("%d\n",vctrls.id);
    if(ioctl(fd,VIDIOC_G_CTRL,&vctrls)==-1){
        printf("[v4l2]Error getting control val\n");
        return -1;
    }
    printf("The value of the selected control is : %d\n",vctrls.value);
    return vctrls.value;
}

/*
func name:  set_formats
args:
    int fd: file descriptor to the camera driver
    int width: width of the image to be taken
    int height: height of the image to be taken
    int pixformat: format of the image to be taken
desc:  
    this function sets the parameters/modes to use when running the camera to the driver
returns:
    NULL
*/
void set_formats(int fd,int width,int height,int pixformat){
    struct v4l2_format vformat={0};
    vformat.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vformat.fmt.pix.width=width;
    vformat.fmt.pix.height=height;
    vformat.fmt.pix.pixelformat=pixformat;
    vformat.fmt.pix.field=V4L2_FIELD_NONE;
    int res=ioctl(fd,VIDIOC_S_FMT,&vformat);
    if(res==-1)
    {
        printf("[v4l2]Error setting formats\n");
    }
    return;
}

/*
func name:  req_buff
args:
    int fd: file descriptor to the camera driver
    int count: number of buffers needed to be allocated
desc:  
    this function requests number of buffers to be allocated
returns:
    count of the buffers allocated
*/
int req_buff(int fd,int count){
    struct v4l2_requestbuffers req={0};
    req.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.count=count;
    req.memory=V4L2_MEMORY_MMAP;
    if(ioctl(fd,VIDIOC_REQBUFS,&req)==-1){
        printf("[v4l2]Error requesting buffer\n");
    }
    return req.count;
}

/*
func name:  query_buff
args:
    int fd: file descriptor to the camera driver
    int index: index of the buffer to be allocated
    unsigned char ** buffer: double pointer to the buffer to be allocated
desc:  
    this function queries the buffers and then allocates them
returns:
    length of the buffer allocated
*/
int query_buff(int fd,int index,unsigned char ** buffer){
    struct v4l2_buffer buff={0};
    buff.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buff.memory=V4L2_MEMORY_MMAP;
    buff.index=index;
    if(ioctl(fd,VIDIOC_QUERYBUF,&buff)==-1){
        printf("[v4l2]Error querying buffer\n");
    }
    *buffer= (uint8_t *)mmap(NULL,buff.length,PROT_READ|PROT_WRITE,MAP_SHARED,fd,buff.m.offset);
    return buff.length;
}

/*
func name:  queue_buff
args:
    int fd: file descriptor to the camera driver
    int index: index to the buffer
desc:  
    this function queues a buffer to input data from the camera
returns:
    no of bytes of the buffer used
*/
int queue_buff(int fd,int index){
    struct v4l2_buffer buff = {0};
	buff.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buff.memory = V4L2_MEMORY_MMAP;
	buff.index = index;

	if (ioctl(fd, VIDIOC_QBUF, &buff) == -1) {
		printf("[v4l2]Error Queueing Buffer\n");
	}
    return buff.bytesused;
}

/*
func name:  dequeue_buff
args:
    int fd: file descriptor to the camera driver
desc:  
    this function dequeues a buffer to output data 
returns:
    index of the buffer
*/
int dequeue_buff(int fd,int * bytes_deq){
    struct v4l2_buffer buff = {0};
	buff.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buff.memory = V4L2_MEMORY_MMAP;
	buff.index = 0;

	if (ioctl(fd, VIDIOC_DQBUF, &buff) == -1) {
		printf("[v4l2]Error DeQueueing Buffer\n");
        return -1;
	}
    *bytes_deq=buff.bytesused;
    return buff.index;
}

/*
func name: start_streaming
args:
    int fd: file descriptor to the camera driver
desc:  
    this function starts the video stream
returns:
    0
*/
int start_streaming(int fd){
    unsigned int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(fd,VIDIOC_STREAMON,&type)==-1){
        printf("[v4l2]Error starting stream\n");
    }
    return 0;
}

/*
func name: stop_streaming
args:
    int fd: file descriptor to the camera driver
desc:  
    this function stops the video stream
returns:
    0
*/
int stop_streaming(int fd){
    unsigned int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(fd,VIDIOC_STREAMOFF,&type)==-1){
        printf("Error stopping stream\n");
    }
    return 0;
}

/*
func name: openDev()
args:
    NULL
desc:  
    this function intializes the file descriptor and returns the fd
returns:
    -1 on fail
    fd on success
*/
int openDev(){
    int fd = open("/dev/video0", O_RDWR);
    if(fd==-1){
        printf("Unable to connect to device.\nExiting....");
        return -1;
    }
    return fd;
}

// int snap(int fd){
//     unsigned char *buff[3];
    
//     int input;
// 	int index;
// 	int num_buffs;
//     int buff_size;

//     struct pix_formats available[16]={0};
//     struct img_res res[16]={0};
//     struct img_ctrl ctrls[20];

//     query_capablities(fd);  

//     //format query
//     int fmt_count=enum_formats(fd,available);
//     char fmt_name[16];
//     int fmt_id;
//     if(fmt_count==0) {
//         printf("No valid format detected. Exiting...\n");
//         return 0;
//     }

//     //format choice
//     printf("[-1]Exit\nInput:");
//     scanf("%d",&input);
//     if(input==-1) return 0; 

//     fmt_id=available[input].id;
//     strncpy(fmt_name,available[input].format,16);

//     //resolution query
//     int res_count= enum_resolution(fd,res,fmt_id);
//     if(res_count==0) {
//         printf("No valid resolution detected. Exiting...\n");
//         return 0;
//     }

//     //resolution choice
//     printf("[-1]Exit\nInput:");
//     scanf("%d",&input);
//     if(input==-1) return 0;

//     int height=res[input].height;
//     int width=res[input].width;

//     set_formats(fd,width,height,fmt_id);

//     num_buffs=req_buff(fd,3);

//     if(num_buffs<3){
//         printf("Buffers allocated lesser than requested : %d\n",num_buffs);
//     }

//     for(int i=0;i<3;i++){
//         buff_size=query_buff(fd,i,&buff[i]);
//         printf("Bytes allocated for buff %d : %d\n",i,buff_size);
//         queue_buff(fd,i);
//     }

//     start_streaming(fd);

//     fd_set fds;
// 	FD_ZERO(&fds);
// 	FD_SET(fd, &fds);
// 	struct timeval tv = {0};
// 	tv.tv_sec = 2;
// 	int r = select(fd+1, &fds, NULL, NULL, &tv);

// 	if(-1 == r){
// 		printf("Waiting for Frame\n");
// 		return -1;
// 	}

//     int bytes_dque;
// 	index = dequeue_buff(fd,&bytes_dque);

//     printf("[BUFF %d] Amount of bytes dequeued: %d\n",index,bytes_dque);
//     char filename[64];
//     char extention[8];
//     //TODO add timestamp to name
//     if(fmt_id==V4L2_PIX_FMT_YUYV) strncpy(extention,"yuv",3);
//     else if(fmt_id==V4L2_PIX_FMT_MJPEG) strncpy(extention,"jpg",3);
//     sprintf(filename, "%s_.%s", fmt_name,extention);

// 	int file = open(filename, O_RDWR | O_CREAT, 0666);
// 	write(file, buff[index], bytes_dque);

// 	stop_streaming(fd);

// 	for (int i =0; i < 3; i++) {
// 		munmap(buff[i], buff_size);
// 	}
// 	close(file);
// 	close(fd);

// 	return 0;
// }
