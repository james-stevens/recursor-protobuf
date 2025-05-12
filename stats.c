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
	logmsg(MSG_DEBUG,"STATS: Dead child %ld found at slot %d\n",pid,i);
	if (i >= 0) stats[i].pid = 0;
	return i;
}



struct stats_st * stats_find_spare_slot(struct stats_st *stats,int max_procs)
{
	int i = stats_find_pid_slot(stats,max_procs,0);
	if (i < 0) return NULL;

	memset(&stats[i],0,sizeof(struct stats_st));
	logmsg(MSG_DEBUG,"STATS: putting new pid in slot %d\n",i);
	return &stats[i];
}




void stats_log(loglvl_t level,struct stats_st *stats)
{
	logmsg(level,"stats: %d: bin=%lu bout=%lu pkin=%lu pkout=%lu lostb=%lu lostpk=%lu\n",
		stats->pid,stats->in_bytes,stats->out_bytes,stats->in_pkts,stats->out_pkts,stats->lost_bytes,stats->lost_pkts);
}




void stats_total(struct stats_st *total,struct stats_st *stats,int max_procs)
{
	memset(total,0,sizeof(struct stats_st));
	total->pid = getpid();
	for(int i=0;i<max_procs;i++) {
		if (!stats[i].pid) continue;
		stats_log(MSG_DEBUG,&stats[i]);
		total->in_bytes += stats[i].in_bytes;
		total->out_bytes += stats[i].out_bytes;
		total->in_pkts += stats[i].in_pkts;
		total->out_pkts += stats[i].out_pkts;
		total->lost_bytes += stats[i].lost_bytes;
		total->lost_pkts += stats[i].lost_pkts;
		}
}



void stats_write_to_prom(char * path,char * service,struct stats_st *stats,int max_procs)
{
struct stats_st total;
char tags[1000] = {0};

	stats_total(&total,stats,max_procs);
	stats_log(MSG_NORMAL,&total);
	if (!path[0]) return;

	FILE * fp = fopen(path,"w");
	if (!fp) {
		logmsg(MSG_ERROR,"ERROR: Failed to write stats file '%s' - %s\n",path,ERRMSG);
		return; }

	if (service[0]) {
		if (strchr(service,'=')!=NULL)
			snprintf(tags,sizeof(tags),"{%s}",service);
		else
			snprintf(tags,sizeof(tags),"{service=\"%s\"}",service);
		}

	fprintf(fp,"# HELP recursor_protobuf_in_bytes Bytes read in from PDNS Recursor\n");
	fprintf(fp,"# TYPE recursor_protobuf_in_bytes counter\n");
	fprintf(fp,"recursor_protobuf_in_bytes%s %lu\n",tags,total.in_bytes);
	fprintf(fp,"# HELP recursor_protobuf_out_bytes Bytes of JSON written out to output\n");
	fprintf(fp,"# TYPE recursor_protobuf_out_bytes counter\n");
	fprintf(fp,"recursor_protobuf_out_bytes%s %lu\n",tags,total.out_bytes);
	fprintf(fp,"# HELP recursor_protobuf_in_pkts Frames read in from PDNS Recursor\n");
	fprintf(fp,"# TYPE recursor_protobuf_in_pkts counter\n");
	fprintf(fp,"recursor_protobuf_in_pkts%s %lu\n",tags,total.in_pkts);
	fprintf(fp,"# HELP recursor_protobuf_out_pkts JSON ojects written to output\n");
	fprintf(fp,"# TYPE recursor_protobuf_out_pkts counter\n");
	fprintf(fp,"recursor_protobuf_out_pkts%s %lu\n",tags,total.out_pkts);
	fprintf(fp,"# HELP recursor_protobuf_lost_bytes Bytes of JSON that failed to output\n");
	fprintf(fp,"# TYPE recursor_protobuf_lost_bytes counter\n");
	fprintf(fp,"recursor_protobuf_lost_bytes%s %lu\n",tags,total.lost_bytes);
	fprintf(fp,"# HELP recursor_protobuf_lost_pkts JSON objects that failed to output\n");
	fprintf(fp,"# TYPE recursor_protobuf_lost_pkts counter\n");
	fprintf(fp,"recursor_protobuf_lost_pkts%s %lu\n",tags,total.lost_pkts);
	fclose(fp);
}
