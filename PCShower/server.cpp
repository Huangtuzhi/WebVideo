#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern "C"{
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include "capture.h"
#include "vcompress.h"
#include "sender.h"

/*Server：打开/dev/video0，获取图像，压缩，发送到localhost端口
 *
 *使用 320*240，fps=10 
*/

#define VIDEO_WIDTH 320
#define VIDEO_HEIGHT 240
#define VIDEO_FPS 10.0

#define TARGET_IP "202.201.13.130"
#define TARGET_PORT 3020

int main(int argc,char **argv)
{
	void *capture=capture_open("/dev/video0",VIDEO_WIDTH,VIDEO_HEIGHT,PIX_FMT_YUV420P);
	if(!capture){
		fprintf(stderr,"ERR:can't open '/dev/video0'\n");
		exit(-1);
	}

	void *encoder=vc_open(VIDEO_WIDTH,VIDEO_HEIGHT,VIDEO_FPS);
	if(!encoder){
		fprintf(stderr,"ERR:can't open X264 encoder\n");
		exit(-1);
	}

	void *sender=sender_open(TARGET_IP,TARGET_PORT);
	if(!sender){
		fprintf(stderr,"ERR:can't open sender for %s:%d\n",TARGET_IP,TARGET_PORT);
		exit(-1);
	}

	int tosleep=1000000/VIDEO_FPS;

	for(;;){
		//抓
		Picture pic;
		capture_get_picture(capture,&pic);

		//压
		const void *outdata;
		int outlen;
		int rc=vc_compress(encoder,pic.data,pic.stride,&outdata,&outlen);
		if(rc<0) continue;

		//发
		sender_send(sender,outdata,outlen);

		//等
		usleep(tosleep);
	}

	sender_close(sender);
	vc_close(encoder);
	capture_close(capture);

	return 0;
}




