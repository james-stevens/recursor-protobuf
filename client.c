#include <stdio.h>
#include <unistd.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>
#include <jansson.h>

#include "protobuf2json.h"
extern size_t base64_decode(char *dst, const char *src, size_t src_len);

#include "log_message.h"
#include "dnsmessage.pb-c.h"
#include "client.h"

#define MAXBUF 100000

extern int interupt;


struct buffer_st {
	int len,pos,hdr_len;
	uint8_t data[MAXBUF];
	};



// do we have enough header length bytes & enough data bytes
int have_packet(struct buffer_st *buf)
{
	logmsg(MSG_NONE,"%d bytes in store\n",buf->pos);

	if (buf->pos < buf->hdr_len) return 0;
	if (!buf->len) {
		uint8_t *p = buf->data;
		if (buf->hdr_len==4) GETLONG(buf->len,p); else GETSHORT(buf->len,p);
		logmsg(MSG_DEBUG,"I see %d bytes\n",buf->len);
		}

	if ((buf->len+buf->hdr_len) <= buf->pos) {
		logmsg(MSG_DEBUG,"have packet = 1\n");
		return 1; }

	return 0;
}



// decode serverIdentity from binary to text & decode IP from/to from binary to presentation
// we shouuld also decode IPs in response.rrs[].rdata, but don't yet
void fix_json_fields(PBDNSMessage *pdnsmsg,json_t *json_object)
{
char * keys[] = { "serverIdentity","to","from",NULL };

	for(int i=0;keys[i];i++) {
		char buffer[500],output[500],*new_val = NULL;
		char * json_key = keys[i];

		json_t *json_value = json_object_get(json_object,json_key);
		if (!json_value) continue;

		const char * value_string = json_string_value(json_value);
		int len = base64_decode(buffer,value_string,strlen(value_string));

		switch(i) {
			case 0: new_val = buffer; break;
			default:
				int af_type = 0;
				if (len==4) af_type = AF_INET;
				if (len==16) af_type = AF_INET6;
				if (af_type) {
					inet_ntop(af_type,buffer,output,sizeof(output));
					new_val=output;
					}
				break;
			}

		if (new_val) {
			json_t *json_new_value = json_string(new_val);
			if (json_new_value) {
				json_object_set(json_object,json_key,json_new_value);
				json_decref(json_new_value);
				}
			}
		}
}



int process_packet(struct buffer_st *buf)
{
int len = buf->len+buf->hdr_len;

	if ((!buf->len)||(len > buf->pos)) {
		logmsg(MSG_ERROR,"ERROR: process_packet fail, len=%d, pos=%d\n",len,buf->pos);
		return -1; }

	PBDNSMessage * pdnsmsg = pbdnsmessage__unpack(NULL,buf->len,buf->data+buf->hdr_len);
    logmsg(MSG_DEBUG,"PBDNSMessage type=%d, inbytes=%lu\n",pdnsmsg->type,pdnsmsg->inbytes);

	if (pdnsmsg) {
		char errbuf[250];
		json_t *json_object;
		int ret = protobuf2json_object((ProtobufCMessage *)pdnsmsg,&json_object,errbuf,sizeof(errbuf));
		if (ret) {
			logmsg(MSG_ERROR,"ERROR: protobuf2json_object failed with code %d\n",ret);
		} else {
			fix_json_fields(pdnsmsg,json_object);
			char * json = json_dumps(json_object,JSON_COMPACT);
			if (json) {
				puts(json);
				free(json);
				}
			json_decref(json_object);
			pbdnsmessage__free_unpacked(pdnsmsg,NULL);
			}
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
	buf.hdr_len = 2;

	while(!interupt) {
		int ret;

		ret = read_poll(client_fd,1000);
		if (ret < 0) break;
		if (ret == 0) continue;

		if ((ret = read(client_fd,buf.data+buf.pos,MAXBUF-buf.pos)) <= 0) break;
		logmsg(MSG_DEBUG,"read %d bytes\n",ret);
		buf.pos += ret;
		while(have_packet(&buf)) process_packet(&buf);
		}

	shutdown(client_fd,SHUT_RDWR); close(client_fd);
	return 0;
}
