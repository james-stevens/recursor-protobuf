#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

#include "log_message.h"
#include "socket_server.h"
#include "client.h"

#define DEFAULT_PORT 7011

int interupt = 0;
void sig(int s) { interupt=s; }


void usage()
{
	puts("Usage:");
	exit(1);
}

int main(int argc,char * argv[])
{
loglvl_t level = MSG_DEBUG|MSG_NORMAL|MSG_STDOUT|MSG_HIGH|MSG_FILE_LINE;
struct sigaction sa;
int prevent_fork = 0;
struct net_addr_st from_ni;
char from_path[PATH_MAX];

	memset(&from_ni,0,sizeof(struct net_addr_st));

	time_t now = time(NULL);

	memset(&sa,0,sizeof(sa));
	sa.sa_handler = sig;

	signal(SIGPIPE,SIG_IGN);
	sigaction(SIGINT,&sa,NULL);
	sigaction(SIGTERM,&sa,NULL);

	sa.sa_flags |= SA_RESTART;
	sigaction(SIGHUP,&sa,NULL);

	init_log(argv[0],level);

	int opt;
	while((opt=getopt(argc,argv,"l:Dc:f:t:L:d:U:")) > 0)
		{
		switch(opt)
			{
			default  : usage(); exit(-1); break;
			case 'l' : level = LEVEL(optarg); init_log(argv[0],level); break;
			case 'D' : prevent_fork = 1; break;
			case 'i' :
				if (decode_net_addr(&from_ni,optarg,DEFAULT_PORT) < 0) {
					logmsg(MSG_ERROR,"ERROR: Invalid from address '%s'\n",optarg);
					usage(); }
				break;
			}
		}

	if (!from_ni.is_type) decode_net_addr(&from_ni,"127.0.0.1",DEFAULT_PORT);

	int sock = 0;
	if (from_ni.is_type) sock = tcp_server_any(&from_ni,1);
	if (from_path[0]) {
		unlink(from_path);
		sock = init_unix_server(from_path,50,1);
		}

	if (!sock) {
		logmsg(MSG_ERROR,"ERROR: Failed to open listening socket");
		usage(); }

	while(!interupt) {
		int ret,client_fd;
		now = time(NULL);

		while (waitpid(0,&ret,WNOHANG) > 0);

		if ((ret = read_poll(sock,1000)) < 0) break;
		if (ret==0) continue;

		if ((client_fd = accept(sock,NULL,NULL)) <= 0) break;

		pid_t client_pid = fork();
		if (client_pid < 0) break;
		if (client_pid == 0) {
			close(sock);
			exit(run_client(client_fd));
			}
		close(client_fd);
		}

	shutdown(sock,SHUT_RDWR); close(sock);
	if (from_path[0]) unlink(from_path);
	return 0;
}
