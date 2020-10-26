#include "unp.h"

void sock5(int);
void sig_chld(int);

int main(int argc, char **argv) {
	int listenfd, connfd;
	socklen_t addrlen, clilen;
	struct sockaddr *cliaddr;
	pid_t childpid;
	char *port = "8000";

	listenfd = Tcp_listen(NULL, port, &addrlen);
	printf("server listen at %s\n", port);

	cliaddr = Malloc(addrlen);

	Signal(SIGCHLD, sig_chld);

	for ( ; ; ) {
		clilen = addrlen;
		connfd = Accept(listenfd, cliaddr, &clilen);

		if ( (childpid = Fork()) == 0) { // child
			Close(listenfd);
			sock5(connfd);
			exit(0);
		}
		Close(connfd);
	}
}

void sig_chld(int signo) {
	pid_t pid;
	int stat;

	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0) {
		//
	}
	return;
}

void sock5(int connfd) {
	str_echo(connfd);
}
