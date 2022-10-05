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


/*
 * Epoch Time: 1664630770.036
 * Src: 192.168.0.171, Dst: 255.255.255.255
 * User Datagram Protocol, Src Port: 51312, Dst Port: 5728
 *    Destination Port: 5728
 * Data (13 bytes)
 *
 * 0000  31 36 36 34 36 33 30 37 36 39 39 38 32            1664630769982
 *
 *
 *  Epoch Time [ms]: 1664630770036 - 1664630769982 => 54 ms
 *
 *  Summary:
 *  UDP Broadcast (255.255.255.255) packet with destination port 5728 with Unix epoch timestamp [ms] string
 */

#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sqlite3.h>

#include "read-cs2-config.h"

#define MAXDG		4096	/* maximum datagram size */
#define MAXLINE         256
#define Z21PORT		5728

#define SQL_EXEC() do { \
ret = sqlite3_exec(db, sql, 0, 0, &err_msg); \
if (ret != SQLITE_OK) { \
    fprintf(stderr, "SQL error: %s\n", err_msg); \
    sqlite3_free(err_msg); \
    sqlite3_close(db); \
    return EXIT_FAILURE; \
}\
} while (0)

unsigned char udpframe[MAXDG];
char config_dir[MAXLINE] = "/www/config/";
extern struct loco_data_t *loco_data;

void print_usage(char *prg) {
    fprintf(stderr, "\nUsage: %s -c config_dir\n", prg);
    fprintf(stderr, "   Version 0.1\n\n");
    fprintf(stderr, "         -c <config_dir>     set the config directory - default %s\n", config_dir);
    fprintf(stderr, "         -v                  verbose\n\n");
}

int send_udp_broadcast(void) {
    int len, s, sa, sb;
    struct sockaddr_in baddr, saddr;
    struct timeval tv;
    char *timestamp;
    fd_set readfds;

    const int on = 1;

    int destination_port = Z21PORT;
    int local_port = Z21PORT;
    memset(&baddr, 0, sizeof(baddr));
    memset(&baddr, 0, sizeof(saddr));

    /* prepare udp destination struct with defaults */
    s = inet_pton(AF_INET, "255.255.255.255", &baddr.sin_addr);
    if (s <= 0) {
        if (s == 0) {
            fprintf(stderr, "UDP IP invalid\n");
        } else {
            fprintf(stderr, "invalid address family\n");
        }
        exit(EXIT_FAILURE);
    }

    baddr.sin_port = htons(destination_port);

    /* prepare UDP sending socket */
    if ((sb = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "Send UDP socket error %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (setsockopt(sb, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0) {
        fprintf(stderr, "UDP set socket option error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* prepare receiving socket */
    if ((sa = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
	fprintf(stderr, "UDP socket error: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }

    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(local_port);

    if (bind(sa, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        fprintf(stderr, "UDP bind error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* get timestamp */
    gettimeofday(&tv, NULL);

    unsigned long long millisecondsSinceEpoch =
    (unsigned long long)(tv.tv_sec) * 1000 +
    (unsigned long long)(tv.tv_usec) / 1000;

    asprintf(&timestamp, "%llu", millisecondsSinceEpoch);

    if (sendto(sb, timestamp, strlen(timestamp), 0, (struct sockaddr *)&baddr, sizeof(baddr)) != strlen(timestamp))
	fprintf(stderr, "UDP write error: %s\n", strerror(errno));

    FD_ZERO(&readfds);
    while (1) {
        FD_SET(sa, &readfds);

        if (select(sa + 1, &readfds, NULL, NULL, NULL) < 0) {
            fprintf(stderr, "select error: %s\n", strerror(errno));
        };

	if (FD_ISSET(sa, &readfds)) {
	   printf("received UDP packet\n");
	   if ((len = read(sa, udpframe, MAXDG)) > 0) {
		udpframe[len] = 0;
		printf("%s\n", udpframe);
	   }
	}
    }

    return EXIT_SUCCESS;
}

int sql_update_history(sqlite3 * db) {
    char *err_msg;
    int ret;

    /* delete existing data */
    char *sql="DELETE FROM update_history;";
    SQL_EXEC();

    sql = "INSERT INTO update_history VALUES(1, 'ios', '18.11.21, 06:41:18 Mitteleuropäische Normalzeit', '1.4.2', 1000, 100);";
    SQL_EXEC();
    return EXIT_SUCCESS;
}

int sql_insert_locos(sqlite3 * db) {
    char *err_msg;
    int i, j, n, ret;
    struct loco_data_t *l;
    char *sql;

    i = 1;
    j = 1;
    char *ip_s = "192.168.0.9";
    char *uuid_s = "42849456-5902-4F87-951F-616E57387CA1.png";

    /* delete existing data */
    sql ="DELETE FROM vehicles;";
    SQL_EXEC();
    sql ="DELETE FROM functions;";
    SQL_EXEC();

    for (l = loco_data; l != NULL; l = l->hh.next) {
	asprintf(&sql, "INSERT INTO vehicles VALUES(%d, '%s', '%s', 0, %d, %d, 1, %d, '', '', 0, '', '', '', '', '', '', '', '', '', '', '', "
		       "0, '', 0, '%s', 0, 0, 0, 786, 0, 0, 1024, '', 0, 0);", i, l->name, uuid_s, l->tmax, l->uid, i - 1, ip_s);
	printf("%s\n", sql);
	SQL_EXEC();
	for (n = 0; n < 32; n++) {
	    if (l->function[n].type) {
		asprintf(&sql, "INSERT INTO functions VALUES( %d, %d, %d, '', %d.0, %d, 'bugle', %d, %d, %d);",
				j, i, 0,   l->function[n].duration, n,    n, 1, 0);
		printf("%s\n", sql);
		SQL_EXEC();
		j++;
	    }
	}
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
    send_udp_broadcast();
    sqlite3_close(db);
    return EXIT_SUCCESS;
}
