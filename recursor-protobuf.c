#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>


#include "log_message.h"

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
            case 'f' : if (optarg[0]=='/') STRCPY(from_path,optarg); else decode_net_addr(&from_ni,optarg); break;
            }
		}

	while(!interupt) {
		now = time(NULL);
		}

	return 0;
}
