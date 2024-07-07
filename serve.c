#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdint.h>
#include <sqlite3.h>
#include <dbus/dbus.h>

// SQLite functions (from SQL.c)
void handle_error(int rc, sqlite3 *db) {
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        if (db) {
            sqlite3_close(db);
        }
        exit(1);
    }
}

int callback(void *NotUsed __attribute__((unused)), int argc, char **argv, char **azColName) {
    for (int i = 0; i < argc; i++) {
        printf("%s : %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

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

// Global SQLite database connection
sqlite3 *db;

// D-Bus connection
DBusConnection *conn;

// Initialize D-Bus connection
void init_dbus() {
    DBusError err;
    dbus_error_init(&err);

    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "D-Bus Connection Error: %s\n", err.message);
        dbus_error_free(&err);
    }
    if (conn == NULL) {
        exit(1);
    }
}

// Send message via D-Bus
void send_dbus_message(const char *message) {
    DBusMessage *msg;
    msg = dbus_message_new_signal("/test/signal/Object", "test.signal.Type", "Test");
    if (msg == NULL) {
        fprintf(stderr, "Message NULL\n");
        exit(1);
    }

    DBusMessageIter args;
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &message)) {
        fprintf(stderr, "Out of Memory!\n");
        exit(1);
    }

    if (!dbus_connection_send(conn, msg, NULL)) {
        fprintf(stderr, "Out of Memory!\n");
        exit(1);
    }

    dbus_connection_flush(conn);
    dbus_message_unref(msg);
}

void* recv_thread(void* argv) {
    int c = (intptr_t)argv;
    while(1) {
        char buff[128] = {0};
        int n = recv(c, buff, 127, 0);
        if (n <= 0) {
            break;
        }
        printf("recv(%d): %s\n", c, buff);

        // Insert received data into database
        char insert_sql[256];
        snprintf(insert_sql, sizeof(insert_sql), 
                 "INSERT INTO MESSAGES (CONTENT) VALUES ('%s');", buff);
        database_insert(db, insert_sql);

        // Send data via D-Bus
        send_dbus_message(buff);

        // Send response to client
        const char *response = "ok";
        send(c, response, strlen(response), 0);

        // Insert response into database
        snprintf(insert_sql, sizeof(insert_sql), 
                 "INSERT INTO RESPONSES (CONTENT) VALUES ('%s');", response);
        database_insert(db, insert_sql);
    }
    printf("client close\n");
    close(c);
    return NULL;
}

int main() {
    // Initialize SQLite database
    int rc = sqlite3_open("server.db", &db);
    handle_error(rc, db);

    // Create tables
    const char *create_messages_table = "CREATE TABLE IF NOT EXISTS MESSAGES "
                                        "(ID INTEGER PRIMARY KEY AUTOINCREMENT, "
                                        "CONTENT TEXT NOT NULL);";
    database_insert(db, create_messages_table);

    const char *create_responses_table = "CREATE TABLE IF NOT EXISTS RESPONSES "
                                         "(ID INTEGER PRIMARY KEY AUTOINCREMENT, "
                                         "CONTENT TEXT NOT NULL);";
    database_insert(db, create_responses_table);

    // Initialize D-Bus
    init_dbus();

    // Set up socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sockfd != -1);

    struct sockaddr_in saddr, caddr;
    memset(&saddr, 0, sizeof(saddr));

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(6000);
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int res = bind(sockfd, (struct sockaddr*)&saddr, sizeof(saddr));
    assert(res != -1);

    res = listen(sockfd, 5);
    assert(res != -1);

    while (1) {
        socklen_t len = sizeof(caddr);
        int c = accept(sockfd, (struct sockaddr*)&caddr, &len);
        if (c < 0) {
            continue;
        }

        printf("accept c = %d\n", c);
        pthread_t id;
        pthread_create(&id, NULL, recv_thread, (void*)(intptr_t)c);
        pthread_detach(id);
    }

    close(sockfd);
    sqlite3_close(db);
    return 0;
}