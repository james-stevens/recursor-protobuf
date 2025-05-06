/*******************************************************************
*	(c) Copyright 2009-now JRCS Ltd  - See LICENSE for details   *
********************************************************************/
#ifndef _INCLUDE_STATS_H_
#define _INCLUDE_STATS_H_

#include <stdint.h>
#include <unistd.h>

struct stats_st {
	uint64_t in_bytes,out_bytes,in_pkts,out_pkts,lost_bytes,lost_pkts;
	pid_t pid;
	};

extern int stats_find_pid_slot(struct stats_st *stats,int max_processes,pid_t pid);
extern struct stats_st * stats_find_spare_slot(struct stats_st *stats,int max_processes);
extern int stats_clear_pid_slot(struct stats_st *stats,int max_processes,pid_t pid);
extern void stats_write_to_prom(char * path,char * server_id,struct stats_st *stats,int max_procs);

#endif // _INCLUDE_STATS_H_
