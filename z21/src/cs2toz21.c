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

/*
 * {"os":"android","appVersion":"1.4.6","deviceName":"Z21 Emulator","deviceType":"OpenWRT",
 * "request":"device_information_request","buildNumber":6076,"apiVersion":1}
 */

/*
 * {"owningDevice":{"os":"ios","appVersion":"1.4.6","deviceName":"iPad von Gerhard","deviceType":"iPad7,3",
 *  "request":"device_information_request","buildNumber":6076,"apiVersion":1},"fileName":"MeineAnlage.z21",
 *  "request":"file_transfer_info","fileSize":744444}
 */

#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sqlite3.h>

#include "read-cs2-config.h"
#include "fmapping.h"

#define MAXDG		4096	/* maximum datagram size */
#define MAXLINE         256
#define Z21PORT		5728

char z21_fstring_none[] = "none";

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

int send_tcp_data(struct sockaddr_in *client_sa) {
    int buf_len, fd, st;
    struct sockaddr_in server_sa;
    char *offer;
    void *p;
    struct stat file_stat;
    char filename[] = {"/tmp/Data.z21"};

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
	fprintf(stderr, "can't open Z21 data %s: %s\n", filename, strerror(errno));
	return EXIT_FAILURE;
    }

    if (fstat(fd, &file_stat) < 0) {
	fprintf(stderr, "can't get Z21 data %s size: %s\n", filename, strerror(errno));
	return EXIT_FAILURE;
    }

    printf("Filesize %ld", file_stat.st_size);

    /* prepare TCP client socket */
    if ((st = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	fprintf(stderr, "can't create TCP socket: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }

    memset(&server_sa, 0, sizeof(server_sa));
    server_sa.sin_family = AF_INET;
    /* copy IP adresse from UDP request */
    memcpy(&server_sa.sin_addr.s_addr, &client_sa->sin_addr.s_addr, 4);
    server_sa.sin_port = htons(Z21PORT);

    if (connect(st, (struct sockaddr *)&server_sa, sizeof(server_sa))) {
	fprintf(stderr, "can't connect to TCP socket : %s\n", strerror(errno));
	return(EXIT_FAILURE);
    }
    asprintf(&offer, "{\"owningDevice\":{\"os\":\"android\",\"appVersion\":\"1.4.6\",\"deviceName\":\"Z21 Emulator\",\"deviceType\":\"OpenWRT\","
		     "\"request\":\"device_information_request\",\"buildNumber\":6076,\"apiVersion\":1},\"fileName\":\"MeineAnlage.z21\","
		     "\"request\":\"file_transfer_info\",\"fileSize\":%ld}\n", file_stat.st_size);
    printf("send TCP\n%s", offer);
    send(st, offer, strlen(offer), 0);

    buf_len = strlen(offer);
    memset(offer, 0, buf_len);

    printf("Waiting for <install>\n");
    if (recv(st, offer, buf_len, 0) < 0) {
	fprintf(stderr, "error receiveing answer install: %s\n", strerror(errno));
	return EXIT_FAILURE;
    }

    p = mmap(NULL, file_stat.st_size, PROT_READ, MAP_SHARED, fd, 0);

    if (strncmp(offer, "install", 7) == 0 ) {
        if (send(st, p, file_stat.st_size, 0) < 0) {
	    fprintf(stderr, "error sending Z21 data file: %s\n", strerror(errno));
	    return EXIT_FAILURE;
	} else {
	    if (close(st)) {
		fprintf(stderr, "error closing TCP stream: %s\n", strerror(errno));
		return EXIT_FAILURE;
	    }
	}
    }

    printf("data send complete\n");

    return EXIT_SUCCESS;
}

int send_udp_broadcast(void) {
    int n, s, sa, sb;
    struct sockaddr_in baddr, saddr;
    struct sockaddr_in client;
    struct timeval tv;
    char *timestamp;
    char buffer[64];
    socklen_t len;
    fd_set readfds;

    const int on = 1;

    int destination_port = Z21PORT;
    int local_port = Z21PORT;
    memset(&baddr, 0, sizeof(baddr));
    memset(&baddr, 0, sizeof(saddr));
    memset(&baddr, 0, sizeof(client));

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
    FD_SET(sa, &readfds);
    while (1) {
        if (select(sa + 1, &readfds, NULL, NULL, NULL) < 0) {
            fprintf(stderr, "select error: %s\n", strerror(errno));
        };

	if (FD_ISSET(sa, &readfds)) {
	    n = recvfrom(sa, udpframe, sizeof (udpframe), 0, &client, &len);
	    printf("received UDP packet len %u from %s\n", n, inet_ntop(AF_INET, &client.sin_addr, buffer, sizeof(buffer)));
	    if (n > 0) {
		udpframe[n+1] = 0;
		printf("%s\n", udpframe);
		/* only look for real IP adresse not 0.0.0.0 */
		if (ntohl(client.sin_addr.s_addr) != 0) {
		    send_tcp_data(&client);
		}
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
    char *z21_fstring;

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
	/* printf("%s\n", sql); */
	SQL_EXEC();
	for (n = 0; n < 32; n++) {
	    if (l->function[n].type) {
		if (l->function[n].type <= sizeof(fmapping)/sizeof(fmapping[0]))
		    z21_fstring = fmapping[l->function[n].type];
		else
		    z21_fstring = z21_fstring_none;

		asprintf(&sql, "INSERT INTO functions VALUES( %d, %d, %d, '', %d.0, %d, \"%s\", %d, %d, %d);",
				j, i, 0,   l->function[n].duration, n, z21_fstring, n, 1, 0);
		/* printf("%s\n", sql); */
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
