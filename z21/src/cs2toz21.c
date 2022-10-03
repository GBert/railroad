/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 */

/*
 * CS2 lokonotive.cs2 to Z21 SQLite converter
 */

#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sqlite3.h>

#include "read-cs2-config.h"

#define MAXLINE         256

#define SQL_EXEC() do { \
ret = sqlite3_exec(db, sql, 0, 0, &err_msg); \
if (ret != SQLITE_OK) { \
    fprintf(stderr, "SQL error: %s\n", err_msg); \
    sqlite3_free(err_msg); \
    sqlite3_close(db); \
    return EXIT_FAILURE; \
}\
} while (0)

char config_dir[MAXLINE] = "/www/config/";
extern struct loco_data_t *loco_data;

void print_usage(char *prg) {
    fprintf(stderr, "\nUsage: %s -c config_dir\n", prg);
    fprintf(stderr, "   Version 0.1\n\n");
    fprintf(stderr, "         -c <config_dir>     set the config directory - default %s\n", config_dir);
    fprintf(stderr, "         -v                  verbose\n\n");
}

int sql_update_history(sqlite3 * db) {
    char *err_msg;
    int ret;

    char *sql = "INSERT INTO update_history VALUES(1, 'ios', '18.11.21, 06:41:18 Mitteleuropäische Normalzeit', '1.4.2', 1000, 100);";
    SQL_EXEC();
    return EXIT_SUCCESS;
}

int sql_insert_locos(sqlite3 * db) {
    char *err_msg;
    int i, ret;
    struct loco_data_t *l;
    char *sql;

    i = 1;
    char *ip_s = "192.168.0.9";
    char *uuid_s = "42849456-5902-4F87-951F-616E57387CA1.png";

    for (l = loco_data; l != NULL; l = l->hh.next) {
	asprintf(&sql, "INSERT INTO vehicles VALUES(%d, %s, %s, 0, %d, %d, 1, %d, , , 0, , , , , , , , , , , , "
		 "0, , 0, %s, 0, 0, 0, 786, 0, 0, 1024, , 0, 0)", i, loco_data->name, uuid_s, loco_data->vmax, loco_data->uid, i - 1, ip_s);
	SQL_EXEC();
	i++;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    struct config_data_t config_data;
    char *loco_file;
    int opt, ret;
    sqlite3 *db;

    config_data.verbose = 1;

    while ((opt = getopt(argc, argv, "c:vh?")) != -1) {
	switch (opt) {
	case 'c':
	    if (strnlen(optarg, MAXLINE) < MAXLINE) {
		strncpy(config_dir, optarg, sizeof(config_dir) - 1);
	    } else {
		fprintf(stderr, "config file dir to long\n");
		exit(EXIT_FAILURE);
	    }
	    break;
	case 'v':
	    config_data.verbose = 1;
	    break;
	case 'h':
	case '?':
	    print_usage(basename(argv[0]));
	    exit(EXIT_SUCCESS);
	    break;
	}
    }

    if (asprintf(&loco_file, "%s/%s", config_dir, loco_name) < 0) {
	fprintf(stderr, "can't alloc buffer for loco_name: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }

    read_loco_data(loco_file, CONFIG_FILE);
    if (config_data.verbose == 1)
	printf("locos in CS2 File: %u\n", HASH_COUNT(loco_data));

    /* open empty SQLite database */

    ret = sqlite3_open("Loco_empty.sqlite", &db);

    if (ret != SQLITE_OK) {

	fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
	sqlite3_close(db);

	return EXIT_FAILURE;
    }
    sql_update_history(db);
    sql_insert_locos(db);
    sqlite3_close(db);
    return EXIT_SUCCESS;
}
