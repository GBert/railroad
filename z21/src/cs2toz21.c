/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 */

/*
 * CS2 lokomotive.cs2 to Z21 App (SQLite) converter
 */

/*
 * Server sends initial packet / UDP
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
 * Z21 App answer / UDP
 *
 * {"os":"android","appVersion":"1.4.6","deviceName":"Z21 Emulator","deviceType":"OpenWRT",
 * "request":"device_information_request","buildNumber":6076,"apiVersion":1}
 */

/*
 * Switch to TCP Connection on Port 5728 / Server is Z21 App
 * Data Source sends file
 *
 * {"owningDevice":{"os":"ios","appVersion":"1.4.6","deviceName":"iPad von Gerhard","deviceType":"iPad7,3",
 *  "request":"device_information_request","buildNumber":6076,"apiVersion":1},"fileName":"Data.z21",
 *  "request":"file_transfer_info","fileSize":744444}
 *
 * Z21 App sends:
 * install
 *
 * Data Source sends the file Data.z21 and closes TCP connection
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

#include "net.h"
#include "cs2_net.h"
#include "read-cs2-config.h"
#include "fmapping.h"
#include "geturl.h"
#include "utils.h"
#include "z21.h"

#include "Empty_Z21_sqlite.h"

#define MAXLINE         256
#define Z21PORT		5728

#define UUIDTEXTSIZE (sizeof(uuid_t) * 2) + 5
#define INTERFACE_LIST "wlan0,br-lan,br0"

char z21_fstring_none[] = "none";
char *timestamp;
struct z21_data_t z21_data;
struct z21_config_data_t config_data;
extern char rfc3986[256];

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
extern struct loco_data_t *loco_data;

void print_usage(char *prg) {
    fprintf(stderr, "\nUsage: %s -v -c <config_dir> -i <interface list> -s <config link> -p <icons link>\n", prg);
    fprintf(stderr, "   Version 0.992\n\n");
    fprintf(stderr, "         -a <time_out>       try to find CS2/CS2 for <time_out> seconds using -i <interface list>\n");
    fprintf(stderr, "         -c <config_dir>     set the config directory - default %s\n", config_data.config_dir);
    fprintf(stderr, "         -i <interface list> interface list - default %s\n", INTERFACE_LIST);
    fprintf(stderr, "         -s <link to config> link to the lokomotive.cs2\n");
    fprintf(stderr, "         -p <link to icons>  link to the icons server directory\n");
    fprintf(stderr, "         -v                  verbose\n\n");
}

int send_tcp_data(struct sockaddr_in *client_sa) {
    int n, st;
    size_t b;
    FILE *fp;
    struct sockaddr_in server_sa;
    char *offer;
    char *buffer;
    int32_t filesize;
    char filename[] = { "/tmp/Data.z21" };

    fp = fopen(filename, "rb");
    if (!fp) {
	fprintf(stderr, "can't open Z21 data %s: %s\n", filename, strerror(errno));
	return EXIT_FAILURE;
    }

    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    v_printf(config_data.verbose, "Filesize %d \n", filesize);

    /* prepare TCP client socket */
    if ((st = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	fprintf(stderr, "can't create TCP socket: %s\n", strerror(errno));
	fclose(fp);
	exit(EXIT_FAILURE);
    }

    memset(&server_sa, 0, sizeof server_sa);
    server_sa.sin_family = AF_INET;
    /* copy IP adresse from UDP request */
    memcpy(&server_sa.sin_addr.s_addr, &client_sa->sin_addr.s_addr, 4);
    server_sa.sin_port = htons(Z21PORT);

    if (connect(st, (struct sockaddr *)&server_sa, sizeof server_sa)) {
	fprintf(stderr, "can't connect to TCP socket : %s\n", strerror(errno));
	fclose(fp);
	return (EXIT_FAILURE);
    }
    asprintf(&offer, "{\"owningDevice\":{\"os\":\"android\",\"appVersion\":\"1.4.7\",\"deviceName\":\"Z21 Emulator\",\"deviceType\":\"OpenWRT\","
		     "\"request\":\"device_information_request\",\"buildNumber\":6076,\"apiVersion\":1},\"fileName\":\"Data.z21\","
		     "\"request\":\"file_transfer_info\",\"fileSize\":%d}\n", filesize);
    v_printf(config_data.verbose, "send TCP\n%s", offer);
    send(st, offer, strlen(offer), 0);
    free(offer);

    buffer = calloc(1024, 1);
    v_printf(config_data.verbose, "Waiting for <install> from client\n");
    n = recv(st, buffer, sizeof(buffer), 0);

    if (n < 0) {
	fprintf(stderr, "error receiveing answer install: %s\n", strerror(errno));
	free(buffer);
	fclose(fp);
	return EXIT_FAILURE;
    }

    v_printf(config_data.verbose, "received from Z21 App %d chars: >%s<\n", n, buffer);

    if (strncmp(buffer, "install", n) == 0) {
	while (!feof(fp)) {
	    b = fread(buffer, 1, sizeof buffer, fp);
	    if (send(st, buffer, b, 0) < 0) {
		fprintf(stderr, "error sending Z21 data file: %s\n", strerror(errno));
		fclose(fp);
		free(buffer);
		return EXIT_FAILURE;
	    }
	}
	if (shutdown(st, SHUT_RDWR)) {
	    fprintf(stderr, "error closing TCP stream: %s\n", strerror(errno));
	    fclose(fp);
	    free(buffer);
	    return EXIT_FAILURE;
	}
    }

    printf("data send complete\n");
    free(buffer);
    fclose(fp);
    return EXIT_SUCCESS;
}

int send_udp_broadcast(void) {
    int n, sa, sb;
    struct sockaddr_in baddr, saddr;
    struct sockaddr_in client;
    struct timeval tv;
    char buffer[64];
    socklen_t len;
    fd_set readfds;

    int destination_port = Z21PORT;
    int local_port = Z21PORT;
    memset(&baddr, 0, sizeof baddr);
    memset(&saddr, 0, sizeof saddr);
    memset(&client, 0, sizeof client);

    /* prepare UDP sending socket */
    sb = setup_udp_socket(&baddr, "255.255.255.255", destination_port, UDP_SENDING);
    if (sb <= 0) {
	fprintf(stderr, "problem to setup UDP (Z21 App) sending socket\n");
	exit(EXIT_FAILURE);
    }

    /* prepare receiving socket */
    sa = setup_udp_socket(&saddr, NULL, local_port, UDP_READING);
    if (sa <= 0) {
	fprintf(stderr, "problem to setup UDP (Z21 App) receiving socket\n");
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
	    n = recvfrom(sa, udpframe, sizeof udpframe, 0, (struct sockaddr *)&client, &len);
	    v_printf(config_data.verbose, "received UDP packet len %d from %s\n", n, inet_ntop(AF_INET, &client.sin_addr, buffer, sizeof buffer));
	    if (n > 0) {
		udpframe[n + 1] = 0;
		/* look for different packet than the sent packet */
		if (memcmp(timestamp, udpframe, strlen(timestamp)) != 0) {
		    v_printf(config_data.verbose, "%s\n", udpframe);
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

int sql_insert_locos(sqlite3 * db, struct z21_config_data_t *config_data, char *z21_dir, char *icon_dir, char *ip_s) {
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

    url_encoder_rfc_tables_init();

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
	asprintf(&loco_icon_d, "%s/%s", z21_dir, picture);

	/* do we get the icons from a webserver ? */
	if (config_data->icon_server) {
	    char *icon_s = calloc(strlen(l->icon) * 3, 1);
	    url_encode((unsigned char *)l->icon, icon_s);
	    asprintf(&loco_icon_s, "%s/%s.png", config_data->icon_server, icon_s);
	    get_url(loco_icon_s, NULL, loco_icon_d);
	    free(icon_s);
	} else {
	    asprintf(&loco_icon_s, "%s/%s.png", icon_dir, l->icon);
	    /* TODO: get picture false */
	    if (copy_file(loco_icon_s, loco_icon_d) == EXIT_FAILURE) {
		free(loco_icon_s);
		asprintf(&loco_icon_s, "%s/leeres Gleis.png", icon_dir);
		copy_file(loco_icon_s, loco_icon_d);
	    }
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
    FILE *fp;
    char *broadcast_ip, *config_file, *loco_file, *interface_list;
    int opt, ret;
    sqlite3 *db;
    char *z21_dir, *sql_file, *icon_dir, *systemcmd;
    uuid_t z21_uuid;
    char uuidtext[UUIDTEXTSIZE];

    memset(&config_data, 0, sizeof config_data);
    config_data.config_dir = strdup("/www");
    interface_list = strdup(INTERFACE_LIST);

    while ((opt = getopt(argc, argv, "a:c:i:p:s:vh?")) != -1) {
	switch (opt) {
	case 'a':
	    config_data.auto_timeout = atoi(optarg);
	    break;
	case 'c':
	    if (strnlen(optarg, MAXLINE) < MAXLINE) {
		free(config_data.config_dir);
		config_data.config_dir = strndup(optarg, MAXLINE - 1);
	    } else {
		fprintf(stderr, "config file dir to long\n");
		exit(EXIT_FAILURE);
	    }
	    break;
	case 'i':
	    if (strnlen(optarg, MAXLINE) < MAXLINE) {
		free(interface_list);
		interface_list = strndup(optarg, MAXLINE - 1);
	    } else {
		fprintf(stderr, "interface list to long\n");
		exit(EXIT_FAILURE);
	    }
	    break;
	case 'p':
	    if (strnlen(optarg, MAXLINE) < MAXLINE) {
		config_data.icon_server = strndup(optarg, MAXLINE - 1);
	    } else {
		fprintf(stderr, "server address to long\n");
		exit(EXIT_FAILURE);
	    }
	    break;
	case 's':
	    if (strnlen(optarg, MAXLINE) < MAXLINE) {
		config_data.config_server = strndup(optarg, MAXLINE - 1);
	    } else {
		fprintf(stderr, "server address to long\n");
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

    /* we try to find the CS2 IP */
    if (config_data.auto_timeout) {
	/* find the broadcast address */
	if (!(broadcast_ip = find_first_ip(interface_list, BROADCAST_IP))) {
	    fprintf(stderr, "can't find a valid broadcast IP on list: %s\n", interface_list);
	    exit(EXIT_FAILURE);
	}
	/* we have found the CS2 IP so let's use network config */
	config_data.cs2_ip = find_cs2(broadcast_ip, config_data.auto_timeout);
	if (config_data.cs2_ip.s_addr) {
	    config_data.ip_address = inet_ntoa(config_data.cs2_ip);
	    asprintf(&config_data.config_server, "http://%s/config/lokomotive.cs2", config_data.ip_address);
	    asprintf(&config_data.icon_server, "http://%s/icons", config_data.ip_address);
	    v_printf(config_data.verbose, "guessed CS2 IP address %s\n", config_data.ip_address);
	} else {
	    fprintf(stderr, "can't find CS2 on interfaces: %s\n", interface_list);
	    exit(EXIT_FAILURE);
	}
    }

    /* find the IP address for the database */
    if (!(config_data.ip_address = find_first_ip(interface_list, 0)))
	config_data.ip_address = strdup("127.0.0.1");

    if (config_data.config_server || config_data.icon_server)
	curl_global_init(0);

    /* try to read the lokomotive.cs via http ... */
    if (config_data.config_server) {
	v_printf(config_data.verbose, "using network for config file\n");
	config_file = get_url(config_data.config_server, NULL, NULL);
	if (config_file) {
	    read_loco_data(config_file, 0);
	} else {
	    fprintf(stderr, "can't read loco file: %s\n", strerror(errno));
	    exit(EXIT_FAILURE);
	}
	free(config_file);
    /* ... otherwise use file */
    } else {
	if (asprintf(&loco_file, "%s/config/%s", config_data.config_dir, loco_name) < 0) {
	    fprintf(stderr, "can't alloc buffer for loco_name: %s\n", strerror(errno));
	    exit(EXIT_FAILURE);
	}
	v_printf(config_data.verbose, "loco_file: >%s<\n", loco_file);
	read_loco_data(loco_file, CONFIG_FILE);
    }

    if (!HASH_COUNT(loco_data)) {
	fprintf(stderr, "no locos found !\n");
	exit(EXIT_FAILURE);
    }
    v_printf(config_data.verbose, "\nlocos in CS2 File: %u\n", HASH_COUNT(loco_data));

    /* preparing export directory */
    uuid_generate(z21_uuid);
    uuid_unparse_upper(z21_uuid, uuidtext);
    z21_dir = create_directory("/tmp", uuidtext);
    if (!z21_dir) {
	fprintf(stderr, "problems creating export directory\n");
	return EXIT_FAILURE;
    }

    /* open empty SQLite database */

    v_printf(config_data.verbose, "storing in %s\n", z21_dir);
    asprintf(&sql_file, "%s/Loco.sqlite", z21_dir);

    /* copy empty Z21 SQLite3 Database */
    fp = fopen(sql_file, "w");
    if (!fp) {
	fprintf(stderr, "Cannot open empty Z21 database file for writing: %s\n", sql_file);
	return EXIT_FAILURE;
    }
    if (fwrite(Empty_Z21_sqlite, 1, sizeof Empty_Z21_sqlite, fp) != sizeof Empty_Z21_sqlite) {
	fprintf(stderr, "Cannot write Z21 empty database file: %s\n", sql_file);
	return EXIT_FAILURE;
    }
    fclose(fp);

    /* open the database now */
    ret = sqlite3_open(sql_file, &db);
    if (ret != SQLITE_OK) {
	fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
	sqlite3_close(db);

	return EXIT_FAILURE;
    }
    sql_update_history(db);
    asprintf(&icon_dir, "%s/icons", config_data.config_dir);
    sql_insert_locos(db, &config_data, z21_dir, icon_dir, config_data.ip_address);

    sqlite3_close(db);
    free(sql_file);
    if (config_data.config_server || config_data.icon_server)
	curl_global_cleanup();
    free(icon_dir);
    free(config_data.config_dir);

    /* create zip file and delete directory */
    asprintf(&systemcmd, "cd /tmp; minizip -i -o Data.z21 export/%s/* 2>&1 > /dev/null", uuidtext);
    v_printf(config_data.verbose, "Zipping %s\n", systemcmd);
    system(systemcmd);
    free(systemcmd);
    asprintf(&systemcmd, "rm -rf /tmp/export/%s", uuidtext);
    system(systemcmd);
    free(systemcmd);
    send_udp_broadcast();
    return EXIT_SUCCESS;
}
