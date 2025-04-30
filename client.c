#include <stdio.h>
#include <unistd.h>
#include <arpa/nameser.h>

#include "log_message.h"
#include "dnsmessage.pb-c.h"
#include "client.h"

#define MAXBUF 100000

extern int interupt;


struct buffer_st {
	int len,pos,frame;
	uint8_t data[MAXBUF];
	};



int have_packet(struct buffer_st *buf)
{
	logmsg(MSG_DEBUG,"%d bytes in store\n",buf->pos);
	//     for(int l=0;l<buf->pos;l++) printf("%02X ",buf->data[l]); puts("");

	if (buf->pos < buf->frame) return 0;
	if (!buf->len) {
		uint8_t *p = buf->data;
		if (buf->frame==4) GETLONG(buf->len,p); else GETSHORT(buf->len,p);
		logmsg(MSG_DEBUG,"I see %d bytes\n",buf->len);
		}
	if ((buf->len+buf->frame) <= buf->pos) return 1;
	return 0;
}



int process_packet(struct buffer_st *buf)
{
int len = buf->len+buf->frame;

	if ((!buf->len)||(len > buf->pos)) {
		logmsg(MSG_ERROR,"ERROR: process_packet fail, len=%d, pos=%d\n",len,buf->pos);
		return -1; }

	PBDNSMessage * pdnsmsg = pbdnsmessage__unpack(NULL,buf->len,buf->data+buf->frame);
	logmsg(MSG_DEBUG,"PBDNSMessage type=%d, inbytes=%lu\n",pdnsmsg->type,pdnsmsg->inbytes);
	pbdnsmessage__free_unpacked(pdnsmsg,NULL);

	memmove(buf->data,buf->data+len,buf->pos-len);
	buf->pos -= len;
	buf->len = 0;
	return len;
}



int run_client(int client_fd)
{
struct buffer_st buf;

	buf.len = buf.pos = 0;
	buf.frame = 2;

	while(!interupt) {
		int ret;

		ret = read_poll(client_fd,1000);
		// logmsg(MSG_DEBUG,"poll gave %d\n",ret);
		if (ret < 0) break;
		if (ret == 0) continue;

		if ((ret = read(client_fd,buf.data+buf.pos,MAXBUF-buf.pos)) <= 0) break;
		logmsg(MSG_DEBUG,"read %d bytes\n",ret);
		buf.pos += ret;
		logmsg(MSG_DEBUG,"have packet = %d\n",have_packet(&buf));
		while(have_packet(&buf)) process_packet(&buf);
		}

	shutdown(client_fd,SHUT_RDWR); close(client_fd);
	return 0;
}
