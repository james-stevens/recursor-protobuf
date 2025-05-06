/*******************************************************************
*    (c) Copyright 2009-now JRCS Ltd  - See LICENSE for details   *
********************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>
#include <jansson.h>

#include "protobuf2json.h"
extern size_t base64_decode(char *dst, const char *src, size_t src_len);
#include "dnsmessage.pb-c.h"

#include "log_message.h"
#include "socket_client.h"
#include "stats.h"
#include "client.h"

#define MAX_JSON 25000
#define MAX_READ 70000

extern int interupt;

static struct stats_st *my_stats = NULL;

struct buffer_st {
	int len,pos,hdr_len;
	uint8_t data[MAX_READ];
	};

int buffered_data_out = 0;
int dst_sock = -1;
struct net_addr_st dst_ni;


static void reconnect_socket()
{
	logmsg(MSG_DEBUG,"dst_sock = %d\n",dst_sock);
	dst_sock = SockOpenAny(&dst_ni,NULL);
	set_blocking(dst_sock,0);
	if (dst_sock < 0) logmsg(MSG_ERROR,"ERROR: Failed to connect to vector\n");
	else logmsg(MSG_DEBUG,"Yeah - connected to vector\n");
}



// do we have enough header length bytes & enough data bytes
static int have_packet(struct buffer_st *buf)
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
static int fix_one_field(ProtobufCBinaryData *input, json_t *json_object,char *json_key, fix_type_e key_type)
{
char buffer[500],output[500],*new_val = NULL;
int af_type = 0;

	logmsg(MSG_DEBUG,"fix_one_field '%s', %lu bytes\n",json_key,input->len);

	switch(key_type) {
		case fix_type_basic:
			memcpy(buffer,input->data,input->len);
			buffer[input->len] = 0;
			new_val = buffer;
			break;
		case fix_type_ip_addr:
			if (input->len==4) af_type = AF_INET;
			if (input->len==16) af_type = AF_INET6;
			logmsg(MSG_DEBUG,"af = %d (%d,%d), key_type=%d\n",af_type,AF_INET,AF_INET6,key_type);
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

	logmsg(MSG_DEBUG,"Fixing '%s' to '%s'\n",json_key,new_val);

	json_object_set(json_object,json_key,json_new_value);
	json_decref(json_new_value);
	return 0;
}



static int has_ip_addr_rr(PBDNSMessage__DNSResponse *response)
{
	if ((!response)||(!response->has_rcode)||(response->rcode)) return 0;
	if ((!response->n_rrs)||(!response->rrs)) return 0;

	for(size_t i=0;i<response->n_rrs;i++) {
		PBDNSMessage__DNSResponse__DNSRR *rr = response->rrs[i];
		if ((rr->has_class_)&&(rr->class_==C_IN)
		  &&(rr->has_type)&&((rr->type==T_A)||(rr->type==T_AAAA))) return 1;
		}
	return 0;
}



static void fix_json_fields(PBDNSMessage * pdnsmsg,json_t *json_object)
{
	fix_one_field(&pdnsmsg->serveridentity,json_object,"serverIdentity",fix_type_basic);
	fix_one_field(&pdnsmsg->from,json_object,"from",fix_type_ip_addr);
	fix_one_field(&pdnsmsg->to,json_object,"to",fix_type_ip_addr);

	if ((pdnsmsg->type != PBDNSMESSAGE__TYPE__DNSResponseType)||(!has_ip_addr_rr(pdnsmsg->response))) return;

	json_t *json_response = json_object_get(json_object,"response");
	if ((!json_response)||(!json_is_object(json_response))) return;

	json_t *json_rrs = json_object_get(json_response,"rrs");
	if ((!json_rrs)||(!json_is_array(json_rrs))) return;
	logmsg(MSG_DEBUG,"json rrs array found\n");

	size_t json_index;
	json_t *json_array_item;
	json_array_foreach(json_rrs, json_index, json_array_item) {
		uint8_t buffer[50];
		ProtobufCBinaryData input;

		json_t *json_rdata = json_object_get(json_array_item,"rdata");
		const char *value_string = json_string_value(json_rdata);

		input.len = base64_decode((char *)buffer,value_string,strlen(value_string));
		input.data = buffer;

		logmsg(MSG_DEBUG,"rdata '%s' decodes to %lu bytes\n",value_string,input.len);
		fix_one_field(&input,json_array_item,"rdata",fix_type_ip_addr);
		}
}



static int process_packet(struct buffer_st *buf)
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
				if (dst_sock <= 0) reconnect_socket();
				if (dst_sock > 0) {
					json_len++;
					size_t wrote = write(dst_sock,json,json_len);
					if (wrote != json_len) {
						my_stats->lost_pkts++;
						my_stats->lost_bytes += json_len;
						if (errno != EAGAIN) {
							shutdown(dst_sock,SHUT_RDWR); close(dst_sock);
							dst_sock = -1;
							}
						}
					else {
						my_stats->out_pkts++;
						my_stats->out_bytes += json_len;
						}
					}
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



int run_client(int client_fd,struct net_addr_st *to_ni,struct stats_st *client_stats)
{
struct buffer_st buf;
struct stats_st buf_stats;;

	my_stats = (client_stats)?client_stats:&buf_stats;

	memcpy(&dst_ni,to_ni,sizeof(struct net_addr_st));
	reconnect_socket();

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
		my_stats->in_bytes += ret;
		while(have_packet(&buf)) {
			my_stats->in_pkts++;
			process_packet(&buf);
			}
		}

	shutdown(client_fd,SHUT_RDWR); close(client_fd);
	shutdown(dst_sock,SHUT_RDWR); close(dst_sock);
	return 0;
}
