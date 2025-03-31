#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <unistd.h>

static int callback(void *data, int argc, char **argv, char **azColName)
{
    int i;
    fprintf(stderr, "%s: ", (const char *)data);

    for (i = 0; i < argc; i++)
    {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }

    printf("\n");
    return 0;
}

void exec_error(int rc, char *zErrMsg, char *sucessMessage)
{
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        fprintf(stdout, "%s\n", sucessMessage);
    }
}

void create_tables(sqlite3 *db)
{
    char *zErrMsg = 0;
    int rc;
    char *sql;

    /* Create SQL statement for genre table */
    sql = "CREATE TABLE IF NOT EXISTS GENRE("
          "GENRE_ID INT PRIMARY KEY     NOT NULL,"
          "NAME           TEXT    NOT NULL);";

    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);

    exec_error(rc, zErrMsg, "Table GENRE created successfully");

    /* Create SQL statement for movie table */
    sql = "CREATE TABLE IF NOT EXISTS MOVIE("
          "MOVIE_ID INT PRIMARY KEY     NOT NULL,"
          "TITLE           TEXT    NOT NULL,"
          "DIRECTOR        TEXT NOT NULL,"
          "YEAR         INT NOT NULL );";

    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);

    exec_error(rc, zErrMsg, "Table MOVIE created successfully");

    /* Create SQL statement for movie genre relationship table */
    sql = "CREATE TABLE IF NOT EXISTS MOVIE_GENRE("
          "MOVIE_GENRE_ID INT PRIMARY KEY     NOT NULL,"
          "GENRE_ID           INT    NOT NULL,"
          "MOVIE_ID        INT NOT NULL,"
          "FOREIGN KEY (GENRE_ID) REFERENCES GENRE(GENRE_ID) ON DELETE CASCADE,"
          "FOREIGN KEY (MOVIE_ID) REFERENCES MOVIE(MOVIE_ID) ON DELETE CASCADE );";

    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);

    exec_error(rc, zErrMsg, "Table MOVIE_GENRE created successfully");
}

int connect_database(sqlite3 **db)
{
    int rc;

    rc = sqlite3_open("../src/streaming.db", db);

    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(*db));
        return (0);
    }
    else
    {
        fprintf(stderr, "Opened database successfully\n");
    }
}

void exec_query(sqlite3 *db, char *sql)
{
    char *zErrMsg = 0;
    int rc;
    const char *data = "Callback function called";

    rc = sqlite3_exec(db, sql, callback, (void *)data, &zErrMsg);

    exec_error(rc, zErrMsg, "Operation done successfully");
    // TODO: to list actions we need to return data with the results
}

void close_db(sqlite3 *db)
{
    sqlite3_close(db);
    fprintf(stdout, "Closing database.\n");
}
