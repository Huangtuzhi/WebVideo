#ifndef __capture__h
#define __capture__h

#include "types.h"

void *capture_open (const char *dev_name, int v_width, int v_height, PixelFormat  fmt);
int capture_get_picture (void *id, Picture *pic);
int capture_close (void *id);

int capture_get_output_ptr (void *id, unsigned char ***ptr, int **stride);
#endif
