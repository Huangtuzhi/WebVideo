#ifndef __types__h
#define __types__h

//YUVÍ¼Ïñ¸ñÊ½
struct YUV
{
	unsigned char *data[4];
	int stride[4];
};
typedef struct YUV Picture;
#endif
