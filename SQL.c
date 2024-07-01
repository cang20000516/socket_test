#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

// 错误处理函数
void handle_error(int rc, sqlite3 *db) {
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        if (db) {
            sqlite3_close(db);
        }
        exit(1);
    }
}

// 查询回调函数
int callback(void *NotUsed __attribute__((unused)), int argc, char **argv, char **azColName) {
    for (int i = 0; i < argc; i++) {
        printf("%s : %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

// 插入数据函数
void database_insert(sqlite3 *db, const char *sql) {
    char *err_msg = 0;
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        exit(1);
    } else {
        printf("Records inserted successfully\n");
    }
}

// 查询数据函数
void database_select(sqlite3 *db, const char *sql) {
    char *err_msg = 0;
    int rc = sqlite3_exec(db, sql, callback, 0, &err_msg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        exit(1);
    }
}

int main() {
    sqlite3 *db;
    int rc;

    // 打开数据库
    printf("Opening database...\n");
    rc = sqlite3_open("test.db", &db);
    handle_error(rc, db);

    const char *drop_table_sql = "DROP TABLE IF EXISTS COMPANY;";
    printf("Dropping table if exists...\n");
    database_insert(db, drop_table_sql);

    // 创建表
    const char *create_table_sql = "CREATE TABLE IF NOT EXISTS COMPANY("
                                   "ID INT PRIMARY KEY     NOT NULL,"
                                   "NAME           TEXT    NOT NULL,"
                                   "AGE            INT     NOT NULL,"
                                   "ADDRESS        CHAR(50));";
    printf("Creating table...\n");
    database_insert(db, create_table_sql);

    // 插入数据
    const char *insert_sql = "INSERT INTO COMPANY (ID, NAME, AGE, ADDRESS) "
                             "VALUES (1, 'CX', 24, 'HongKong' );";
    database_insert(db, insert_sql);

    // 查询数据
    const char *select_sql = "SELECT * FROM COMPANY;";
    database_select(db, select_sql);

    // 关闭数据库
    sqlite3_close(db);

    return 0;
}