#include "stats.h"
#include "string.h"



int stats_find_pid_slot(struct stats_st *stats,int max_processes,pid_t pid)
{
	for(int i=0;i<max_processes;i++) if (stats[i].pid==pid) return i;
	return -1;
}



int stats_clear_pid_slot(struct stats_st *stats,int max_processes,pid_t pid)
{
	int i = stats_find_pid_slot(stats,max_processes,pid);
	if (i >= 0) stats[i].pid = 0;
	return i;
}



struct stats_st * stats_find_spare_slot(struct stats_st *stats,int max_processes)
{
	int i = stats_find_pid_slot(stats,max_processes,0);
	if (i < 0) return NULL;

	memset(&stats[i],0,sizeof(struct stats_st));
	return &stats[i];
}
