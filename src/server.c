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

// envia resultados da query para o cliente (código dessa função adaptado do código fornecido pelo monitor)
void send_result(int client_sock, sqlite3 *db, const char *sql) {
    sqlite3_stmt *stmt;
    char buffer[MAXDATASIZE];
	int result_found = 0;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        snprintf(buffer, MAXDATASIZE, "Error SQL: %s\n", sqlite3_errmsg(db));
        send_message(client_sock, buffer);
        return;
    }

    int cols = sqlite3_column_count(stmt);

    // enviar filas de dados
    while (sqlite3_step(stmt) == SQLITE_ROW) {
		result_found = 1;
        buffer[0] = '\0';
        for (int i = 0; i < cols; i++) {
            const char *valor = (const char *)sqlite3_column_text(stmt, i);
            strcat(buffer, valor ? valor : "NULL");
            if (i < cols - 1) strcat(buffer, ",");
        }
		printf("%s\n", buffer);
		send_message(client_sock, buffer);
    }

	if (result_found == 0) send_message(client_sock, "nenhum resultado encontrado\n");

    sqlite3_finalize(stmt);
}

// insere ou remove dados na tabela do banco de dados
void insert_remove_data(char *sql, int client_sock, sqlite3 *db) {
	char *zErrMsg = 0;

	if (sqlite3_exec(db, sql, NULL, 0, &zErrMsg) != SQLITE_OK) {
		char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "SQL error: %s", zErrMsg);
		send_message(client_sock, error_msg);

		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	} else {
		fprintf(stdout, "%s\n", "operação realizada com sucesso!");
		send_message(client_sock, "operação realizada com sucesso!\n");
	}
}

// retorna o id para querys que precisam do id
char *get_id(int client_sock, sqlite3 *db, const char *sql) {
    sqlite3_stmt *stmt;
    char buffer[MAXDATASIZE];
	char *id_str = NULL;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        snprintf(buffer, MAXDATASIZE, "Error SQL: %s\n", sqlite3_errmsg(db));
		send_message(client_sock, buffer);
        return NULL;
    }

    int cols = sqlite3_column_count(stmt);

    // enviar filas de dados
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        buffer[0] = '\0';
        for (int i = 0; i < cols; i++) {
            const char *valor = (const char *)sqlite3_column_text(stmt, i);
            strcat(buffer, valor ? valor : "NULL");
            if (i < cols - 1) strcat(buffer, ",");
        }
		printf("%s\n", buffer);
		
        id_str = malloc(12);
        if (id_str != NULL) {
            snprintf(id_str, sizeof(buffer), "%s", buffer);
        }
    }

    sqlite3_finalize(stmt);
	return id_str;
}

// lê a entrada do cliente e responde de acordo
void read_response(int client_sock, sqlite3 *db) {
	char buf[MAXDATASIZE];

    while(1) {
		if (receive_message(client_sock, buf, sizeof(buf)) == 0) {
            printf("recebido do cliente: %s\n", buf);
			int op = atoi(buf);

			switch(op) {
				case 1: {
					printf("--- opção 1: cadastrar um filme ---\n\n");
					send_message(client_sock, "--- opção 1: cadastrar um filme ---\n");
					send_message(client_sock, "digite as informações desejadas da seguinte forma: nome do filme,diretor,número do gênero,ano de lançamento");
					send_message(client_sock, "escolha o gênero a partir da lista a seguir (insira o número)");
					send_result(client_sock, db, "SELECT * FROM genres");
					send_message(client_sock, "__END__");
					if (receive_message(client_sock, buf, sizeof(buf)) == 0) {
						int genre_id, year;
						char *movie_id, title[100], director[100], sql[512];

        				sscanf(buf, "%99[^,],%99[^,],%d,%d", title, director, &genre_id, &year);
						snprintf(sql, sizeof(sql), "INSERT INTO movies (title,director,year) VALUES ('%s', '%s', %d);", title, director, year);
							
						insert_remove_data(sql, client_sock, db);

						snprintf(sql, sizeof(sql), "SELECT movie_id FROM movies WHERE title LIKE '%%%s%%';", title);
						movie_id = get_id(client_sock, db, sql);

						if (movie_id != NULL) {
							snprintf(sql, sizeof(sql), "INSERT INTO movies_genres (genre_id,movie_id) VALUES (%d, %s);", genre_id, movie_id);
							insert_remove_data(sql, client_sock, db);
						}

						free(movie_id);
					} else {
						printf("cliente desconectado ou erro.\n");
						break;
					}
					break;
				}
				case 2: {
					printf("--- opção 2: adicionar um gênero ao filme ---\n\n");
					send_message(client_sock, "--- opção 2: adicionar um gênero ao filme ---\n");
					send_message(client_sock, "insira o número do gênero escolhido e o número do filme que você deseja inserir");
					send_message(client_sock, "formato: número do gênero,número do filme\n");
					send_result(client_sock, db, "SELECT * FROM genres");
					send_message(client_sock, "\n");
					send_result(client_sock, db, "SELECT movie_id, title FROM movies");
					send_message(client_sock, "__END__");
					if (receive_message(client_sock, buf, sizeof(buf)) == 0) {
						int genre_id, movie_id;
						char sql[512];

        				sscanf(buf, "%d,%d", &genre_id, &movie_id);
						snprintf(sql, sizeof(sql),
							"INSERT INTO movies_genres (genre_id,movie_id) VALUES (%d, %d);",
							genre_id, movie_id);

						insert_remove_data(sql, client_sock, db);
					} else {
						printf("cliente desconectado ou erro.\n");
						break;
					}
					break;
				}
				case 3: {
					printf("--- opção 3: remover um filme ---\n\n");
					send_message(client_sock, "--- opção 3: remover um filme ---\n");
					send_message(client_sock, "digite o nome do filme:");
					send_message(client_sock, "__END__");
					if (receive_message(client_sock, buf, sizeof(buf)) == 0) {
						char sql[512], *movie_id, msg[256];

						snprintf(sql, sizeof(sql), "SELECT movie_id FROM movies WHERE title LIKE '%%%s%%';",buf);

						movie_id = get_id(client_sock, db, sql);

						if (movie_id != NULL) {
        					snprintf(msg, sizeof(msg), "filme encontrado: identificador = %s\n", movie_id);
							printf("%s", msg);
							send_message(client_sock, msg);

							snprintf(sql, sizeof(sql), "DELETE FROM movies WHERE movie_id = %s;", movie_id);
							insert_remove_data(sql, client_sock, db);
						} else {
							printf("filme não encontrado.\n");
							send_message(client_sock, "filme não encontrado.\n");
						}

						free(movie_id);
					} else {
						printf("cliente desconectado ou erro.\n");
						break;
					}
					break;
				}
				case 4: {
					printf("--- opção 4: listar todos os títulos ---\n\n");
					send_message(client_sock, "--- opção 4: listar todos os títulos ---\n");
					send_result(client_sock, db, "SELECT movie_id, title FROM movies");
					break;
				}
				case 5: {
					printf("--- opção 5: listar todas as informações dos filmes ---\n\n");
					send_message(client_sock, "--- opção 5: listar todas as informações dos filmes ---\n");
					char *sql = "SELECT\n"
									"m.movie_id,\n"
									"m.title,\n"
									"m.director,\n"
									"m.year,\n"
									"GROUP_CONCAT(g.name, ',') AS genres\n"
								"FROM\n"
									"movies m\n"
								"JOIN\n"
									"movies_genres mg ON m.movie_id = mg.movie_id\n"
								"JOIN\n"
									"genres g ON mg.genre_id = g.genre_id\n"
								"GROUP BY\n"
									"m.movie_id;\n";
					send_result(client_sock, db, sql);
					break;
				}
				case 6: { 
					printf("--- opção 6: listar informações de um filme específico ---\n");
					send_message(client_sock, "--- opção 6: listar informação de um filme específico ---\n");
					send_message(client_sock, "digite o identificador do filme:");
					send_message(client_sock, "__END__");
					if (receive_message(client_sock, buf, sizeof(buf)) == 0) {
						int movie_id;
						char sql[512];

        				sscanf(buf, "%d", &movie_id);
						snprintf(sql, sizeof(sql), 
							"SELECT m.movie_id, m.title, m.director, m.year, GROUP_CONCAT(g.name, ',') AS genres\n"
							"FROM movies m\n"
							"JOIN movies_genres mg ON m.movie_id = mg.movie_id\n"
							"JOIN genres g ON g.genre_id = mg.genre_id\n"
							"WHERE m.movie_id = %d\n"
							"GROUP BY m.movie_id, m.title, m.director, m.year;", movie_id);

						send_result(client_sock, db, sql);
					}  else {
						printf("cliente desconectado ou erro.\n");
						break;
					}
					break;
				}
				case 7: {
					printf("--- opção 7: listar todos os filmes de um gênero ---\n");
					send_message(client_sock, "--- opção 7: listar todos os filmes de um gênero ---\n");
					send_result(client_sock, db, "SELECT * FROM genres");
					send_message(client_sock, "\ndigite o identificador do gênero desejado:");
					send_message(client_sock, "__END__");
					if (receive_message(client_sock, buf, sizeof(buf)) == 0) {
						int genre_id;
						char sql[512];

        				sscanf(buf, "%d", &genre_id);
						snprintf(sql, sizeof(sql),
							"SELECT m.movie_id, m.title\n"
							"FROM movies m\n"
							"JOIN movies_genres mg ON m.movie_id = mg.movie_id\n"
							"JOIN genres g ON mg.genre_id = g.genre_id\n"
							"WHERE g.genre_id = %d\n"
							"ORDER BY m.title;", genre_id);

						send_result(client_sock, db, sql);
					} else {
						printf("cliente desconectado ou erro.\n");
						break;
					}
					break;
				}
				case 8: {
					send_message(client_sock, "--- opção 8: menu de opções ---\n");
					send_message(client_sock, "digite a opção desejada e aperte enter: \n");
					send_message(client_sock, "1 - cadastrar um filme");
					send_message(client_sock, "2 - adicionar um gênero ao filme");
					send_message(client_sock, "3 - remover um filme");
					send_message(client_sock, "4 - listar todos os títulos");
					send_message(client_sock, "5 - listar informação de todos os filmes");
					send_message(client_sock, "6 - listar informação de um filme específico");
					send_message(client_sock, "7 - listar todos os filmes de um gênero");
					send_message(client_sock, "8 - menu de opções");
					send_message(client_sock, "0 - desconectar \n");
					break;
				}
				default: {
					printf("opção inexistente\n");
					send_message(client_sock, "--- opção inexistente ---\n");
					break;
				}
			}
			// avisar para o cliente que todas mensagens foram enviadas
			send_message(client_sock, "__END__");
        } else {
            printf("cliente desconectado ou erro.\n");
            break;
        }
    }

	close(client_sock); // fechando o socket do cliente
    exit(0); // fechando o processo filho
}

// crias as tebelas necessárias para o banco de dados
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
          "movie_id INTEGER PRIMARY KEY,"
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
          "movie_genre_id INTEGER PRIMARY KEY,"
          "genre_id           INT    NOT NULL,"
          "movie_id        INT NOT NULL,"
          "FOREIGN KEY (genre_id) REFERENCES genres(genre_id) ON DELETE CASCADE,"
          "FOREIGN KEY (movie_id) REFERENCES movies(movie_id) ON DELETE CASCADE,"
		  "UNIQUE (movie_id, genre_id)"
		  ");";

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

// criando o banco e as tabelas caso não tenham sido criados
void setup_database() {
	sqlite3 *db;

	// conectando com o banco de dados
	if (sqlite3_open("streaming.db", &db) != SQLITE_OK) {
        fprintf(stderr, "Error open database: %s\n", sqlite3_errmsg(db));
    } else {
        fprintf(stderr, "Opened database successfully\n");
    }

	// criando tabelas e inserindo dados de gênero caso não existam
	create_tables(db);
	insert_genres(db);

	sqlite3_close(db);
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

	setup_database();

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
			sqlite3 *db;

			close(sockfd); // child doesn't need the listener

			sqlite3_open("streaming.db", &db); // abrindo conexão exclusiva para o processo filho
			sqlite3_exec(db, "PRAGMA foreign_keys = ON;", 0, 0, 0); // habilitando remoção em cascata
			read_response(new_fd, db);
			sqlite3_close(db);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}