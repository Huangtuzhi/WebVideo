#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <linux/videodev2.h>
extern "C"{//在Cpp中引用C中的变量
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include "capture.h"

struct Buffer
{
	void *start;
	size_t length;
};
typedef struct Buffer Buffer;

struct Ctx
{
	int vid;
	int width, height;  //出图像大小
	struct SwsContext *sws; //用于转换
	int rows; //用于sws_scale()
	int bytesperrow;
	AVPicture pic_src, pic_target;
	Buffer bufs[2];
	PixelFormat fmt;
};
typedef struct Ctx Ctx;

void *capture_open(const char *dev_name,int t_width, int t_height, PixelFormat tarfmt)
{
	int id = open(dev_name, O_RDWR);
	if (id < 0) return 0;
	Ctx *ctx = new  Ctx;
	ctx->vid = id;

	//to query caps
	v4l2_capability caps;
	ioctl(id, VIDIOC_QUERYCAP, &caps);

	if (caps.capabilities & V4L2_CAP_VIDEO_CAPTURE){
		if(caps.capabilities & V4L2_CAP_READWRITE){
			//TODO:
		}
		if(caps.capabilities & V4L2_CAP_STREAMING){
			//检查是否支持 MMAP || USERPTR
			v4l2_requestbuffers bufs;
			memset(&bufs, 0, sizeof(bufs));
			bufs.count = 2;
			bufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			bufs.memory = V4L2_MEMORY_MMAP;
			if(ioctl(id, VIDIOC_REQBUFS, &bufs) < 0) {
				fprintf(stderr, "%s: don't support MEMORY_MMAP mode!\n",__func__);
				close(id);
				delete ctx;
				return 0;
			}

			fprintf(stderr, "%s: using MEMORY_MMAP mode, but cnt=%d\n",__func__,bufs.count);

			for(int i=0;i<2;i++)
			{
				v4l2_buffer buf;
				memset(&buf,0,sizeof(buf));
				buf.type=bufs.type;
				buf.memory=bufs.memory;
				if(ioctl(id,VIDIOC_QUERYBUF,&buf)<0){//获取到buf，并mmap到进程空间。
					fprintf(stderr,"%s:VIDIOC_QUERYBUF ERR\n",__func__);
					close(id);
					delete ctx;
					return 0;
				}

				ctx->bufs[i].length=buf.length;
				ctx->bufs[i].start=mmap(0,buf.length,PROT_READ|PROT_WRITE,MAP_SHARED,id,buf.m.offset);
			}
		}
		else{
			fprintf(stderr,"%s:cannot support read()/write() mode and streaming mode\n",__func__);
			close(id);
			delete ctx;
			return 0;
		}
	}
	else{
		fprintf(stderr,"%s:cannt support video capture!\n",__func__);
		close(id);
		delete ctx;
		return 0;
	}

	int rc;

	//enum all support image fmt
	v4l2_fmtdesc fmt_desc;
	uint32_t index=0;

#if 1
	do{
		fmt_desc.index=index;
		fmt_desc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
		rc=ioctl(id,VIDIOC_ENUM_FMT,&fmt_desc);

		if(rc>=0)
		{
			fprintf(stderr,"\t support %s\n",fmt_desc.description);
		}
		index++;
	}while (rc>0);
#endif
	v4l2_format fmt;
	fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	rc=ioctl(id,VIDIOC_G_FMT,&fmt);
	if(rc<0){
		fprintf(stderr,"%s:cannot VIDIOC_G_FMT...\n",__func__);
		return 0;
	}

	PixelFormat pixfmt=PIX_FMT_NONE;
	switch (fmt.fmt.pix.pixelformat){
	case V4L2_PIX_FMT_YUYV:
		pixfmt=PIX_FMT_YUYV422;
		break;

	case V4L2_PIX_FMT_MJPEG:
		//pixfmt=PIX_FMT_YUVJ422P
        break;
	}

	if(pixfmt==PIX_FMT_NONE){
		fprintf(stderr,"%s:cannot support %4s\n",__func__,(char *)&fmt.fmt.pix.pixelformat);
		return 0;
	}

	//构造转换器
	fprintf(stderr,"capture_width=%d,height=%d,stride=%d\n",fmt.fmt.pix.width,fmt.fmt.pix.height,fmt.fmt.pix.bytesperline);
	ctx->width=t_width;
	ctx->height=t_height;
	ctx->sws=sws_getContext(fmt.fmt.pix.width,fmt.fmt.pix.height,pixfmt,ctx->width,ctx->height,tarfmt,SWS_FAST_BILINEAR,0,0,0);//PIX_FMT_YUV420P 对应 X264_CSP_I420

	ctx->rows=fmt.fmt.pix.height;
	ctx->bytesperrow=fmt.fmt.pix.bytesperline;
	 
	   avpicture_alloc(&ctx->pic_target,tarfmt,ctx->width,ctx->height);

    //queue buf buf入列
    for(int i=0;i<sizeof(ctx->bufs)/sizeof(Buffer);i++){
	   v4l2_buffer buf;
	   memset(&buf,0,sizeof(buf));
	   buf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	   buf.memory=V4L2_MEMORY_MMAP;
	   buf.index=i;

	   if(ioctl(id,VIDIOC_QBUF,&buf)<0){
		   fprintf(stderr,"%s:VIDIOC_QBUF err\n",__func__);
		   exit(-1);
	   }
	}

	int type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(ioctl(id,VIDIOC_STREAMON,&type)<0){
		fprintf(stderr,"%s:VIDIOC_STREAMON err\n",__func__);
		exit(-1);
	}

	    ctx->fmt=tarfmt;//转换后的图像赋值

		return ctx;
}

int capture_get_picture(void *id,Picture *pic)
{
	Ctx *ctx=(Ctx*)id;
	v4l2_buffer buf;
	memset(&buf,0,sizeof(buf));
	buf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory=V4L2_MEMORY_MMAP;

	//Exchange a buffer with the driver
	if(ioctl(ctx->vid,VIDIOC_DQBUF,&buf)<0){
		fprintf(stderr,"%s:VIDIOC_DQBUF err\n",__func__);
		return -1;
	}

	ctx->pic_src.data[0]=(unsigned char *)ctx->bufs[buf.index].start;
	ctx->pic_src.data[1]=ctx->pic_src.data[2]=ctx->pic_src.data[3]=0;
	ctx->pic_src.linesize[0]=ctx->bytesperrow;
	ctx->pic_src.linesize[1]=ctx->pic_src.linesize[2]=ctx->pic_src.linesize[3]=0;

	//sws_scale
    int rs=sws_scale(ctx->sws,ctx->pic_src.data,ctx->pic_src.linesize,0,ctx->rows,ctx->pic_target.data,ctx->pic_target.linesize);

	//out
	for(int i=0;i<4;i++){
		pic->data[i]=ctx->pic_target.data[i];
		pic->stride[i]=ctx->pic_target.linesize[i];
	}

	//re queue buf
	if(ioctl(ctx->vid,VIDIOC_QBUF,&buf)<0){
		fprintf(stderr,"%s:VIDIOC_QBUF err\n",__func__);
		return -1;
	}

	return 1;
}


int capture_close(void *id)
{
	Ctx *ctx=(Ctx*)id;
	for(int i=0;i<sizeof(ctx->bufs)/sizeof(Buffer);i++){
		munmap(ctx->bufs[i].start,ctx->bufs[i].length);
	}
	avpicture_free(&ctx->pic_target);
	sws_freeContext(ctx->sws);
	delete ctx;

	return 1;
}

int capture_get_output_ptr(void *c,unsigned char ***ptr,int **ls)
{
	Ctx *ctx=(Ctx*)c;
	*ptr=ctx->pic_target.data;
	*ls=ctx->pic_target.linesize;
	return 1;
}
