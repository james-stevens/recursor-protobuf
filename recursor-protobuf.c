/*******************************************************************
*	(c) Copyright 2009-now JRCS Ltd  - See LICENSE for details   *
********************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/mman.h>

#include "log_message.h"
#include "socket_server.h"
#include "client.h"
#include "stats.h"

#define DEFAULT_SRC_PORT 7011
#define DEFAULT_DST_PORT 9000
#define MAX_PROCESSES 200
#define DEFAULT_STATS_PATH "/var/run/recursor-protobuf.prom"

int interupt = 0;
void sig(int s) { interupt=s; }


void usage()
{
	puts("Usage:");
	puts("-i <socket>       - Listen here for PDNS Recursor to connect, supports IPv4, IPv6 or Unix socket (named or anonymous/unnamed)");
	puts("-o <socket>       - Connect to vector here, supports IPv4, IPv6 or Unix socket (named or anonymous/unnamed)");
	puts("-l <log-level>    - see 'log_message.h' for values, preceed with 'x' to specify a hex value, 'x200001' = Normal log level to stderr");
	puts("-p <path>         - Path name to save Prometheus style metrics to, default = `/var/run/recursor-protobuf.prom`");
	puts("-t <secs>         - Period in seconds to write Prometheus stats, default = 30");
	puts("-D                - Debug mode, prevent forking");
	exit(1);
}



int main(int argc,char * argv[])
{
int max_procs = MAX_PROCESSES;
time_t stats_interval = 30;
char stats_path[PATH_MAX];
loglvl_t level = MSG_DEBUG|MSG_NORMAL|MSG_STDOUT|MSG_HIGH|MSG_FILE_LINE;
struct sigaction sa;
int prevent_fork = 0;
struct net_addr_st from_ni,to_ni;

	init_log(argv[0],level);

	memset(&from_ni,0,sizeof(struct net_addr_st));
	memset(&to_ni,0,sizeof(struct net_addr_st));
	strcpy(stats_path,DEFAULT_STATS_PATH);

	time_t now = time(NULL);

	memset(&sa,0,sizeof(sa));
	sa.sa_handler = sig;

	signal(SIGPIPE,SIG_IGN);
	sigaction(SIGINT,&sa,NULL);
	sigaction(SIGTERM,&sa,NULL);

	sa.sa_flags |= SA_RESTART;
	sigaction(SIGHUP,&sa,NULL);

	int opt;
	while((opt=getopt(argc,argv,"l:i:o:Dx:")) > 0)
		{
		switch(opt)
			{
			default  : usage(); exit(-1); break;
			case 'x' : max_procs = atoi(optarg); break;
			case 'l' : level = LEVEL(optarg); init_log(argv[0],level); break;
			case 'D' : prevent_fork = 1; break;
			case 'i' :
				if (decode_net_addr(&from_ni,optarg,DEFAULT_SRC_PORT) < 0) {
					logmsg(MSG_ERROR,"ERROR: Invalid input address '%s'\n",optarg);
					usage(); }
				break;
			case 'o' :
				if (decode_net_addr(&to_ni,optarg,DEFAULT_DST_PORT) < 0) {
					logmsg(MSG_ERROR,"ERROR: Invalid output address '%s'\n",optarg);
					usage(); }
				break;
			}
		}

	size_t stats_sz = sizeof(struct stats_st)*max_procs;
	struct stats_st *stats = mmap(0, stats_sz,  PROT_READ | PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	if (stats==MAP_FAILED) { logmsg(MSG_ERROR,"ERROR: mmap failed - %s\n",ERRMSG); exit(1); }
	for(int i=0;i<max_procs;i++) stats[i].pid = 0;

	if (!from_ni.is_type) decode_net_addr(&from_ni,"127.0.0.1",DEFAULT_SRC_PORT);
	if (!to_ni.is_type) decode_net_addr(&from_ni,"127.0.0.1",DEFAULT_DST_PORT);

	int sock = tcp_server_any(&from_ni,1);
	if (!sock) {
		logmsg(MSG_ERROR,"ERROR: Failed to open listening socket");
		usage(); }

	time_t next_stats = now + stats_interval;
	while(!interupt) {
		int ret,client_fd;
		now = time(NULL);
		if (now >= next_stats) {
			logmsg(MSG_DEBUG,"stats:\n");
			next_stats = now + stats_interval;
			}

		pid_t dead_pid;
		while ((dead_pid = waitpid(0,&ret,WNOHANG)) > 0) stats_clear_pid_slot(stats,max_procs,dead_pid);

		if ((ret = read_poll(sock,1000)) < 0) break;
		if (ret==0) continue;

		if ((client_fd = accept(sock,NULL,NULL)) <= 0) break;

		struct stats_st *client_stats = stats_find_spare_slot(stats,max_procs);

		// this is strictly for debugging only
		if (prevent_fork) {
			run_client(client_fd,&to_ni,client_stats);
			continue;
			}

		pid_t client_pid = fork();
		if (client_pid < 0) break;
		if (client_pid == 0) {
			close(sock);
			exit(run_client(client_fd,&to_ni,client_stats));
			}
		if (client_stats) client_stats->pid = client_pid;
		close(client_fd);
		}

	shutdown(sock,SHUT_RDWR); close(sock);
	if ((from_ni.is_type==1)&&(from_ni.addr.path[0]=='/')) unlink(from_ni.addr.path);

	munmap(stats,stats_sz);

	return 0;
}
