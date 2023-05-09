/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 */

/*
 * CS2 lokomotive.cs2 to Z21 SQLite converter
 */

/*
 * Server sends initial packet
 *
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
 * Client answer
 *
 * {"os":"android","appVersion":"1.4.6","deviceName":"Z21 Emulator","deviceType":"OpenWRT",
 * "request":"device_information_request","buildNumber":6076,"apiVersion":1}
 */

/*
 * Server sends file
 *
 * {"owningDevice":{"os":"ios","appVersion":"1.4.6","deviceName":"iPad von Gerhard","deviceType":"iPad7,3",
 *  "request":"device_information_request","buildNumber":6076,"apiVersion":1},"fileName":"Data.z21",
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
#include <uuid/uuid.h>

#include "read-cs2-config.h"
#include "fmapping.h"
#include "geturl.h"

#define MAXDG		4096	/* maximum datagram size */
#define MAXLINE         256
#define Z21PORT		5728

#define UUIDTEXTSIZE (sizeof(uuid_t) * 2) + 5

char z21_fstring_none[] = "none";
char *timestamp;

#define SQL_EXEC(SQL) do { \
ret = sqlite3_exec(db, (SQL), 0, 0, &err_msg); \
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
    fprintf(stderr, "   Version 0.9\n\n");
    fprintf(stderr, "         -c <config_dir>     set the config directory - default %s\n", config_dir);
    fprintf(stderr, "         -v                  verbose\n\n");
}

uint16_t loco_address_mapping(uint16_t uid) {
    /* dcc */
    if (uid > 0xc000)
	return (uid - 0xc000 + 5000);
    /* mfx */
    if (uid > 0x4000)
	return (uid - 0x4000 + 1000);
    return (uid);
}

uint16_t loco_address_demapping(uint16_t z21app_address) {
    /* dcc */
    if (z21app_address > 5000)
	return (z21app_address - 5000 + 0xc000);
    /* mfx */
    if (z21app_address > 1000)
	return (z21app_address - 1000 + 0x4000);
    return (z21app_address);
}

int send_tcp_data(struct sockaddr_in *client_sa) {
    int buf_len, fd, st;
    struct sockaddr_in server_sa;
    char *offer;
    void *p;
    struct stat file_stat;
    char filename[] = { "/tmp/Data.z21" };

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
	fprintf(stderr, "can't open Z21 data %s: %s\n", filename, strerror(errno));
	return EXIT_FAILURE;
    }

    if (fstat(fd, &file_stat) < 0) {
	fprintf(stderr, "can't get Z21 data %s size: %s\n", filename, strerror(errno));
	close(fd);
	return EXIT_FAILURE;
    }

    printf("Filesize %ld ", file_stat.st_size);

    /* prepare TCP client socket */
    if ((st = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	fprintf(stderr, "can't create TCP socket: %s\n", strerror(errno));
	close(fd);
	exit(EXIT_FAILURE);
    }

    memset(&server_sa, 0, sizeof server_sa);
    server_sa.sin_family = AF_INET;
    /* copy IP adresse from UDP request */
    memcpy(&server_sa.sin_addr.s_addr, &client_sa->sin_addr.s_addr, 4);
    server_sa.sin_port = htons(Z21PORT);

    if (connect(st, (struct sockaddr *)&server_sa, sizeof server_sa)) {
	fprintf(stderr, "can't connect to TCP socket : %s\n", strerror(errno));
	close(fd);
	return (EXIT_FAILURE);
    }
    asprintf(&offer, "{\"owningDevice\":{\"os\":\"android\",\"appVersion\":\"1.4.7\",\"deviceName\":\"Z21 Emulator\",\"deviceType\":\"OpenWRT\","
		     "\"request\":\"device_information_request\",\"buildNumber\":6076,\"apiVersion\":1},\"fileName\":\"Data.z21\","
		     "\"request\":\"file_transfer_info\",\"fileSize\":%ld}\n", file_stat.st_size);
    printf("send TCP\n%s", offer);
    send(st, offer, strlen(offer), 0);

    buf_len = strlen(offer);
    memset(offer, 0, buf_len);

    printf("Waiting for <install>\n");
    if (recv(st, offer, buf_len, 0) < 0) {
	fprintf(stderr, "error receiveing answer install: %s\n", strerror(errno));
	close(fd);
	return EXIT_FAILURE;
    }

    p = mmap(NULL, file_stat.st_size, PROT_READ, MAP_SHARED, fd, 0);

    if (strncmp(offer, "install", 7) == 0) {
	if (send(st, p, file_stat.st_size, 0) < 0) {
	    fprintf(stderr, "error sending Z21 data file: %s\n", strerror(errno));
	    close(fd);
	    return EXIT_FAILURE;
	} else {
	    if (close(st)) {
		fprintf(stderr, "error closing TCP stream: %s\n", strerror(errno));
		close(fd);
		return EXIT_FAILURE;
	    }
	}
    }

    printf("data send complete\n");

    close(fd);
    return EXIT_SUCCESS;
}

int send_udp_broadcast(void) {
    int n, s, sa, sb;
    struct sockaddr_in baddr, saddr;
    struct sockaddr_in client;
    struct timeval tv;
    char buffer[64];
    socklen_t len;
    fd_set readfds;

    const int on = 1;

    int destination_port = Z21PORT;
    int local_port = Z21PORT;
    memset(&baddr, 0, sizeof baddr);
    memset(&saddr, 0, sizeof saddr);
    memset(&client, 0, sizeof client);

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
    if (setsockopt(sb, SOL_SOCKET, SO_BROADCAST, &on, sizeof on) < 0) {
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

    if (bind(sa, (struct sockaddr *)&saddr, sizeof saddr) < 0) {
	fprintf(stderr, "UDP bind error: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }

    /* get timestamp */
    gettimeofday(&tv, NULL);

    unsigned long long millisecondsSinceEpoch =
	    (unsigned long long)(tv.tv_sec) * 1000 +
	    (unsigned long long)(tv.tv_usec) / 1000;

    asprintf(&timestamp, "%llu", millisecondsSinceEpoch);

    if (sendto(sb, timestamp, strlen(timestamp), 0, (struct sockaddr *)&baddr, sizeof baddr) != strlen(timestamp))
	fprintf(stderr, "UDP write error: %s\n", strerror(errno));

    FD_ZERO(&readfds);
    FD_SET(sa, &readfds);
    while (1) {
	if (select(sa + 1, &readfds, NULL, NULL, NULL) < 0) {
	    fprintf(stderr, "select error: %s\n", strerror(errno));
	};

	if (FD_ISSET(sa, &readfds)) {
	    len = sizeof client;
	    n = recvfrom(sa, udpframe, sizeof udpframe, 0, &client, &len);
	    printf("received UDP packet len %d from %s\n", n, inet_ntop(AF_INET, &client.sin_addr, buffer, sizeof buffer));
	    if (n > 0) {
		udpframe[n + 1] = 0;
		printf("%s\n", udpframe);
		/* only look for real IP adresse not 0.0.0.0 */
		if (memcmp(timestamp, udpframe, strlen(timestamp) != 0)) {
		    send_tcp_data(&client);
		}
	    }
	}
    }
    return EXIT_SUCCESS;
}

char *create_directory(char *basedir, char *uuidtext) {
    struct stat st;
    char *dir;

    memset(&st, 0, sizeof st);
    if (stat(basedir, &st)) {
	fprintf(stderr, "%s: basedir \'%s\' doesn't exist\n", __func__, basedir);
	return NULL;
    }

    memset(&st, 0, sizeof st);
    asprintf(&dir, "%s/export", basedir);
    if (stat(dir, &st) == -1) {
	if (mkdir(dir, 0775)) {
	    fprintf(stderr, "%s: can't create %s\n", __func__, dir);
	    return NULL;
	}
    }
    free(dir);

    asprintf(&dir, "%s/export/%s", basedir, uuidtext);
    if (mkdir(dir, 0775)) {
	fprintf(stderr, "%s: can't create %s\n", __func__, dir);
	return NULL;
    }
    return dir;
}

int copy_file(char *src, char *dst) {
    char c[4096];
    FILE *fd_in, *fd_out;

    // printf("copy file %s -> %s\n", src, dst);

    fd_in = fopen(src, "r");
    if (!fd_in) {
	fprintf(stderr, "%s: can't fopen %s\n", __func__, src);
	return EXIT_FAILURE;
    }

    fd_out = fopen(dst, "w");
    if (!fd_out) {
	fprintf(stderr, "%s: can't fopen %s\n", __func__, dst);
	fclose(fd_in);
	return EXIT_FAILURE;
    }

    while (!feof(fd_in)) {
	size_t bytes = fread(c, 1, sizeof c, fd_in);
	if (bytes) {
	    fwrite(c, 1, bytes, fd_out);
	}
    }

    fclose(fd_in);
    fclose(fd_out);
    return EXIT_SUCCESS;
}

int sql_update_history(sqlite3 * db) {
    int ret;
    char *err_msg;
    char *time_st;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    char *sql = "DELETE FROM update_history;";
    SQL_EXEC(sql);
    asprintf(&time_st, "INSERT INTO update_history VALUES(1, \'ios\', \'%02d.%02d.%02d, %02d:%02d:%02d Mitteleuropäische Normalzeit\', \'1.4.7\', 1000, 100);",
		    tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
    SQL_EXEC(time_st);
    free(time_st);
    sql = "PRAGMA user_version = 15";
    SQL_EXEC(sql);
    return EXIT_SUCCESS;
}

int sql_insert_locos(sqlite3 * db, char *z21_dir, char *icon_dir, char *ip_s) {
    char *err_msg;
    int button, i, j, n, ret;
    struct loco_data_t *l;
    char *sql;
    char *z21_fstring;
    uuid_t uuid;
    char uuidtext[UUIDTEXTSIZE];
    char *picture;
    char *loco_icon_s, *loco_icon_d;
    uint16_t loco_address;

    i = 1;
    j = 1;

    /* delete existing data
       sql = "DELETE FROM vehicles;";
       SQL_EXEC(sql);
       sql = "DELETE FROM functions;";
       SQL_EXEC(sql);
     */

    for (l = loco_data; l != NULL; l = l->hh.next) {
	uuid_generate(uuid);
	uuid_unparse_upper(uuid, uuidtext);
	asprintf(&picture, "%s.png", uuidtext);

	// printf("Loco picture: %s.png\n", l->icon);
	asprintf(&loco_icon_s, "%s/%s.png", icon_dir, l->icon);
	asprintf(&loco_icon_d, "%s/%s", z21_dir, picture);
	/* TODO: get picture false */
	if (copy_file(loco_icon_s, loco_icon_d) == EXIT_FAILURE) {
	    free(loco_icon_s);
	    asprintf(&loco_icon_s, "%s/leeres Gleis.png", icon_dir);
	    copy_file(loco_icon_s, loco_icon_d);
	}

	loco_address = loco_address_mapping(l->uid);
	asprintf(&sql, "INSERT INTO vehicles VALUES(%d, '%s', '%s', 0, %d, %d, 1, %d, '', '', 0, '', '', '', '', '', '', '', '', '', '', '', "
		       "0, '', 0, '%s', 0, 0, 0, 786, 0, 0, 1024, '', 0, 0);", i, l->name, picture, l->tmax, loco_address, i - 1, ip_s);
	/* printf("%s\n", sql); */
	SQL_EXEC(sql);
	free(sql);
	free(loco_icon_s);
	free(loco_icon_d);
	free(picture);
	for (n = 0; n < 32; n++) {
	    if (l->function[n].type) {
		z21_fstring = (l->function[n].type <= sizeof fmapping / sizeof fmapping[0] - 1) ? fmapping[l->function[n].type] : z21_fstring_none;
		button = l->function[n].duration ? 2 : 0;
		asprintf(&sql, "INSERT INTO functions VALUES( %d, %d, %d, '', %d.0, %d, \"%s\", %d, %d, %d);",
				j, i, button, l->function[n].duration, n, z21_fstring, n, 1, 0);
		/* printf("%s\n", sql); */
		SQL_EXEC(sql);
		free(sql);
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
    char *z21_dir, *sql_file, *icon_dir, *systemcmd;
    uuid_t z21_uuid;
    char uuidtext[UUIDTEXTSIZE];

    config_data.verbose = 1;

    while ((opt = getopt(argc, argv, "c:vh?")) != -1) {
	switch (opt) {
	case 'c':
	    if (strnlen(optarg, MAXLINE) < MAXLINE) {
		strncpy(config_dir, optarg, sizeof config_dir - 1);
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

    if (asprintf(&loco_file, "%s/config/%s", config_dir, loco_name) < 0) {
	fprintf(stderr, "can't alloc buffer for loco_name: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }
    printf("loco_file: >%s<\n", loco_file);

    read_loco_data(loco_file, CONFIG_FILE);
    if (config_data.verbose == 1)
	printf("locos in CS2 File: %u\n", HASH_COUNT(loco_data));

    /* preparing export directory */
    uuid_generate(z21_uuid);
    uuid_unparse_upper(z21_uuid, uuidtext);
    z21_dir = create_directory("/tmp", uuidtext);
    if (!z21_dir) {
	fprintf(stderr, "problems creating export directory\n");
	return EXIT_FAILURE;
    }

    /* open empty SQLite database */

    printf("storing in %s\n", z21_dir);

    asprintf(&sql_file, "%s/Loco.sqlite", z21_dir);

    ret = copy_file("Loco_empty.sqlite", sql_file);
    if (ret == EXIT_FAILURE) {
	fprintf(stderr, "Cannot copy file Loco_empty.sqlite to %s\n", sql_file);
	return EXIT_FAILURE;
    }

    ret = sqlite3_open(sql_file, &db);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
	sqlite3_close(db);

	return EXIT_FAILURE;
    }
    sql_update_history(db);
    asprintf(&icon_dir, "%s/icons", config_dir);
    sql_insert_locos(db, z21_dir, icon_dir, "192.168.0.9");
    free(icon_dir);
    sqlite3_close(db);
    free(sql_file);
    /* create zip file and delete directory */
    asprintf(&systemcmd, "cd /tmp; minizip -o Data.z21 export/%s/* 2>&1 > /dev/null", uuidtext);
    printf("Zipping %s\n", systemcmd);
    system(systemcmd);
    free(systemcmd);
    asprintf(&systemcmd, "rm -rf /tmp/export/%s", uuidtext);
    system(systemcmd);
    free(systemcmd);
    send_udp_broadcast();
    return EXIT_SUCCESS;
}
