// platform.c : PI platform access
//
// C 2018 - 2021 Rainer Müller
// Das Programm unterliegt den Bedingungen der GNU General Public License 3 (GPL3).

#include <stdio.h>
#include <string.h>
#ifndef TEST
  #include "syslogmessage.h"
#endif
#include "platform.h"

#define BUF 255

// valid at least for BananaPi running with Armbian, CentOS, and OpenWRT
// and for RaspberryPi with Raspbian
#define PLSERIALFILE "/proc/cpuinfo"
#define PLSERIALKEY  "Serial"
#define PLTEMPERFILE "/sys/class/thermal/thermal_zone0/temp"

static uint64_t serial = 0;
static FILE *temperfile = NULL;

uint64_t getPlatformData(pl_function_t fucode)
{
   	char *found, buffer[BUF];
   	long int temper = 0;

	switch(fucode) {
		case PL_SERIAL:	if (serial == 0) {
							FILE *sfile = fopen(PLSERIALFILE, "r");
   							if(sfile == NULL) break;
   							while( fgets(buffer, BUF, sfile) != NULL ) {
								found = strstr(buffer, PLSERIALKEY);
								if (found) sscanf(found+strlen(PLSERIALKEY),
											" : %16llx",
											(long long unsigned int *) &serial);
							}
							fclose(sfile);
   						}
   						return serial;

		case PL_TEMP:   if (temperfile == NULL) {
							temperfile = fopen(PLTEMPERFILE, "r");
   							if(temperfile == NULL) break;
						}
						else rewind(temperfile);
						fread(buffer, 8, 1, temperfile);
                        sscanf(buffer, "%ld", &temper);
#ifdef REOPEN
                        fclose(temperfile);
                        temperfile = NULL;
#endif
						return temper;

		default:        break;
	}
#ifndef TEST
	syslog_bus(0, DBG_ERROR, "error when extracting platform data.");
#endif
	return 0L;
}

// -----------------------------------------------------------------------------
#ifdef TEST
// additional header files
#include <unistd.h>
#include <sys/time.h>

long long t_now = 0;
struct timeval tv = { 0, 0 };

uint64_t getPlatformTime(pl_function_t fucode)
{
	// Start Zeitmessung
	gettimeofday(&tv, NULL);
	t_now = tv.tv_sec * 1000000 + tv.tv_usec;

	uint64_t retval = getPlatformData(fucode);

	// Ende Zeitmessung und Ausgabe
	gettimeofday(&tv, NULL);
	t_now = tv.tv_sec * 1000000 + tv.tv_usec - t_now;
	printf("Ausführungsdauer %lldµs für Abfrage:\n", t_now);

	if (retval == 0) printf("Fehler aufgetreten... \n");
	return retval;
}

int main(int argc, char *argv[])
{
   	unsigned long long sernmbr;
	for (int i=0; i<10; i++) {
		if ((i & 7) == 0) {
			sernmbr = getPlatformTime(PL_SERIAL);
			printf("Serial : %llx\n", sernmbr);
			sernmbr %= 1000000L;
			printf("Serial : %llu\n", sernmbr);
		}
		printf("Temperatur : %lu\n", (unsigned long) getPlatformTime(PL_TEMP)/100);
		sleep(2);
    }
   	return 0;
}
#endif
