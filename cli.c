#include	"unp.h"

void str_cli(FILE *fp, int sockfd)
{
	int maxfdp1, stdineof;
	fd_set rset;
	char buf[MAXLINE];
	int n;

	stdineof = 0;
	FD_ZERO(&rset);
	for( ; ; )
	{
		if(stdineof == 0)
			FD_SET(fileno(fp), &rset);
		FD_SET(sockfd, &rset);
		maxfdp1 = max(fileno(fp), sockfd) + 1;
		Select(maxfdp1, &rset, NULL, NULL, NULL);

		if(FD_ISSET(sockfd, &rset))
		{
			if((n = Read(sockfd, buf, MAXLINE)) == 0)
			{
				if(stdineof == 1)
					return;
				else
					err_quit("str_cli: server terminated prematurely");
			}
			Write(fileno(stdout), buf, n);
		}
		if(FD_ISSET(fileno(fp), &rset))
		{
			if((n = Read(fileno(fp), buf, MAXLINE)) == 0 || strncmp(buf, "/quit\n", n) == 0)
			{
				stdineof = 1;
				Shutdown(sockfd, SHUT_WR);
				FD_CLR(fileno(fp), &rset);
				continue;
			}
			Writen(sockfd, buf, n);
		}
	}	
}

int main(int argc, char **argv)
{
	int sockfd;
	char name[20];
	struct sockaddr_in servaddr;

	if(argc != 4)
		err_quit("usage: tcpcli <IPaddress> <port> <name>");

	strcpy(name, argv[3]);	

	sockfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));
	Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

	Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));

	Writen(sockfd, name, 20);
	
	printf("hi everyone?\n");
	str_cli(stdin, sockfd);

	exit(0);
}
