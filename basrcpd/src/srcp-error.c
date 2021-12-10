// srcp-error.c - adapted for basrcpd project 2019 - 2021 by Rainer Müller

/* $Id: srcp-error.c 1456 2010-02-28 20:01:39Z gscholz $ */

/* 
 * Vorliegende Software unterliegt der General Public License, 
 * Version 2, 1991. (c) Matthias Trute, 2000-2002.
 *
 */

#include <stdio.h>

#include "srcp-error.h"


int srcp_fmt_msg(int errorcode, char *msg, struct timeval time)
{
	char * emsg;
    switch (errorcode) {
        case 100:
        case 101:
        case 102:
        case 110:
            emsg = "INFO";
            break;
        case 200:
            emsg = "OK";
            break;
        case 202:
            emsg = "OK CONNECTIONMODE";
            break;
        case 201:
            emsg = "OK PROTOCOL SRCP";
            break;
        case 400:
            emsg = "ERROR unsupported protocol";
            break;
        case 401:
            emsg = "ERROR unsupported connection mode";
            break;
        case 402:
            emsg = "ERROR insufficient data";
            break;
        case 410:
            emsg = "ERROR unknown command";
            break;
        case 411:
            emsg = "ERROR unknown value";
            break;
        case 412:
            emsg = "ERROR wrong value";
            break;
        case 413:
            emsg = "ERROR temporarily prohibited";
            break;
        case 414:
            emsg = "ERROR device locked";
            break;
        case 415:
            emsg = "ERROR forbidden";
            break;
        case 416:
            emsg = "ERROR no data";
            break;
        case 417:
            emsg = "ERROR timeout";
            break;
        case 418:
            emsg = "ERROR list too long";
            break;
        case 419:
            emsg = "ERROR list too short";
            break;
        case 420:
            emsg = "ERROR unsupported device protocol";
            break;
        case 421:
            emsg = "ERROR unsupported device";
            break;
        case 422:
            emsg = "ERROR unsupported device group";
            break;
        case 423:
            emsg = "ERROR unsupported operation";
            break;
        case 424:
            emsg = "ERROR device reinitialized";
            break;
        case 500:
            emsg = "ERROR out of resources";
            break;
        default:
            sprintf(msg, "%lld.%.3ld 600 ERROR internal error %d\n",
                    (long long) time.tv_sec,
					(long) (time.tv_usec / 1000), errorcode);
            return errorcode;
    }
    sprintf(msg, "%lld.%.3ld %d %s\n", (long long) time.tv_sec,
                (long) (time.tv_usec / 1000), errorcode, emsg);
    return errorcode;
}
