/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 1024 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int sendall(int s, char *buf, int *len) {
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
} 

int send_message(int sockfd, const char *message) {
    uint8_t len = strlen(message);
    if (len > 255) len = 255;

    uint8_t buffer[256];
    buffer[0] = len;
    memcpy(&buffer[1], message, len);

    ssize_t sent = send(sockfd, buffer, len + 1, 0);
    return sent == len + 1 ? 0 : -1;
}

// Recebe mensagem no mesmo formato
int receive_message(int sockfd, char *buffer, size_t buffer_size) {
    uint8_t len;
    ssize_t received = recv(sockfd, &len, 1, MSG_WAITALL); // Primeiro byte: tamanho
    if (received <= 0) return -1;

    if (len >= buffer_size) len = buffer_size - 1;

    received = recv(sockfd, buffer, len, MSG_WAITALL);
    if (received <= 0) return -1;

    buffer[len] = '\0'; // Null-terminate para facilitar uso como string
    return 0;
}

// read input from console and send to server
// keep waiting until the user exit
void read_input_and_send(int sockfd) {
    printf("hello, what movie do you want to wacth today? \n");
	printf("please, type your action and hit enter: \n");
	printf("1 - insert a movie \n");
	printf("2 - add a genre to a movie \n");
	printf("3 - remove a movie \n");
	printf("4 - insert a movie \n");
	printf("5 - list all movies \n");
	printf("6 - list all movies informations \n");
	printf("7 - list information by movie \n");
	printf("8 - list all movies from a genre \n");
	printf("0 - exit \n");

	char *input = NULL;
    size_t inputSize= 0;
	char buf[MAXDATASIZE];

    while(1) {
        ssize_t  charCount = getline(&input, &inputSize, stdin);
        input[charCount-1] = 0;

        if(charCount > 0) {
            if(strcmp(input, "0") == 0)
                break;

			send_message(sockfd, input);
        }

		if (receive_message(sockfd, buf, sizeof(buf)) == 0) {
            printf("Servidor respondeu: %s\n", buf);
        } else {
            printf("Erro ou desconexão do servidor.\n");
            break;
        }
    }
}

int main(int argc, char *argv[]) {
	int sockfd;  
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	read_input_and_send(sockfd);

	close(sockfd);

	return 0;
}