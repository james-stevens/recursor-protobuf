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
	puts("-i <input-socket> - Recursor connect to here, supports IPv4, IPv6 or Unix socket (named or anonymous/unnamed)");
	puts("-o <input-socket> - Connect to vector here, supports IPv4, IPv6 or Unix socket (named or anonymous/unnamed)");
	puts("-l <log-level>    - see 'log_message.h' for values, preceed with 'x' to specify a hex value");
	puts("-D                - Debug mode, prevent forking");
	exit(1);
}

int main(int argc,char * argv[])
{
loglvl_t level = MSG_DEBUG|MSG_NORMAL|MSG_STDOUT|MSG_HIGH|MSG_FILE_LINE;
struct sigaction sa;
int prevent_fork = 0;
struct net_addr_st from_ni,to_ni;
char from_path[PATH_MAX];

	memset(&from_ni,0,sizeof(struct net_addr_st));
	memset(&to_ni,0,sizeof(struct net_addr_st));

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
	while((opt=getopt(argc,argv,"l:i:o:D")) > 0)
		{
		switch(opt)
			{
			default  : usage(); exit(-1); break;
			case 'l' : level = LEVEL(optarg); init_log(argv[0],level); break;
			case 'D' : prevent_fork = 1; break;
			case 'i' :
				if (decode_net_addr(&from_ni,optarg,DEFAULT_PORT) < 0) {
					logmsg(MSG_ERROR,"ERROR: Invalid input address '%s'\n",optarg);
					usage(); }
				break;
			case 'o' :
				if (decode_net_addr(&to_ni,optarg,DEFAULT_PORT) < 0) {
					logmsg(MSG_ERROR,"ERROR: Invalid output address '%s'\n",optarg);
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

		// this is strictly for debugging only
		if (prevent_fork) {
			run_client(client_fd);
			continue;
			}

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
