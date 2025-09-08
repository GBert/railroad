// ddl_maerklin.c - adapted for basrcpd project 2018 - 2024 by Rainer Müller
//
/* +----------------------------------------------------------------------+ */
/* | DDL - Digital Direct for Linux                                       | */
/* +----------------------------------------------------------------------+ */
/* | Copyright (c) 2002 - 2003 Vogt IT                                    | */
/* +----------------------------------------------------------------------+ */
/* | This source file is subject of the GNU general public license 2,     | */
/* | that is bundled with this package in the file COPYING, and is        | */
/* | available at through the world-wide-web at                           | */
/* | http://www.gnu.org/licenses/gpl.txt                                  | */
/* | If you did not receive a copy of the PHP license and are unable to   | */
/* | obtain it through the world-wide-web, please send a note to          | */
/* | gpl-license@vogt-it.com so we can mail you a copy immediately.       | */
/* +----------------------------------------------------------------------+ */
/* | Authors:   Torsten Vogt vogt@vogt-it.com                             | */
/* |                                                                      | */
/* +----------------------------------------------------------------------+ */

/***************************************************************/
/* erddcd - Electric Railroad Direct Digital Command Daemon    */
/*    generates without any other hardware digital commands    */
/*    to control electric model railroads                      */
/*                                                             */
/* file: maerklin.c                                            */
/* job : implements routines to compute data for the           */
/*       various Maerklin protocols and send this data to      */
/*       the serial device.                                    */
/*                                                             */
/* Torsten Vogt, January 1999                                  */
/*                                                             */
/* last changes: June 2000                                     */
/*               January 2001                                  */
/*                                                             */
/*
   thanks to Dieter Schaefer for testing and correcting the
   handling of the solenoids decoders (thr_protocol_maerklin_ms())
*/
/***************************************************************/

/**********************************************************************

 implemented protocols:

 M1: maerklin protocol type 1 (old) with 14 speed steps
 M2: maerklin protocol type 2 (new) with 14 speed steps
     maerklin protocol type 2 (new) with 28 speed steps (Wikinger decoder)
     maerklin protocol type 2 (new) with 27 speed steps (newer Maerklin decoders)
 MS: maerklin protocol for solenoids (type 1 and type 2)
 MF: maerklin_protocol for function decoders (old)

**********************************************************************/

#include "ddl.h"
#include "ddl_maerklin.h"
#include "srcp-gl.h"
#include "syslogmessage.h"

// Codiertabelle zur Adresswandlung binär -> ternär
static uint8_t mmadr_cod [256] = {
     85,  3,  1, 12, 15, 13,  4,  7,  5, 48, 51, 49, 60, 63, 61, 52,
     55, 53, 16, 19, 17, 28, 31, 29, 20, 23, 21,192,195,193,204,207,
    205,196,199,197,240,243,241,252,255,253,244,247,245,208,211,209,
    220,223,221,212,215,213, 64, 67, 65, 76, 79, 77, 68, 71, 69,112,
    115,113,124,127,125,116,119,117, 80, 83, 81, 92, 95, 93, 84, 87,
      0,  2,  6,233, 14, 18, 22, 26, 30, 34, 38, 42, 46, 50, 54, 58,
     62, 66, 70, 74, 78, 82, 86, 90, 94, 98,102,106,110,114,118,122,
    126,130,134,138,142,146,150,154,158,162,166,249,174,178,182,186,
    190,194,198,202,206,210,214,218,222,226,230,234,238,242,246,250,
    254,  8, 24, 40, 56, 72, 88,104,120,136,152,168,184,200,216,232,
    248, 11, 27, 43, 59, 75, 91,107,123,139,155,171,187,203,219,235,
    251,  9, 25, 41, 57, 73, 89,105,121,137,153,169,185,201,217, 10,
    170, 32, 96,160,224, 35, 99,163,227, 33, 97,161,225, 44,108,172,
    236, 47,111,175,239, 45,109,173,237, 36,100,164,228, 39,103,167,
    231, 37,101,165,229,128,131,129,140,143,141,132,135,133,176,179,
    177,188,191,189,180,183,181,144,147,145,156,159,157,148,151,149 };


typedef struct _tMaerklinData {
	uint8_t	addr;	uint8_t	func;	uint8_t	data;	uint8_t	xdat;
} tMaerklinData;

/* speed coding for 27 and 28 step handling */
    /* xFS    rFS   sFS1 -> (50ms) -> sFS2			FS28

       0       0      0                  %			 0
	CHG_DIR    1      1                  1			 1
       2       2      0                  2			 2
       3     2.5      3                  2			 2
       4       3      2                  3
       5     3.5      4                  3
       6       4      3                  4
       7     4.5      5                  4
       8       5      4                  5
       9     5.5      6                  5
       10      6      5                  6
       11    6.5      7                  6
       12      7      6                  7
       13    7.5      8                  7
       14      8      7                  8
       15    8.5      9                  8
       16      9      8                  9
       17    9.5     10                  9
       18     10      9                 10
       19   10.5     11                 10
       20     11     10                 11
       21   11.5     12                 11
       22     12     11                 12
       23   12.5     13                 12
       24     13     12                 13
       25   13.5     14                 13
       26     14     13                 14
       27   14.5     15                 14
       28     15     14                 15			15
       29   ----     --                 --			15
     */

void comp_MM(bus_t bus, gl_data_t *glp, int direction, int speed)
{
	tMaerklinData mdata;

	uint16_t addr = glp->id;
	if (addr > 255 || speed < 0 || speed > (glp->n_fs + 1)) return;

	mdata.addr = mmadr_cod[addr];
	mdata.func = (glp->funcs & 1) ? 3 : 0;
	mdata.data = speed;

	if (glp->protocolversion == 1) {	// MM1 coding
		if (addr > 80) return;
		mdata.xdat = speed;
		char *packet = (char *)&mdata;
		update_MaerklinPacketPool(bus, glp, packet, packet, packet, packet, packet);
		send_packet(bus, packet, QM1LOCOPKT, 2);

		if (glp->n_func > 1) {			// MF-Decoder
    		syslog_bus(bus, DBG_DEBUG,
               "Command for func decoder (Maerklin) (MF) addr %d received", addr);
			mdata.func = 3;
			mdata.xdat = mdata.data = (glp->funcs >> 1) & 0xF;
			send_packet(bus, (char *)&mdata, QM1FUNCPKT, 2);
		}
	}
	else {
		int sFS1, sFS2 = 0;
		switch (glp->n_fs) {
		case 14:	break;							// no special handling
		case 27:	if (speed > 1) {
						sFS2 = (speed / 2) + 1;
						if (speed & 1) {
							sFS1 = sFS2 + 1;
						}
						else {
							sFS1 = sFS2 - 1;
							if (sFS1 == 1) sFS1 = 0;
						}
						mdata.data = sFS1;			// first speed value
					}
					if (sFS2) {
						if (mdata.data & 8) mdata.xdat = (direction) ? 2 : 5;
			  			else mdata.xdat = (direction) ? 10 : 13;
						send_packet(bus, (char *)&mdata, QM2LOCOPKT, 3);
						mdata.data = sFS2;			// use second speed value
					}
					break;
		case 28:	if (speed > 1) {
						if (speed & 1) mdata.func ^= 2;	// halfstep bit
						mdata.data = (speed / 2) + 1;
					}
					break;
		default:	return;							// invalid value
		}

		char packets[5][4];
		for (int f = 0; f < 5; f++) {
			switch (f) {				// MM2 coding of direction and functions
			  case 0:	if (mdata.data & 8) mdata.xdat = (direction) ? 2 : 5;
			  			else mdata.xdat = (direction) ? 10 : 13;
						break;
			  case 1:	mdata.xdat = (glp->funcs & 2) ? 11 : 3;		break;
			  case 2:	mdata.xdat = (glp->funcs & 4) ? 12 : 4;		break;
			  case 3:	mdata.xdat = (glp->funcs & 8) ? 14 : 6;		break;
			  case 4:	mdata.xdat = (glp->funcs & 16) ? 15 : 7;	break;
			}
			if (mdata.xdat == mdata.data) {
        		mdata.xdat = (mdata.data & 8) ? 10 : 5;
			}
			memcpy(packets[f], &mdata, 4);
		}
		update_MaerklinPacketPool(bus, glp, packets[0],
								packets[1], packets[2], packets[3], packets[4]);
		send_packet(bus, packets[0], QM2LOCOPKT, 2);

		for (int i=1; i < 5; i++) {
			if ((glp->funcschange >> i) & 1) {
				send_packet(bus, packets[i], QM2FXPKT, 1);
				return;
			}
		}
	}
}

int comp_maerklin_ms(bus_t bus, int address, int port, int action)
{
    int id, subid;
	tMaerklinData mdata;

    syslog_bus(bus, DBG_DEBUG,
               "command for MM solenoid received addr:%d port:%d action:%d",
			   address, port, action);

    if (address < 1 || address > 324 || action < 0 || action > 1 || port < 0 || port > 1) {
        return 1;
    }
    id = ((address - 1) >> 2);
    if (id == 0) id = 80;		// special translation
    subid = (((address - 1) & 3) << 1) + port;

	mdata.addr = mmadr_cod[id];
	mdata.func = 0;
	mdata.xdat = mdata.data = subid + ((action) ? 8 : 0);
	send_packet(bus, (char *)&mdata, QM1SOLEPKT, 2);
	return 0;
}

void comp_maerklin_loco(bus_t bus, gl_data_t *glp)
{
	int pv = glp->protocolversion;
    int addr = glp->id;
    int speed = glp->speed;
    int direction = glp->direction;

    if (glp->speedchange & SCEMERG) {   // Emergency Stop
        speed = 0;
        direction = glp->cacheddirection;
        glp->speedchange &= ~SCEMERG;
    }
    else if (glp->speedchange & SCDIREC) {
        speed = 1;                      // change direction
        glp->speedchange &= ~SCDIREC;
    }
    else if (speed) speed++;        	// Never send FS1

    syslog_bus(bus, DBG_DEBUG,
    	"command for M%d protocol received addr:%d dir:%d (%d) speed:%d of %d chg %d funcs:%x",
        pv, addr, direction, glp->cacheddirection, speed, glp->n_fs, glp->speedchange, glp->funcs);

	comp_MM(bus, glp, direction, speed);

	glp->speedchange &= ~SCSPEED;       // handled now
	glp->funcschange = 0;
}

static void progstep(bus_t bus, int addr, int npre, int nact, int npast)
{
	tMaerklinData mdata;

	mdata.addr = mmadr_cod[addr];
	mdata.func = 0;
	if (npre) {
		mdata.xdat = mdata.data = 0;
		send_packet(bus, (char *)&mdata, QM1LOCOPKT, npre);
	}
	mdata.xdat = mdata.data = 1;
	send_packet(bus, (char *)&mdata, QM1LOCOPKT, nact);

	mdata.xdat = mdata.data = 0;
	send_packet(bus, (char *)&mdata, QM1LOCOPKT, npast);
}

int mm_reg_prog(bus_t bus, bool ubmode, int addr, int regnr, int regval)
{
	syslog_bus(bus, DBG_DEBUG,
			"command for MM programming received addr:%d register:%d value:%d",
			addr, regnr, regval);

	if (addr <= 0) addr = 80;
	if (addr > 255 || regnr < 0 || regnr > 255 || regval < 0 || regval > 255) {
		return 1;
	}
	progstep(bus, addr, 0, ubmode ? 400 : 100, 20);
	progstep(bus, regnr, 10, 60, 50);
	progstep(bus, regval, 10, 60, 50);
	return 0;
}

int mm_search_loco(bus_t bus, int proto)
{
	int t = sleep(19);			// HACK: simulate action duration (<20s)
	syslog_bus(bus, DBG_DEBUG, "*** MM-SEARCH not yet implemented, t=%d", t);
	return proto;
}

