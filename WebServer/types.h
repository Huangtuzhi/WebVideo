#ifndef __types__h
#define __types__h

//YUVͼ���ʽ
struct YUV
{
	unsigned char *data[4];
	int stride[4];
};
typedef struct YUV Picture;
#endif
