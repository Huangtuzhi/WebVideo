#ifndef __sender__h
#define __sender__h

#ifdef __cplusplus
extern "C"{
#endif

void *sender_open(const char *target_ip,int target_port);
void sender_close(void *snd);
int sender_send(void *snd,const void *data,int len);

#ifdef __cplusplus
}
#endif

#endif
