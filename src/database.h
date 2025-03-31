#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>

int connect_database(sqlite3 **db);

void exec_query(sqlite3 *db, char *sql);

void close_db(sqlite3 *db);

#endif
