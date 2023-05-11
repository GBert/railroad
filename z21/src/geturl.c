/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 */

#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "utils.h"


extern char *rfc3986;
extern char *html5;

struct memory_struct {
    char *memory;
    size_t size;
};

static size_t cb_write_url_data(void *ptr, size_t size, size_t nmemb, void *stream) {
    // printf("write to file/pipe\n");
    size_t written = fwrite(ptr, size, nmemb, (FILE *) stream);
    return written;
}

static size_t cb_write_url_mem(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct memory_struct *mem = (struct memory_struct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
	/* out of memory! */
	fprintf(stderr, "%s: not enough memory (realloc returned NULL)\n", __func__);
	return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

char *get_url(char *url, char *a_url, char *filename) {
    CURL *curl_handle;
    CURLcode ret = 0;

    printf("getting url %s\n", url);

    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    // curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, 1L);

    if (filename) {
	FILE *pagefile;
	pagefile = fopen(filename, "wb");
	if (pagefile) {
	    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, cb_write_url_data);
	    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, pagefile);
	    ret = curl_easy_perform(curl_handle);
	    if (ret == CURLE_HTTP_RETURNED_ERROR) {
		printf("curl problem %d\n", ret);
		curl_easy_setopt(curl_handle, CURLOPT_URL, a_url);
		ret = curl_easy_perform(curl_handle);
		if (ret == CURLE_HTTP_RETURNED_ERROR)
		    fprintf(stderr, "%s: error getting alternate %s\n", __func__, a_url);
	    } else {
		printf("curl fine %d\n", ret);
	    }
	    fclose(pagefile);
	} else {
	    fprintf(stderr, "%s: error opening file %s for writing\n", __func__, filename);
	}
	/* cleanup curl stuff */
	curl_easy_cleanup(curl_handle);
	return NULL;
    }

    struct memory_struct chunk;
    chunk.memory = malloc(1);	/* will be grown as needed by the realloc above */
    chunk.size = 0;		/* no data at this point */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, cb_write_url_mem);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    ret = curl_easy_perform(curl_handle);
    /* cleanup curl stuff */
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
    return chunk.memory;
}
