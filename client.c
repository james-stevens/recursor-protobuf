#include <stdio.h>
#include <unistd.h>
#include <arpa/nameser.h>
#include <jansson.h>

#include "protobuf2json.h"
extern size_t base64_decode(char *dst, const char *src, size_t src_len);

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


void fix_json_fields(PBDNSMessage *pdnsmsg,json_t *json_object)
{
const char *json_key;
json_t *json_object_value;

	json_object_foreach(json_object, json_key, json_object_value) {
		// logmsg(MSG_DEBUG,"json_key=%s %d\n",json_key,json_is_string(json_object_value));
		if (strcmp(json_key,"serverIdentity")==0) {
			char svr_id[260];
			const char* value_string = json_string_value(json_object_value);
			size_t len = base64_decode(svr_id,value_string,strlen(value_string));

			logmsg(MSG_DEBUG,"json_key=%s %s\n",json_key,svr_id);

			json_t *json_value = json_string(svr_id);
			json_object_set(json_object,json_key,json_value);
			json_decref(json_value);
			}
		}
}



int process_packet(struct buffer_st *buf)
{
int len = buf->len+buf->frame;

	if ((!buf->len)||(len > buf->pos)) {
		logmsg(MSG_ERROR,"ERROR: process_packet fail, len=%d, pos=%d\n",len,buf->pos);
		return -1; }

	PBDNSMessage * pdnsmsg = pbdnsmessage__unpack(NULL,buf->len,buf->data+buf->frame);
	if (pdnsmsg) {
		logmsg(MSG_DEBUG,"PBDNSMessage type=%d, inbytes=%lu\n",pdnsmsg->type,pdnsmsg->inbytes);

		char errbuf[250];
		json_t *json_object;
		int ret = protobuf2json_object((ProtobufCMessage *)pdnsmsg,&json_object,errbuf,sizeof(errbuf));

		fix_json_fields(pdnsmsg,json_object);
		logmsg(MSG_DEBUG,"protobuf2json_string gave %d\n",ret);
		char * json = json_dumps(json_object,0);
		if (json) {
			puts(json);
			free(json);
			}
		json_decref(json_object);
		pbdnsmessage__free_unpacked(pdnsmsg,NULL);
		}

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
