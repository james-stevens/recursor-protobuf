#include <stdio.h>

#include "log_message.h"
#include "stats.h"
#include "string.h"



int stats_find_pid_slot(struct stats_st *stats,int max_procs,pid_t pid)
{
	for(int i=0;i<max_procs;i++) if (stats[i].pid==pid) return i;
	return -1;
}



int stats_clear_pid_slot(struct stats_st *stats,int max_procs,pid_t pid)
{
	int i = stats_find_pid_slot(stats,max_procs,pid);
	if (i >= 0) stats[i].pid = 0;
	return i;
}



struct stats_st * stats_find_spare_slot(struct stats_st *stats,int max_procs)
{
	int i = stats_find_pid_slot(stats,max_procs,0);
	if (i < 0) return NULL;

	memset(&stats[i],0,sizeof(struct stats_st));
	return &stats[i];
}



void stats_total(struct stats_st *total,struct stats_st *stats,int max_procs)
{
	memset(total,0,sizeof(struct stats_st));
	for(int i=0;i<max_procs;i++) {
		total->in_bytes += stats->in_bytes;
		total->out_bytes += stats->out_bytes;
		total->in_pkts += stats->in_pkts;
		total->out_pkts += stats->out_pkts;
		total->lost_bytes += stats->lost_bytes;
		total->lost_pkts += stats->lost_pkts;
		}
}



void stats_write_to_prom(char * path,char * server_id,struct stats_st *stats,int max_procs)
{
struct stats_st total;

	stats_total(&total,stats,max_procs);
	logmsg(MSG_DEBUG,"stats: %lu %lu %lu %lu %lu %lu\n",
		total.in_bytes,total.out_bytes,total.in_pkts,total.out_pkts,total.lost_bytes,total.lost_pkts);

	FILE * fp = fopen(path,"w");
	if (!fp) {
		logmsg(MSG_ERROR,"ERROR: Failed to write stats file '%s' - %s\n",path,ERRMSG);
		return; }

	fprintf(fp,"# HELP recursor_protobuf_in_bytes Bytes read in from PDNS Recursor\n");
	fprintf(fp,"# TYPE recursor_protobuf_in_bytes counter\n");
	fprintf(fp,"recursor_protobuf_in_bytes{server_id=\"%s\"} %lu",server_id,total.in_bytes);
	fprintf(fp,"# HELP recursor_protobuf_out_bytes Bytes of JSON written out to output\n");
	fprintf(fp,"# TYPE recursor_protobuf_out_bytes counter\n");
	fprintf(fp,"recursor_protobuf_out_bytes{server_id=\"%s\"} %lu",server_id,total.out_bytes);
	fprintf(fp,"# HELP recursor_protobuf_in_pkts Frames read in from PDNS Recursor\n");
	fprintf(fp,"# TYPE recursor_protobuf_in_pkts counter\n");
	fprintf(fp,"recursor_protobuf_in_pkts{server_id=\"%s\"} %lu",server_id,total.in_pkts);
	fprintf(fp,"# HELP recursor_protobuf_out_pkts JSON ojects written to output\n");
	fprintf(fp,"# TYPE recursor_protobuf_out_pkts counter\n");
	fprintf(fp,"recursor_protobuf_out_pkts{server_id=\"%s\"} %lu",server_id,total.out_pkts);
	fprintf(fp,"# HELP recursor_protobuf_lost_bytes Bytes of JSON that failed to output\n");
	fprintf(fp,"# TYPE recursor_protobuf_lost_bytes counter\n");
	fprintf(fp,"recursor_protobuf_lost_bytes{server_id=\"%s\"} %lu",server_id,total.lost_bytes);
	fprintf(fp,"# HELP recursor_protobuf_lost_pkts JSON objects that failed to output\n");
	fprintf(fp,"# TYPE recursor_protobuf_lost_pkts counter\n");
	fprintf(fp,"recursor_protobuf_lost_pkts{server_id=\"%s\"} %lu",server_id,total.lost_pkts);
	fclose(fp);
}
