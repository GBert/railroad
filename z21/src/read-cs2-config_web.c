#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "geturl.h"
#include "read-cs2-config.h"
#include "uthash.h"
#include "z21.h"

extern struct loco_data_t *loco_data, *loco_data_by_uid;
extern struct magnet_data_t *magnet_data;
extern struct subscriber_t *subscriber;
struct z21_config_data_t config_data;

int main() {
    char *ptr = NULL;

    ptr = get_url("http://192.168.0.30/config/lokomotive.cs2", NULL, NULL);
    if (ptr) {
	read_loco_data(ptr, 0);
	printf("locos in CS2 File: %u\n", HASH_COUNT(loco_data));
	free(ptr);
    }
    ptr = get_url("http://192.168.0.30/config/magnetartikel.cs2", NULL, NULL);
    if (ptr) {
	read_magnet_data(ptr, 0);
	printf("magnets in CS2 File: %u\n", HASH_COUNT(magnet_data));
	free(ptr);
    }
    return 0;
}
