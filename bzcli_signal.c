#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>


static struct sigaction SIGINT_action_default;
static struct sigaction SIGQUIT_action_default;
static struct sigaction SIGSTOP_action_default;


static void bzcli_SIGINT_handler(int sig)
{
	return;
}


void bzcli_signal_reset(void)
{
	sigaction(SIGINT,  &SIGINT_action_default, NULL);
	sigaction(SIGQUIT,  &SIGQUIT_action_default, NULL);
	sigaction(SIGSTOP,  &SIGSTOP_action_default, NULL);
}

void bzcli_signal_init(void)
{
	struct sigaction sa;

    sa.sa_handler = bzcli_SIGINT_handler;
    sa.sa_flags =  0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, &SIGINT_action_default); 

	sa.sa_handler = bzcli_SIGINT_handler;
    sa.sa_flags =  0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGQUIT, &sa, &SIGQUIT_action_default); 

	sa.sa_handler = bzcli_SIGINT_handler;
    sa.sa_flags =  0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSTOP, &sa, &SIGSTOP_action_default); 

	return;
}



