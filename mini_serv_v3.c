#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>


typedef struct s_client {
  int   id;
  char  *msg;
} t_client;

int fd_max;
fd_set  read_fds, write_fds, fds;
int id = 0;

t_client  clients[1024];
char      send_buff[1024];
char      read_buff[1024];

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

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
				return (-1);
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
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}


void  print_error(char *msg) {
  if (msg == NULL)
    return;
  write(2, msg, strlen(msg));
  write(2, "\n", 1);
}

void  broadcast(int client_fd, char *msg) {
  for (int fd = 0; fd < fd_max + 1; fd++) {
    if (FD_ISSET(fd, &write_fds) && fd != client_fd) {
      send(fd, msg, strlen(msg), 0);
    }
  }
}

int main(int ac, char **av) {

  if (ac != 2) {
    print_error("Wrong number of arguments");
    return (EXIT_FAILURE);
  }

	int sockfd, connfd;
  socklen_t len;
	struct sockaddr_in servaddr, cli; 

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
    print_error("Fatal error");
		exit(EXIT_FAILURE); 
	} 

	bzero(&servaddr, sizeof(servaddr)); 
	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(*++av)); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
    print_error("Fatal error");
    close(sockfd);
		exit(EXIT_FAILURE); 
	} 
	if (listen(sockfd, 10) != 0) {
    print_error("Fatal error");
    close(sockfd);
		exit(EXIT_FAILURE); 
	}
  len = sizeof(cli);
  fd_max = sockfd;
  FD_ZERO(&fds);
  FD_SET(sockfd, &fds);
  while (1) {
    read_fds = fds;
    write_fds = fds;
    if (select(fd_max + 1, &read_fds, &write_fds, NULL, NULL) < 0) {
      continue ;
    }
    for (int fd = 0; fd < fd_max + 1; fd++) {
      if (FD_ISSET(fd, &read_fds)) {
        if (fd == sockfd) {
          connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
          if (connfd < 0)
            continue ;
          if (connfd > fd_max)
            fd_max = connfd;
          clients[connfd].id = id++;
          clients[connfd].msg = NULL;
          sprintf(send_buff, "server: client %d just arrived\n", clients[connfd].id);
          broadcast(connfd, send_buff);
          FD_SET(connfd, &fds);
          break ;
        } else {
          int re = recv(fd, read_buff, 1023, 0);
          if (re <= 0){
            sprintf(send_buff, "server: client %d just left\n", clients[fd].id);
            broadcast(fd, send_buff);
            free(clients[fd].msg);
            close(fd);
            FD_CLR(fd, &fds);
            break ;
          }
          read_buff[re] = 0;
          clients[fd].msg = str_join(clients[fd].msg, read_buff);
          char *msg = NULL; 
          while (extract_message(&clients[fd].msg, &msg))
          {
            sprintf(send_buff, "client %d: ", clients[fd].id);
            broadcast(fd, send_buff);
            broadcast(fd, msg);
            free(msg);
          }
        }
      }
    }
  }
}
