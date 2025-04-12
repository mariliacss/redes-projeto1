/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sqlite3.h>

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

#define MAXDATASIZE 256 // max number of bytes we can get at once 

void sigchld_handler(int s)
{
	(void)s; // quiet unused variable warning

	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// verifica se todos os dados foram realmente enviados e envia se não foram
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

// read response from client and keep wating until user exits
void read_response(int sockfd, sqlite3 *db) {
	char buf[MAXDATASIZE];

    while(1) {
		if (receive_message(sockfd, buf, sizeof(buf)) == 0) {
            printf("Recebido do cliente: %s\n", buf);

            // Echo ou resposta
            send_message(sockfd, "Mensagem recebida!");
        } else {
            printf("Cliente desconectado ou erro.\n");
            break;
        }

		// int op = atoi(buf);
		// char *message;
		// int len;

		// sprintf(message, "Opção %s escolhida\n", buf);

		// len = strlen(message);

		// switch(op) {
		// 	case 5: {
		// 		printf("opção 5: listar todos os filmes\n");
		// 		// send_result(sockfd, db, "SELECT * FROM genres");
		// 		// if (sendall(sockfd, message, &len)) {
		// 		// 	perror("sendall");
		// 		// 	printf("We only sent %d bytes because of the error!\n", len);
		// 		// } 
		// 		break;
		// 	}
		// 	default: printf("opção inexistente\n"); break;
		// }
    }

	close(sockfd); // closing client socket
    exit(0); // closing child process
}

void create_tables(sqlite3 *db) {
    char *zErrMsg = 0;
    char *sql;

    /* Create SQL statement for genre table */
    sql = "CREATE TABLE IF NOT EXISTS genres("
          "genre_id INT PRIMARY KEY     NOT NULL,"
          "name           TEXT    NOT NULL);";

    /* Execute SQL statement */
    if (sqlite3_exec(db, sql, NULL, 0, &zErrMsg) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "%s\n", "Table genres created successfully");
    }

    /* Create SQL statement for movie table */
    sql = "CREATE TABLE IF NOT EXISTS movies("
          "movie_id INT PRIMARY KEY     NOT NULL,"
          "title           TEXT    NOT NULL,"
          "director        TEXT NOT NULL,"
          "year         INT NOT NULL );";

    /* Execute SQL statement */
    if (sqlite3_exec(db, sql, NULL, 0, &zErrMsg) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "%s\n", "Table movies created successfully");
    }

    /* Create SQL statement for movie genre relationship table */
    sql = "CREATE TABLE IF NOT EXISTS movies_genres("
          "movie_genre_id INT PRIMARY KEY     NOT NULL,"
          "genre_id           INT    NOT NULL,"
          "movie_id        INT NOT NULL,"
          "FOREIGN KEY (genre_id) REFERENCES GENRE(genre_id) ON DELETE CASCADE,"
          "FOREIGN KEY (movie_id) REFERENCES MOVIE(movie_id) ON DELETE CASCADE );";

    /* Execute SQL statement */
    if (sqlite3_exec(db, sql, NULL, 0, &zErrMsg) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "%s\n", "Table movies_genres created successfully");
    }
}

// inserir gêneros padrões para serem vinculados com os filmes
void insert_genres(sqlite3 *db) {
    char *zErrMsg = 0;
    char *sql;

    /* Create SQL statement for genre table */
	sql = "INSERT INTO genres (genre_id,name) "
		  "VALUES (1, 'Ação'); "
		  "INSERT INTO genres (genre_id,name) "
		  "VALUES (2, 'Aventura'); "
		  "INSERT INTO genres (genre_id,name)"
		  "VALUES (3, 'Comédia');"
		  "INSERT INTO genres (genre_id,name)"
		  "VALUES (4, 'Documentário');"
		  "INSERT INTO genres (genre_id,name)"
		  "VALUES (5, 'Drama');"
		  "INSERT INTO genres (genre_id,name)"
		  "VALUES (6, 'Ficção científica');"
		  "INSERT INTO genres (genre_id,name)"
		  "VALUES (7, 'Musical');"
		  "INSERT INTO genres (genre_id,name)"
		  "VALUES (8, 'Romance');"
		  "INSERT INTO genres (genre_id,name)"
		  "VALUES (9, 'Terror');";

	/* Execute SQL statement */
    if (sqlite3_exec(db, sql, NULL, 0, &zErrMsg) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "%s\n", "Genre records created successfully");
    }
}

int main(void) {
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;
	sqlite3 *db;

	// conectando com o banco de dados
	if (sqlite3_open("streaming.db", &db) != SQLITE_OK) {
        fprintf(stderr, "Error open database: %s\n", sqlite3_errmsg(db));
        return 1;
    } else {
        fprintf(stderr, "Opened database successfully\n");
    }

	// criando tabelas e inserindo dados de Gênero caso não existam
	create_tables(db);
	insert_genres(db);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			read_response(new_fd, db);
		}
		close(new_fd);  // parent doesn't need this
	}

	sqlite3_close(db);

	return 0;
}