#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>


int server, client, fd_max;
struct sockaddr_in servaddr, cli; 
socklen_t len = sizeof(cli);

typedef struct s_client
{
	int id;
	char *msg;
}	t_client;

t_client clients[1024];

char buffer2[1024], buffer[1024];
int limit = 0;

fd_set fd_master, fd_read, fd_write;



void err_msg(char *msg)
{
	if (msg)
		write(2, msg, strlen(msg));
	else
    		write(2, "Fatal error", 11);
	write(2, "\n", 1);
    exit(1);
}

int extract_message(char **buf, char **msg)
{
    char    *newbuf;
    int    i;

    *msg = 0;
    if (*buf == 0)
        return (0);
    i = 0;
    while ((*buf)[i])
    {
        if ((*buf)[i] == '\n')
        {
            newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
            if (newbuf == 0)
                err_msg(NULL);
            strcpy(newbuf, *buf + i + 1);
            *msg = *buf;
            (*msg)[i + 1] = 0;
            *buf = newbuf;
            return (1);
        }
        i++;
    }
    return (0);
}

char *str_join(char *buf, char *add)
{
    char    *newbuf;
    int        len;

    if (buf == 0)
        len = 0;
    else
        len = strlen(buf);
    newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
    if (newbuf == 0)
        err_msg(NULL);
    newbuf[0] = 0;
    if (buf != 0)
        strcat(newbuf, buf);
    free(buf);
    strcat(newbuf, add);
    return (newbuf);
}

void send_all(int sender, char *msg)
{
    for(int fd = 0; fd <= fd_max; fd++)
    {
        if (fd != sender && FD_ISSET(fd, &fd_write))
            send(fd, msg, strlen(msg), 0);
    }
}

int main(int ac, char **av) {
    if (ac != 2)
        err_msg("Wrong number of arguments");

    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == -1)
        err_msg(NULL);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
    servaddr.sin_port = htons(atoi(av[1]));
      if ((bind(server, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
        err_msg(NULL);
    if (listen(server, 0) != 0)
        err_msg(NULL);
    FD_ZERO(&fd_master);
    FD_SET(server, &fd_master);
    fd_max = server;
    while (1)
    {
        fd_read = fd_write = fd_master;
        if (select(fd_max + 1, &fd_read, &fd_write, NULL, NULL) < 0)
            continue;
        for(int fd = 0; fd <= fd_max; fd++)
        {
            if (FD_ISSET(fd, &fd_read))
	    {
            	if (fd == server)
            	{
                	client = accept(server, (struct sockaddr *)&cli, &len);
	                if (client >= 0)
        	        {
                	    if (client > fd_max)
                        	fd_max = client;
			    clients[client].id = limit++;
        	            clients[client].msg = NULL;
                	    sprintf(buffer, "server: client %d just arrived\n", clients[client].id);
			    send_all(client, buffer);
			    FD_SET(client, &fd_master);
			    break;
                	}
            	}
            	else
           	 {
                	int byte = recv(fd, buffer2, 1023, 0);
              	  if (byte <= 0)
		  {
                    sprintf(buffer, "server: client %d just left\n", clients[fd].id);
                    send_all(fd, buffer);
                    free(clients[fd].msg);
                    FD_CLR(fd, &fd_master);
                    close(fd);
                    break;
		  }
		  buffer2[byte] = 0;
		  clients[fd].msg = str_join(clients[fd].msg, buffer2);
		  char *msg = NULL;
		  while(extract_message(&(clients[fd].msg), &msg))
		  {
                    sprintf(buffer, "client %d: ", clients[fd].id);
                    send_all(fd, buffer);
                    send_all(fd, msg);
                    free(msg);
		  }
		 }
            }
        }
    }
    return 0;
}


