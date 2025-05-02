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

#define MAX_JSON 25000
#define MAX_READ 70000

extern int interupt;


struct buffer_st {
	int len,pos,hdr_len;
	uint8_t data[MAX_READ];
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



typedef enum { fix_type_basic, fix_type_ip_addr } fix_type_e;
int fix_one_field(ProtobufCBinaryData *input, json_t *json_object,char *json_key, fix_type_e key_type)
{
char buffer[500],output[500],*new_val = NULL;

	switch(key_type) {
		case fix_type_basic:
			memcpy(buffer,input->data,input->len);
			buffer[input->len] = 0;
			new_val = buffer;
			break;
		case fix_type_ip_addr:
			int af_type = 0;
			if (input->len==4) af_type = AF_INET;
			if (input->len==16) af_type = AF_INET6;
			if (af_type) {
				inet_ntop(af_type,input->data,output,sizeof(output));
				new_val=output;
				}
			break;
		default: return -2;
		}

	if (!new_val) return -3;

	json_t *json_new_value = json_string(new_val);
	if (!json_new_value) return -4;

	json_object_set(json_object,json_key,json_new_value);
	json_decref(json_new_value);
	return 0;
}



int has_ip_addr_rrs(PBDNSMessage__DNSResponse *response)
{
	if ((!response)||(!response->has_rcode)||(response->rcode)) return 0;
	if ((!response->n_rrs)||(!response->rrs)) return 0;

	for(int i=0;i<response->n_rrs;i++) {
		PBDNSMessage__DNSResponse__DNSRR *rr = response->rrs[i];
		if ((rr->has_class_)&&(rr->class_==C_IN)
		  &&(rr->has_type)&&((rr->type==T_A)||(rr->type==T_AAAA))) return 1;
		}
	return 0;
}


void fix_json_fields(PBDNSMessage * pdnsmsg,json_t *json_object)
{
	fix_one_field(&pdnsmsg->serveridentity,json_object,"serverIdentity",fix_type_basic);
	fix_one_field(&pdnsmsg->from,json_object,"from",fix_type_ip_addr);
	fix_one_field(&pdnsmsg->to,json_object,"to",fix_type_ip_addr);

	if ((pdnsmsg->type != PBDNSMESSAGE__TYPE__DNSResponseType)||(!has_ip_addr_rrs(pdnsmsg->response))) return;

	json_t *json_response = json_object_get(json_object,"response");
	if ((!json_response)||(!json_is_object(json_response))) return;
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
			char json[MAX_JSON];
			fix_json_fields(pdnsmsg,json_object);
			size_t json_len = json_dumpb(json_object,json,MAX_JSON,JSON_COMPACT);
			if (json_len) {
				char *p = json + json_len;
				*p++ = '\n'; *p++ = 0;
				printf("<%s>\n",json); fflush(stdout);
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

		if ((ret = read(client_fd,buf.data+buf.pos,MAX_READ-buf.pos)) <= 0) break;
		logmsg(MSG_DEBUG,"read %d bytes\n",ret);
		buf.pos += ret;
		while(have_packet(&buf)) process_packet(&buf);
		}

	shutdown(client_fd,SHUT_RDWR); close(client_fd);
	return 0;
}
