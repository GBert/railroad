// ddl_mfx.c - adapted for basrcpd project 2018 - 2024 by Rainer Müller

/* +----------------------------------------------------------------------+ */
/* | DDL - Digital Direct for Linux                                       | */
/* +----------------------------------------------------------------------+ */
/* | Copyright (c) 2016 Daniel Sigg                                       | */
/* +----------------------------------------------------------------------+ */
/* | This source file is subject of the GNU general public license 2,     | */
/* | that is bundled with this package in the file COPYING, and is        | */
/* | available at through the world-wide-web                              | */
/* +----------------------------------------------------------------------+ */
/* | Author:   Daniel Sigg daniel@siggsoftware.ch                         | */
/* |                                                                      | */
/* +----------------------------------------------------------------------+ */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "config.h"
#include "ddl_mfx.h"
#include "syslogmessage.h"
#include "ddl.h"
#include "srcp-gl.h"
#include "srcp-sm.h"
#include "srcp-info.h"
#include "srcp-error.h"

#ifndef SYSCONFDIR
	#define SYSCONFDIR "/etc"
#endif

//Zentrale UID, Dekoder Existenzabfrage und Dekodersuche Intervall (us)
//#define INTERVALL_UID 500000
//Anzahl Durchläufe "Zentrale UID" (INTERVALL_UID) nach der wieder eine Adresszuweisung wiederholt wird
//#define LOOP_UID_SID_REPEAT 10
//Pause nach MFX Read Befehlen(us)
#define PAUSE_MFX_READ 50000

//Max. MFX RDS Leseversuche
#define MAX_RDS_TRIALS 3

//Anzahl MFX Funktionen
#define MFX_FX_COUNT 16

//MFX Verwaltungsthread
static uint32_t decoderUId;
static uint16_t registrationCounter;
static int searchstep = 1;
static int autosearch;

//MFX RDS Feedback thread
static bus_t busnumber = 0;

static tMFXRead mfxread;

//Konfig Cache einer Lok, alle Konfigregister [CVNr10Bit][Index6Bit]
#define CV_SIZE 1024
#define CVINDEX_SIZE 64


/**
 * Servicemode (SM) aktiv oder nicht.
 * Wenn aktiv werden keine weiteren, automatischen, MFX RDS Abfragen mehr gesendet.
 * Grund: Bei den MFX RDS Rückmeldungen müsste sonst unterschieden werden, wohin sie müssen.
 */
static bool sm = false;


#if 0
// HACK: das brauchen wir wohl nicht
/**
 * MFX spezifische INIT Parameter als String.
 * @param uid
 * @param name
 * @param fx Array mit MFX_FX_COUNT Elementen zur Funktionsbeschreibung
 * @param msg Message, an die die Paramater gehängt werden sollen.
 *            Minimale Länge muss für alles ausreichend sein:
 *            Länge UID und fx max 10, Name 16+2 plus Spaces
 */
void getMFXInitParam(uint32_t uid, /*char *name, uint32_t *fx,*/ char *msg)
{
  sprintf(msg + strlen(msg), " %u", uid);
/*  sprintf(msg + strlen(msg), " %u \"%s\"", uid, name);
  int i;
  for (i = 0; i<MFX_FX_COUNT; i++) {
    sprintf(msg + strlen(msg), " %u", fx[i]);
  }	*/
}

/**
 * MFX spezifische INIT Parameter aus Lokdaten ermitteln.
 * @param gl Lok, zu der die MFX spezifischen INIT Paramater ermittelt werden sollen.
 * @param msg Message, an die die Paramater gehängt werden sollen.
 */
void describeGLmfx(gl_data_t *gl, char *msg)
{
  if (gl->protocol == 'X') {
//    getMFXInitParam(gl->decuid, /* gl->optData.mfx.name, gl->optData.mfx.fx,*/ msg);
  	sprintf(msg + strlen(msg), " %u", gl->decuid);
  }
}
#endif
/**
 * Fügt n Bits in einen Ausgabestream hinzu.
 * @param bits Bis zu 32 Bits die hinzugefügt werden sollen.
 *             MSB wird immer zuerst.
 * @param len Anzahl Bits die hinzugefügt werden soll (ab LSB Bit in "bits")
 * @param stream Ausgabestream, zu dem die Bits hibzugefügt werden sollen.
 *               Es wird mit Position 0 mit dem LSB in stream[1] begonnen.
 *               Muss mit 0 initialisiert sein!
 * @param pos Position, an der eingefügt werden soll. Wird um "len" inkrementiert.
 */
static void addBits(unsigned int bits, unsigned int len, char *stream, unsigned int *pos) {
  unsigned int i;
  unsigned int maske = 1U << (len - 1);
  for (i = 0; i<len; i++) {
    if (bits & maske) {
      stream[1 + (*pos / 8)] |= 1 << (*pos % 8);
    }
    else {
    }
    (*pos)++;
    maske >>= 1;
  }
}

/**
 * Fügt den MFX Adresse ein.
 * @param address Adresse die eingefügt werden soll
 * @param stream Strem in dem der CRC an "pos" eingefügt werden soll.
 *               CRC wird ab stream[1] LSB bis pos-1 berechnet.
 * @param pos Position, an der eingefügt werden soll. Wird um CRC Länge (8) inkrementiert.
 */
static void addAdrBits(int address, char *stream, unsigned int *pos) {
  if (address < 128) {
    addBits(0b10, 2, stream, pos); //Adr 7 Bit
    addBits(address, 7, stream, pos);
  }
  else if (address < 512) {
    addBits(0b110, 3, stream, pos); //Adr 9 Bit
    addBits(address, 9, stream, pos);
  }
  else if (address < 2048) {
    addBits(0b1110, 4, stream, pos); //Adr 11 Bit
    addBits(address, 11, stream, pos);
  }
  else {
    addBits(0b1111, 4, stream, pos); //Adr 14 Bit
    addBits(address, 14, stream, pos);
  }
}

/**
 * Fügt den MFX CRC ein.
 * Da der CRC am Schluss ist wird an [0] die total Anzahl Bits gesetzt
 * @param stream Strem in dem der CRC an "pos" eingefügt werden soll.
 *               CRC wird ab stream[1] LSB bis pos-1 berechnet.
 * @param pos Position, an der eingefügt werden soll. Wird um CRC Länge (8) inkrementiert.
 */
#define CRC_LEN 8
static void addCRCBits(char *stream, unsigned int *pos) {
  uint16_t crc = 0x007F; // nicht nur 8 Bit, um Carry zu erhalten
  unsigned int i;
  unsigned int count = *pos + CRC_LEN;
  for (i = 0; i < count; i++) {
    crc = (crc << 1) + ((stream[1 + (i / 8)] & (1 << (i % 8))) ? 1 : 0);
    if ((crc & 0x0100) > 0) {
      crc = (crc & 0x00FF) ^ 0x07;
    }
  }
  addBits(crc, CRC_LEN, stream, pos);
  stream[0] = *pos;
}

/**
 * MFX Paket versenden.
 * @param address Lokadresse oder 0 (Broadcast)
 * @param stream zu versendendes MFX Paket. an [0] muss die Anzahl Bist stehen, die ab [1] LSB folgen.
 * @param packetTyp MFX Pakettyp (QMFX?PKT)
 * @return Anzahl Bytes aus Stream, die gesendet wurden.
 */
static unsigned int sendMFXPaket(int address, char *stream, int packetTyp, int xmits)
{
	unsigned int sizeStream = ((stream[0] + 7) / 8) + 1; //+7 damit immer bei int Division aufgerundet wird
	send_packet(busnumber, stream, packetTyp, xmits);
	return sizeStream;
}

/**
 * Senden eines MFX Pakets für Fx 16-31
 * @param address Lokadresse
 * @param func Funktionsnummer, könnte 0 bis 127 sein, hier sinnvoll ist aber nur 16-31.
 * @param value Zustand der funktion ein/aus
 */
void send_LocoFx16_32(int address, int func, bool value, int xmits)
{
	char packetstream[PKTSIZE];
	unsigned int pos = 0;
	memset(packetstream, 0, PKTSIZE);
	
	addAdrBits(address, packetstream, &pos);
	addBits(0b100, 3, packetstream, &pos); 		// switch one function
	addBits(func, 7, packetstream, &pos);		// number of function
	addBits(0, 1, packetstream, &pos);
	addBits(value ? 1 : 0, 1, packetstream, &pos); // new state
	addCRCBits(packetstream, &pos);
	sendMFXPaket(address, packetstream, QMFX0PKT, xmits);
}

/**
  Generate the packet for MFX-decoder with 128 speed steps and up to 16 functions
  @param pointer to GL data
*/
void comp_mfx_loco(bus_t bus, gl_data_t *glp)
{
	char packetstream[PKTSIZE] = { 0 };

    int address = glp->id;
    int speed = glp->speed;
    int direction = glp->direction;
	uint32_t funcs = glp->funcs;
    uint8_t nspeed = glp->n_fs;
    uint8_t	nfuncs = glp->n_func;

    if (glp->speedchange & SCEMERG) {   // Emergency Stop
        speed = 1;
        direction = glp->cacheddirection;
        glp->speedchange &= ~SCEMERG;
    }
    else if (speed) speed++;        	// Never send FS1

    if (direction == -1) direction = 0; // mfx loco TERM
  	if (speed > 127) speed = 127;
    glp->speedchange &= ~(SCSPEED | SCDIREC);   // handled now

	syslog_bus(bus, DBG_DEBUG,
             "command for MFX protocol received addr:%d "
             "dir:%d speed:%d nspeeds:%d nfunc:%d funcs %x",
             address, direction, speed, nspeed, nfuncs, funcs);

  //packetstream Format:
  //1. Byte länge
  //Ab 2. Byte Low Bit: jedes Bit bis Anzahl Bits Länge

  //Format des Bitstreams für Fahrstufe ist (9 Bit Adresse, 127 Fahrstufen):
  //<=4 Fn       : 110AAAAAAAAA001RSSSSSSS010FFFFCCCCCCCC
  //>4 .. <=8 Fn : 110AAAAAAAAA001RSSSSSSS0110FFFFFFFFCCCCCCCC
  //>8 Fn        : 110AAAAAAAAA001RSSSSSSS0111FFFFFFFFFFFFFFFFCCCCCCCC
  //if (speed mod 16)==0 :     000RSSS.....
  //A=Adresse, für Adressen <128 wird die 7 Bit Adressierung verwendet
  //R=Richtung
  //S=Fahrstufe
  //F=F0 bis F15
  //C=Checksumme

	unsigned int pos = 0;
  	addAdrBits(address, packetstream, &pos);
  	// prefer short speed format
	if ((speed & 15) == 0) {
		addBits(0b000, 3, packetstream, &pos);	// drive using 3 Bits
    	addBits(~direction, 1, packetstream, &pos);
    	addBits(speed >> 4, 3, packetstream, &pos);
  	}
  	else {
    	addBits(0b001, 3, packetstream, &pos); 	// drive using 7 Bits
    	addBits(~direction, 1, packetstream, &pos);
    	addBits(speed, 7, packetstream, &pos);
  	}
	if (nfuncs <=4) {
		addBits(0b010, 3, packetstream, &pos);	// upto 4 functions
		addBits(funcs, 4, packetstream, &pos);
  	}
	else if (nfuncs <=8) {
		addBits(0b0110, 4, packetstream, &pos);	// upto 8 functions
		addBits(funcs, 8, packetstream, &pos);
  	}
  	else {
		addBits(0b0111, 4, packetstream, &pos);	// upto 16 functions
		addBits(funcs, MFX_FX_COUNT, packetstream, &pos);
  	}
  	addCRCBits(packetstream, &pos);
  	unsigned int sizeStream = sendMFXPaket(address, packetstream, QMFX0PKT, 2);
  	update_MFXPacketPool(bus, glp, packetstream, sizeStream);

	//Wenn mehr als 16 MFX Funktionen vorhanden sind -> geänderte Funktionen extra senden
	//Format des Bitstreams für Funktion Einzelansteuerung (9 Bit Adresse):
	//110AAAAAAAAA100NNNNNNN0FCCCCCCCC
	if (nfuncs > 16) {
		uint32_t fxChange = glp->funcschange & 0xffff0000;
		glp->funcschange &= ~fxChange;
		for (int i=16; i<32; i++) {
			if ((fxChange & (1U << i)) != 0) {
				syslog_bus(bus, DBG_DEBUG, "COMP MFX Loco. Adr=%d, nfuncs=%d, Fx%d=%d",
		  						address, nfuncs, i, (funcs >> i) & 1);
				send_LocoFx16_32(address, i, (funcs >> i) & 1, 2);
			}
		}
	}
}

#define REG_COUNTER_FILE SYSCONFDIR"/srcpd.regcount"
/**
 * Den gespeicherten Neuanmeldezähler laden.
 * @return Aktuell gespeicherter Neuanmeldezähler, wenn nichts gespeichert ist 0.
 */
static uint16_t loadRegistrationCounter() {
  syslog_bus(busnumber, DBG_INFO,
  			"Load ReRegistration counter from file %s", REG_COUNTER_FILE);
  int regCountFile = open(REG_COUNTER_FILE, O_RDONLY);
  if (regCountFile < 0) {
    //Kein Reg.Counter File vorhanden -> Start bei 0
    return 0;
  }
  char buffer[10];
  //Lesen und sicherstellen dass mit 0 terminiert wird.
  buffer[read(regCountFile, buffer, sizeof(buffer) - 1)] = 0;
  close(regCountFile);
  return atoi(buffer);
}

/**
 * Den veränderten Neuanmeldezähler speichern.
 * @regCounter Aktueller Neuanmeldezähler, der gespeichert werden soll.
 */
static void saveRegistrationCounter(uint16_t regCounter)
{
	int wrres = 0;
	int regCountFile = open(REG_COUNTER_FILE, O_WRONLY | O_CREAT | O_TRUNC,
												S_IRUSR | S_IWUSR);
	if (regCountFile < 0) wrres = errno;
	else {
		char buffer[10];
		sprintf(buffer, "%d", regCounter);
		if (write(regCountFile, buffer, strlen(buffer)) < 0) wrres = errno;
		close (regCountFile);
	}
	if (wrres) syslog_bus(busnumber, DBG_ERROR,
				"Error %d when trying to store ReRegistration counter", wrres);
}

/**
 * MFX Paket mit UID der Zentrale und Neuanmeldezähler versenden.
 * @param uid
 * @param registrationCounter
 */
static void sendUIDandRegCounter(uint32_t uid, uint16_t registrationCounter)
{
	char packetstream[PKTSIZE] = { 0 };
  //Format des Bitstreams:
  //10AAAAAAA111101UUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUZZZZZZZZZZZZZZZZCCCCCCCC
  //A=0 (Broadcast)
  //U=32 Bit UID
  //Z=16 Bit Neuanmeldezähler
  //C=Checksumme
  unsigned int pos = 0;
  addAdrBits(0, packetstream, &pos); //Adresse 0
  addBits(0b111101, 6, packetstream, &pos); //Kommando UID & RegCounter
  addBits(uid, 32, packetstream, &pos); //32 Bit UID
  addBits(registrationCounter, 16, packetstream, &pos); //16 Bit Neuanmeldezähler
  addCRCBits(packetstream, &pos);

  sendMFXPaket(0, packetstream, QMFX0PKT, 1);
}

/**
 * MFX Dekoder Existanzabfrage senden.
 * Kann mit dekoderUID=0 zum löschend er SID im Dekoder mit anschliessender Neuanmeldung verwendet werden
 * @param adresse SID des Dekoders
 * @param dekoderUID Die UID des Dekoders (0=SID im Dekoder löschen)
 */
static void sendDekoderExist(int adresse, uint32_t dekoderUID, bool noverify)
{
	char packetstream[PKTSIZE] = { 0 };
  //Format des Bitstreams:
  //10AAAAAAA111100UUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUCCCCCCCC
  //Adresse
  //U=32 Bit UID
  //C=Checksumme
  unsigned int pos = 0;
  addAdrBits(adresse, packetstream, &pos); //Adresse 0
  addBits(0b111100, 6, packetstream, &pos); //Kommando Dekoder Existanzabfrage
  addBits(dekoderUID, 32, packetstream, &pos); //32 Bit UID
  addCRCBits(packetstream, &pos);

	unsigned int packetTyp;
	if (noverify)
		packetTyp = QMFX0PKT;		// no verification when assignment removed
	else
		packetTyp = QMFX1PKTV;
	sendMFXPaket(0, packetstream, packetTyp, 1);
}

/**
 * MFX Paket zur Suche nach neuen, noch nicht angemeldeten Dekodern wird versandt.
 * Es wird hier auf die Antwort gewartet.
 * @param countUIDBits Anzahl Bits die im Dekoder Übereinstimmen müssen.
 * @param dekoderUID Dekoder UID die gesucht wird. 0 Beim Start der Suche.
 *                   Wenn ein neuer Dekoder gefunden wurde wird dessen UID zurückgegeben.
 * @return true Wenn ein Dekoder auf die Suche reagiert hat und neu angemeldet werden muss.
 */
static void sendSearchNewDecoder(uint32_t dekoderUID, unsigned int countUIDBits)
{
  //printf("Suche Dekoder C=%d, UID=%u\n", countUIDBits, *dekoderUID);
	char packetstream[PKTSIZE] = { 0 };
  //Format des Bitstreams:
  //10AAAAAAA111010CCCCCCUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUCCCCCCCC
  //A=0 (Broadcast)
  //C=6 Bit Anzahl Bits aus U die im Dekoder übereinstimmen müssen
  //U=32 Bit UID des Dekoder, der gesucht wird.
  //C=Checksumme
  unsigned int pos = 0;
  addAdrBits(0, packetstream, &pos); //Adresse 0
  addBits(0b111010, 6, packetstream, &pos); //Kommando Dekoder Suche
  addBits(countUIDBits, 6, packetstream, &pos); //6 Bit Count relevante UID Bits
  addBits(dekoderUID, 32, packetstream, &pos); //32 Bit UID
  addCRCBits(packetstream, &pos);

	sendMFXPaket(0, packetstream, QMFX1PKTD, 1);
}

/**
 * Neuer Lok Schienenadresse zuweisen.
 * @param dekoderUID UID des neuen Dekoders
 */
static void assignSID(uint32_t dekoderUID, unsigned int sid)
{
	syslog_bus(busnumber, DBG_INFO, "New SID %d for UID %u", sid, dekoderUID);
	char packetstream[PKTSIZE] = { 0 };
  //Format des Bitstreams:
  //10AAAAAAA111011AAAAAAAAAAAAAAUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUCCCCCCCC
  //1.A=0 (Broadcast)
  //2.A die neue Adresse
  //U=32 Bit UID des Dekoder.
  //C=Checksumme
  unsigned int pos = 0;
  addAdrBits(0, packetstream, &pos); //Adresse 0
  addBits(0b111011, 6, packetstream, &pos); //Kommando Dekoder Schienenadresse Zuweiseung
  addBits(sid, 14, packetstream, &pos); //14 Bit neue Adresse
  addBits(dekoderUID, 32, packetstream, &pos); //32 Bit UID
  addCRCBits(packetstream, &pos);

	sendMFXPaket(0, packetstream, QMFX0PKT, 2);
	if ((searchstep == 1) && autosearch) searchstep = 0;	// restart automatic search
}

/**
 * CV einer Lok abrufen.
 * Bei einem Fehler wird bis zu 10 mal wiederholt.
 * @param adresse Schienenadresse des Dekoders
 * @param cv Nummer des CV's (10 Bit)
 * @param index Index im CV (6 Bit)
 * @param bytes Anzahl Bytes die Ab diesem CV ausgelesen werden sollen (1, 2, 4, 8)
 * @param buffer Buffer in den die ausgelesen Bytes geschrieben werden.
 * @return true wenn ohne Fehler ausgelesen werden konnte.
 */
static bool readCV(int adresse, uint16_t cv, uint16_t index, unsigned int bytes)
{
	if ((cv >= CV_SIZE) || (index >= CVINDEX_SIZE)) return false;

	char packetstream[PKTSIZE] = { 0 };
  unsigned int packetTyp;
  unsigned int pos = 0;
  addAdrBits(adresse, packetstream, &pos); //Adresse
  addBits(0b111000, 6, packetstream, &pos); //KommandoCV lesen
  addBits(cv, 10, packetstream, &pos); //10 Bit CV
  addBits(index, 6, packetstream, &pos); //6 Bit Index
	switch (bytes) {						// 2 bits indicating nbr of bytes
		case 1:	addBits(0b00, 2, packetstream, &pos);
				packetTyp = QMFX8PKT;
				break;
		case 2:	addBits(0b01, 2, packetstream, &pos);
				packetTyp = QMFX16PKT;
				break;
		default:addBits(0b10, 2, packetstream, &pos);
				packetTyp = QMFX32PKT;
	}
	addCRCBits(packetstream, &pos);

	sendMFXPaket(0, packetstream, packetTyp, 1);

	return true;
}

#if 0
// HACK: da die einzige Stelle an der das benötigt wurde ausge-IF-t wurde
/**
 * Nächste MFX Lok ermitteln.
 * @param adr Aktuelle Lokadresse.
 *            Es wird die nächste gefunde MFX Lok zurückgegeben.
 *            Bei erreichen der max. Lokadresse wird wieder von vorne begonnen.
 * @return Wenn eine MFX Lok gefunden wurde: die UID dieser, sonst 0.
 */
static uint32_t getNextMFXLok(int *adr) {
  int maxAddr = getMaxAddrGL(busnumber);
  unsigned int i;
//  int glAddr = 0;
  //Maximal alle Loks durchgehen
  for (i=1; i<=maxAddr; i++) {
    (*adr)++;
    if (*adr > maxAddr) {
      *adr = 1;
    }
    gl_data_t gl;
    if (cacheGetGL(busnumber, *adr, &gl) == SRCP_OK) {
      if (gl.protocol == 'X') {
        //Nächste (oder wieder dieselbe) MFX Lok gefunden
        return gl.decuid;
      }
    }
  }
  return 0;
}
#endif

#if 0
/**
 * Vorhandene Loks nach UID durchsuchen.
 * @param adrBekannt Bekannte Lokadresse mit "uid". Diese wird bei der Suche ignoriert.
 *        Wenn 0 übergeben wird: keine bekannte Lokadresse vorhanden
 * @param uid Lokdekoder UID die gesucht werden soll.
 * @retun Lokadresse der Lok mit "uid", resp. 0 wenn nicht gefunden
 */
static int searchLokUID(int adrBekannt, uint32_t uid) {
  int maxAddr = getMaxAddrGL(busnumber);
  unsigned int i;
  int glAddr = 0;
  //Zuerst suchen, ob die UID schon bekannt ist
  for (i=1; i<=maxAddr; i++) {
    if (adrBekannt != i) { //Bekannte Adresse übergehen
      gl_data_t gl;
      if (cacheGetGL(busnumber, i, &gl) == SRCP_OK) {
        if ((gl.protocol == 'X') && (gl.decuid == uid)) {
          //Lok gibt es schon
          glAddr = i;
          break;
        }
      }
    }
  }
  return glAddr;
}
#endif

#if 0
/**
 * Neues MFX Lok Init Kommando empfangen.
 * Wenn diese Lok bereits unter anderer Adresse vorhanden ist -> neue Adresse übernehmen, alte Lok löschen, neue Adresszuordnung senden.
 * @param adresse Lokadresse (Schienenadresse)
 * @param uid Dekoder UID der Lok
 */
void newGLInit(int adresse, uint32_t uid) {
  int glAddr = searchLokUID(adresse, uid);
  if (glAddr > 0) {
    //printf("Neues INIT Adr %d zu bestehender Adr %d für UID=%u\n", adresse, glAddr, uid);
    //Lok Schienenadresse setzen
    assignSID(uid, adresse);
    //Alten Lokeintrag löschen
    if (cacheTermGL(busnumber, glAddr) != SRCP_OK) {
      syslog_bus(busnumber, DBG_ERROR,
                 "Neues INIT Adr %d zu bestehender Adr %d für UID=%u, alte Lok kann nicht gelöscht werden!", adresse, glAddr, uid);
    }
  }
}
#endif

#if 0
/**
 * GL Adresse ermitteln, diese der Lok als Schienenadresse zuweisen.
 * Falls die UID der Lok schon als bekannte MFX Lok vorhanden ist,
 * wird deren Adresse geliefert. Wenn nicht, die erste freie gefundene.
 * @param dekoderUID UID des neuen Dekoders
 * @param bereitsVorhanden Wird auf true gesetzt, wenn Lok UID bereits vorhanden war, false wenn es eine neue UID ist.
 * @return Die neu zugewiesene Schienenadresse. 0 wenn nicht möglich weil keine freie Adresse gefunden wurde.
 */
static unsigned int getSID(uint32_t dekoderUID, bool *bereitsVorhanden) {
  int maxAddr = getMaxAddrGL(busnumber);
  unsigned int i;
  int glAddr = searchLokUID(0, dekoderUID);
  if (glAddr == 0) {
    //Noch nichts gefunden -> neue Lok, erste freie nehmen
    *bereitsVorhanden = false;
    for (i=1; i<=maxAddr; i++) {
      if (! isInitializedGL(busnumber, i)) {
        glAddr = i;
        break;
      }
    }
  }
  else {
    *bereitsVorhanden = true;
  }

  if (glAddr == 0) {
    syslog_bus(busnumber, DBG_WARN,
               "Keine freie GL Adresse vorhanden, neue MFX Lok kann nicht angemeldet werden");
    return 0;
  }
  //Adresse zuweisen
  assignSID(dekoderUID, glAddr);
  return glAddr;
}
#endif

/**
 * MFX Verwaltungsthread:
 * - Periodisches Versenden UID Zentrale und Neuanmeldezähler
 * - Periodisches suchen von noch nicht angemeldeten Dekodern, wenn welche gefunden werden,
 *   dann werden diese angemeldet.
 * @param
 */
long mfxManagement(bus_t busnum)
{
	DDL_DATA *ddl = (DDL_DATA*)buses[busnum].driverdata;
	if ((searchstep < 2) || (searchstep == 32))
		sendUIDandRegCounter(ddl->uid, registrationCounter);

	if (searchstep != 1) {
		unsigned int bitnum = searchstep >> 1;		// bits to be checked
		if (searchstep & 1) decoderUId |= 1 << (32 - bitnum);
		else if (searchstep == 0) decoderUId = 0;	// start from 0
		sendSearchNewDecoder(decoderUId, bitnum);
	}
	return ((searchstep < 2) ? 1000000 : 300000);	// next call in about 1s or 0.3s
}

bool checkMfxCRC(const uint8_t *buff, int n)
{
	uint16_t crcreg = 0xFF;

	for (int i = 0; i < n; i++) {
		crcreg ^= (crcreg << 1) ^ (crcreg << 2) ^ buff[i];
		if (crcreg & 0x0100) crcreg ^= 0x0107;
		if (crcreg & 0x0200) crcreg ^= 0x020E;
	}
	return (crcreg == buff[n]);
}

/**
 * Starten der MFX Threads:
 * - MFX Verwaltung (Lokanmekdungen etc.)
 * - MFX RDS Rückmeldungen
 * @param busnumber SRCP Bus
 * @return 0 für OK, !=0 für Fehler
 */
int startMFXManagement(bus_t _busnumber, int search)
{
	if (busnumber) return 2;		// don't start threads twice
	busnumber = _busnumber;

	registrationCounter = loadRegistrationCounter();
	syslog_bus(busnumber, DBG_INFO,
				"ReRegistration counter initialized with %d", registrationCounter);
	autosearch = search;
	searchstep = autosearch ? 0 : 1;
	return 0;
}

/**
 * stop the mfx search for new decoders
 */
void stopMFXSearch()
{
	searchstep = 1;
}

/**
 * Schienenadresse im Dekoder löschen -> Dekoder ist nicht mehr angemeldet.
 * @param sid	actual SID
 */
void sendDekoderTerm(int sid) {
  sendDekoderExist(sid, 0, true);
}

//---------------------------- SM ----------------------------------------

/**
 * generate mfx discovery report
 */
static int discoveryReport(uint8_t askval)
{
	char minfo[4], info[250];
	struct timeval now;

	minfo[0] = 2;						// send actual partial id
	minfo[1] = searchstep >> 1;
	minfo[2] = askval;
	info_mcs(busnumber, 0x03, decoderUId, minfo);
	if ((minfo[1] == 32) && askval) {
		minfo[0] = 1;					// send final id
		info_mcs(busnumber, 0x03, decoderUId, minfo);

		gettimeofday(&now, NULL);
		sprintf(info, "%lld.%.3ld 100 INFO %lu SM %u MFXDISCOVERY\n",
				(long long) now.tv_sec, (long) (now.tv_usec / 1000),
				busnumber, decoderUId);
		enqueueInfoMessage(info);

		syslog_bus(busnumber, DBG_INFO, "Discovery of decoder with UID %X", decoderUId);
		return 1;		// discovery completed
	}
	return 0;			// discovery ongoing
}

/**
 * handle result received via serial feedback port
 */
void serialMFXresult(uint8_t *buf, int len)
{
	char dbgmsg[200];
	DDL_DATA *ddl = (DDL_DATA*)buses[busnumber].driverdata;
	uint8_t	event = 0x43;
	int wrres = 0;

	for (int n=0; n<len; n++) sprintf(dbgmsg + 3 * n, " %02X", buf[n]);
	syslog_bus(busnumber, DBG_DEBUG, "read data len %d:%s", len, dbgmsg);

	ddl->fbData.serask = buf[0];
	if (ddl->fbData.fbbytnum) {		// reply for a read command
		int minreq = 4 + 2 * ddl->fbData.fbbytnum;
//		syslog_bus(busnumber, DBG_DEBUG, "minimum required data length: %d", minreq);

		if ((len >= minreq) && (buf[1] == 0xA)) {
			for (int n = 0; n <= ddl->fbData.fbbytnum; n++) {
				ddl->fbData.serfbdata[n] = (buf[2*n+2] << 4) | (buf[2*n+3] & 0xF); 
//				syslog_bus(busnumber, DBG_DEBUG, "ser data %d is 0x%02X", n, ddl->fbData.serfbdata[n]);
			}
			if (checkMfxCRC(ddl->fbData.serfbdata, ddl->fbData.fbbytnum)) {
				syslog_bus(busnumber, DBG_DEBUG, "CRC OK");
				event = 0x42;
				wrres = write(ddl->feedbackPipe[1], &event, 1);
			}
		}
		else syslog_bus(busnumber, DBG_DEBUG, 
			"mfx rec data framing error: len %d, min %d, start code 0x%X", len, minreq, buf[1]);
	}
	else wrres = write(ddl->feedbackPipe[1], &event, 1);
	if (wrres < 0) syslog_bus(busnumber, DBG_ERROR, "write feedback pipe failed");
}

/**
 * generate report when read successful
 */
void sendMFXreadCVresult(uint8_t *data)
{
	char minfo[4], info[250];
	struct timeval now;
	DDL_DATA *ddl = (DDL_DATA*)buses[busnumber].driverdata;

	gettimeofday(&now, NULL);
	sprintf(info, "%lld.%.3ld 100 INFO %lu SM %d CVMFX %d %d",
			(long long) now.tv_sec, (long) (now.tv_usec / 1000),
			busnumber, mfxread.addr, mfxread.cvline, mfxread.cvindex);

	for (int i = 0; i < ddl->fbData.fbbytnum; i++) {
		minfo[0] = 3;					// send read config responses
		minfo[1] = (mfxread.cvline >> 8) | ((mfxread.cvindex + i) << 2);
		minfo[2] = mfxread.cvline & 0xFF;
		minfo[3] = data[i];
		info_mcs(busnumber, 0x0F, mfxread.addr | 0x4000, minfo);
		sprintf(info + strlen(info), " %d", data[i]);
	}
	sprintf(info + strlen(info), "\n");
	enqueueInfoMessage(info);

	usleep(200000);						// cooling gap
	mfxread.cvnmbr -= 4;
	if (mfxread.cvnmbr > 0) {
		mfxread.cvindex += 4;
		mfxread.repcount = 0;
		ddl->resumeSM = PROTO_MFX;		// trigger resume
	}
}

/**
 * handle response from mfx ack
 */
void handleMFXacknowledge(uint8_t askval)
{
	char minfo[4], info[250];
	struct timeval now;
	DDL_DATA *ddl = (DDL_DATA*)buses[busnumber].driverdata;
	syslog_bus(busnumber, DBG_INFO, "MFX - A C K  auf Code %d", ddl->fbData.pktcode);

	switch (ddl->fbData.pktcode) {
	case QMFX1PKTD:						// handle discovery packet - carrier detected
					if (discoveryReport(askval)) searchstep = 1;	// suspend
					else if (searchstep & 1) searchstep++;	// try next bit set to 0
					else searchstep += 2;
					break;
	case QMFX1PKTV:	minfo[0] = 3;		// send verify response with ASK
					minfo[1] = mfxread.addr >> 8;
					minfo[2] = mfxread.addr & 0xFF;
					minfo[3] = askval;
					info_mcs(busnumber, 7, mfxread.decuid, minfo);

					gettimeofday(&now, NULL);
					sprintf(info, "%lld.%.3ld 100 INFO %lu SM %d BIND %u\n",
							(long long) now.tv_sec, (long) (now.tv_usec / 1000),
							busnumber, mfxread.addr, mfxread.decuid);
					enqueueInfoMessage(info);
					break;
	}
}

/**
 * handle mfx response timeout
 */
void handleMFXtimeout(void)
{
	char minfo[4];
	DDL_DATA *ddl = (DDL_DATA*)buses[busnumber].driverdata;
	syslog_bus(busnumber, DBG_INFO, "MFX - TIMEOUT auf Code %d", ddl->fbData.pktcode);

	switch (ddl->fbData.pktcode) {
	case QMFX1PKTD:					// handle discovery packet - NO carrier detected
					if (searchstep) discoveryReport(0);
					if (searchstep & 1) {
						syslog_bus(busnumber, DBG_INFO,
							"Discoyery restarted due to error at step %d", searchstep);
						searchstep = 0;					// try again from start
					}
					else if (searchstep) searchstep++;	// try with bit set to 1
					break;
	case QMFX1PKTV:	minfo[0] = 2;		// send verify response with SID = 0
					minfo[1] = 0;
					minfo[2] = 0;
					info_mcs(busnumber, 7, mfxread.decuid, minfo);
					break;
	default:	if (++mfxread.repcount < MAX_RDS_TRIALS)
					ddl->resumeSM = PROTO_MFX;		// trigger resume
				else {
					minfo[0] = 2;		// send read config fail responses
					minfo[1] = (mfxread.cvline >> 8) | (mfxread.cvindex << 2);
					minfo[2] = mfxread.cvline & 0xFF;
					info_mcs(busnumber, 0x0F, mfxread.addr | 0x4000, minfo);
				}
	}
}

/**
 * resume CV read sequence
 */
void resumeMfxGetCV(void)
{
	syslog_bus(busnumber, DBG_INFO, "RESUME GET MFX Adr=%d, Get CV=%d, Index=%d, Len=%d",
							mfxread.addr, mfxread.cvline, mfxread.cvindex, mfxread.cvnmbr);
  	readCV(mfxread.addr, mfxread.cvline, mfxread.cvindex, mfxread.cvnmbr);
}

/**
 * Servicemode ein-ausschalten
 * @param smOn true=SM ein, false=SM aus
 */
void setMfxSM(bool smOn) {
	sm = smOn;
}

/**
 * SM SET: 1 Byte in den MFX Dekoder an CV/Index schreiben.
 * @param address Lokadresse
 * @param cv CV Adresse
 * @param index Index innerhalb CV
 * @param value Zu schreibendes Byte
 */
int smMfxSetCV(int address, int cv, int index, int value)
{
	if ((cv >= CV_SIZE) || (index >= CVINDEX_SIZE)) return -1;
    if (! buses[busnumber].power_state) return -2;

	syslog_bus(busnumber, DBG_INFO, "SM MFX Adr=%d, Set CV=%d, Index=%d, Val=%d",
									address, cv, index, value);
	char packetstream[PKTSIZE] = { 0 };
  unsigned int pos = 0;
  addAdrBits(address, packetstream, &pos); //Adresse
  addBits(0b111001, 6, packetstream, &pos); //Kommando CV Schreiben
  addBits(cv, 10, packetstream, &pos); //10 Bit CV
  addBits(index, 6, packetstream, &pos); //6 Bit Index
  addBits(0b00, 2, packetstream, &pos); //Immer 0 (1 Byte)
  addBits(value, 8, packetstream, &pos); //Das zu schreibende Byte
  addCRCBits(packetstream, &pos);

  	sendMFXPaket(address, packetstream, QMFX0PKT, 2);
	return 0;
}

/**
 * SM SET: 1 Byte in den MFX Dekoder an CV/Index schreiben.
 * @param address Lokadresse
 * @param cv CV Adresse
 * @param index Index innerhalb CV
 * @return Gelesenes Byte 0..255, < 0 für Error
 */
int smMfxGetCV(int address, int cv, int index, int nmbr)
{
  //Nur erlaubt wenn SM Initialisiert ist, MFX Loksuche ausgeschaltet
//	if (! sm) return -1;
    if (! buses[busnumber].power_state) return -2;

	mfxread.addr = address;		// remember task details for continueing and answering
    mfxread.cvline = cv;
    mfxread.cvindex = index;
    mfxread.cvnmbr = nmbr;
    mfxread.repcount = 0;

	syslog_bus(busnumber, DBG_INFO, "SM GET MFX Adr=%d, Get CV=%d, Index=%d, Len=%d",
									address, cv, index, nmbr);

  	return (readCV(address, cv, index, nmbr)) ? 0 : -1;
}


/* MFX BIND */
// if address == 0 uid is used to set the registration counter
int smMfxSetBind(int address, uint32_t uid)
{
	if (address == 0) {
		if (registrationCounter != uid)
  			saveRegistrationCounter(uid);
		registrationCounter = uid;
		syslog_bus(busnumber, DBG_INFO, "** mfx RegCount set to %x", uid);
		return registrationCounter;
	}
    if (! buses[busnumber].power_state) return -2;

	assignSID(uid, address);
    gl_data_t *p = get_gldata_ptr(busnumber, address | 0x4000);
    if (p) p->decuid = uid;
	return 0;
}

/* MFX VERIFY */
// if  uid == 0 then do unbind
int smMfxVerBind(int address, uint32_t uid)
{
	if (address == 0) return registrationCounter;

	if (! buses[busnumber].power_state) return -2;
	mfxread.decuid = uid;
	mfxread.addr = address;		// remember task details for continueing and answering

	sendDekoderExist(address, uid, false);
    gl_data_t *p = get_gldata_ptr(busnumber, address | 0x4000);
    if (p) p->decuid = uid;
	return 0;
}

