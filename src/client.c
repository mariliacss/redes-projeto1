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

#define MAXDATASIZE 256 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// formata a mensagem para ser enviada
int send_message(int sockfd, const char *message) {
    uint8_t len = strlen(message);
    if (len > 255) len = 255;

    uint8_t buffer[256];
    buffer[0] = len;
    memcpy(&buffer[1], message, len);

    ssize_t sent = send(sockfd, buffer, len + 1, 0);
    return sent == len + 1 ? 0 : -1;
}

// recebe mensagem no mesmo formato
int receive_message(int sockfd, char *buffer, size_t buffer_size) {
    uint8_t len;
    ssize_t received = recv(sockfd, &len, 1, MSG_WAITALL);
    if (received <= 0) return -1;

    if (len >= buffer_size) len = buffer_size - 1;

    received = recv(sockfd, buffer, len, MSG_WAITALL);
    if (received <= 0) return -1;

    buffer[len] = '\0';
    return 0;
}

// lê a entrada do console e envia para o servidor e espera a resposta
void read_input_and_send(int sockfd) {
    printf("｡･:*˚:✧｡ olá, qual filme você viu hoje? ｡･:*˚:✧｡\n\n");
	printf("digite a opção desejada e aperte enter: \n");
	printf("1 - cadastrar um filme \n");
	printf("2 - adicionar um gênero ao filme \n");
	printf("3 - remover um filme \n");
	printf("4 - listar todos os títulos \n");
	printf("5 - listar todas as informações dos filmes \n");
	printf("6 - listar informações de um filme específico \n");
	printf("7 - listar todos os filmes de um gênero \n");
	printf("8 - menu de opções \n");
	printf("0 - desconectar \n\n");

	char *input = NULL;
    size_t inputSize= 0;
	char buf[MAXDATASIZE];

    while(1) {
        ssize_t  charCount = getline(&input, &inputSize, stdin);
        input[charCount-1] = 0;

        if(charCount > 0) {
            if(strcmp(input, "0") == 0)
                break;
			// envia a entrada lida para o servidor
			send_message(sockfd, input);
        }

		while (1) {
			// fica esperando a resposta até receber um __END__
			if (receive_message(sockfd, buf, sizeof(buf)) != 0) break;
		
			if (strcmp(buf, "__END__") == 0) break;
		
			printf("%s\n", buf);
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