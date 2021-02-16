#include	"unp.h"

struct client
{
	int fd;
	char name[20];
	char addr[20];
	int port;
};

int main(int argc, char **argv)
{
	int i, j, maxi, maxfd, listenfd, connfd, sockfd;
	int nready, cnt = 0;
	struct client c[FD_SETSIZE];
	ssize_t n;
	fd_set rset, allset;
	char buf[MAXLINE], mesg[MAXLINE];
	char *word[3] = {NULL, };
	char *ptr;
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;

	if(argc != 2)
		err_quit("usage: tcpsrv <port>");

	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[1]));
	
	Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

	Listen(listenfd, LISTENQ);

	printf("[server address is %s:%d]\n", inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));

	maxfd = listenfd;
	maxi = -1;
	for(i = 0; i < FD_SETSIZE; i++)
		c[i].fd = -1;
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);
	for( ; ; )
	{
		rset = allset;
		nready = Select(maxfd + 1, &rset, NULL, NULL, NULL);

		if(FD_ISSET(listenfd, &rset))
		{
			clilen = sizeof(cliaddr);
			connfd = Accept(listenfd, (SA *) &cliaddr, &clilen);

			for(i = 0; i < FD_SETSIZE; i++)
				if(c[i].fd < 0)
				{
					c[i].fd = connfd;
					Read(c[i].fd, buf, 20);
					strcpy(c[i].name, buf);
					strcpy(c[i].addr, inet_ntoa(cliaddr.sin_addr));
					c[i].port = ntohs(cliaddr.sin_port);
					printf("%s is connected from %s\n", c[i].name, c[i].addr);
					cnt++;

					for(j = 0; j <= maxi; j++)
					{
						if(c[j].fd >= 0 && j != i)
						{
							sprintf(mesg, "[%s is connected]\n", c[i].name);
							Writen(c[j].fd, mesg, strlen(mesg));
						}
					}
					for(j = 0; j <= maxi; j++)
					{
						if(c[j].fd >= 0 && j != i)
						{
							sprintf(mesg, "[%s is connected]\n", c[j].name);
							Writen(c[i].fd, mesg, strlen(mesg));
						}
					}
					break;
				}
			if(i == FD_SETSIZE)
				err_quit("too many clients");

			FD_SET(connfd, &allset);
			if(connfd > maxfd)
				maxfd = connfd;
			if(i > maxi)
				maxi = i;

			if(--nready <= 0)
				continue;
		}
		for(i = 0; i <= maxi; i++)
		{
			if((sockfd = c[i].fd) < 0)
				continue;
			if(FD_ISSET(sockfd, &rset))
			{
				if((n = Read(sockfd, buf, MAXLINE)) == 0)
				{
					Close(sockfd);
					printf("%s is leaved at %s\n", c[i].name, c[i].addr);
					cnt--;

					for(j = 0; j <= maxi; j++)
					{
						if(c[j].fd >= 0 && j != i)
						{
							sprintf(mesg, "[%s is disconnected]\n", c[i].name);
							Writen(c[j].fd, mesg, strlen(mesg));
						}
					}
					FD_CLR(sockfd, &allset);
					c[i].fd = -1;
				}

				if(c[i].fd >= 0)
				{
					ptr = strtok(buf, " ");
					word[0] = ptr;
					ptr = strtok(NULL, " ");
					word[1] = ptr;
					ptr = strtok(NULL, "\n");
					word[2] = ptr;

					if(strcmp(word[0], "/smsg\0") == 0)
					{
						for(j = 0; j <= maxi; j++)
						{
							if(strcmp(c[j].name, word[1]) == 0)
							{
								sprintf(mesg, "[smsg from %s] %s\n", c[i].name, word[2]);
								Writen(c[j].fd, mesg, strlen(mesg));
							}
						}
					}
					else if(strncmp(buf, "/list\n", n) == 0)
					{
						sprintf(mesg, "[the number of current user is %d]\n", cnt);
						Writen(c[i].fd, mesg, strlen(mesg));
						for(j = 0; j <= maxi; j++)
						{
							if(c[j].fd >= 0)
							{
								sprintf(mesg, "[%s from %s:%d]\n", c[j].name, c[j].addr, c[j].port);
								Writen(c[i].fd, mesg, strlen(mesg));
							}
						}
					}
					else
					{
						for(j = 0; j <= maxi; j++)
						{
							if(c[j].fd >= 0 && j != i)
							{	
								sprintf(mesg, "[%s] ", c[i].name);
								Writen(c[j].fd, mesg, strlen(mesg));
								Writen(c[j].fd, buf, n);
							}
						}
					}
				}

				if(--nready <= 0)
					break;
			}
		}
	}
}
