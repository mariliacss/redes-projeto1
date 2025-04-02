#include <sqlite3.h>
#include "database.h"

void create_db() {
	sqlite3 *db;
	
	connect_db(&db);
	create_tables(db);
	close_db(db);
}

void insert_movie() {

}

void add_genre_movie() {
	
}

void remove_movie(int id) {
	
}

void list_movies() {
	sqlite3 *db;
	char *sql;

	sql = "SELECT TITLE FROM MOVIE";
	
	connect_db(&db);
	exec_query(db, sql);
	close_db(db);
}

void list_movies_info() {
	sqlite3 *db;
	char *sql;

	sql = "SELECT * FROM MOVIE";
	
	connect_db(&db);
	// exec_query(db, sql);
	close_db(db);
}

void list_info_by_movie(int id) {
	sqlite3 *db;
	char *sql;

	sql = "SELECT * FROM MOVIE WHERE ID_MOVIE = ";
	
	connect_db(&db);
	// exec_query(db, sql);
	close_db(db);
}

void list_movie_by_genre(int id) {
    
}