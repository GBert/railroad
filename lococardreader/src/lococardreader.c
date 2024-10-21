#ifdef WIN32
#undef UNICODE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#include <PCSC/winscard.h>
#include <PCSC/wintypes.h>
#else
#include <winscard.h>
#endif

#define CHUNK	16

#ifdef WIN32
static char *pcsc_stringify_error(LONG rv) {
    static char out[20];
    sprintf_s(out, sizeof(out), "0x%08X", rv);

    return out;
}
#endif

void printbuffer(BYTE *buffer, BYTE length) {
    int i;
    for (i = 0; i < length; i++)
	printf("%02X ", buffer[i]);
    printf("\n");
}

#define CHECK(f, rv) \
 if (SCARD_S_SUCCESS != rv) \
 { \
  printf(f ": %s\n", pcsc_stringify_error(rv)); \
  return -1; \
 }

int main(void) {
    LONG rv;

    SCARDCONTEXT hContext;
    LPTSTR mszReaders;
    SCARDHANDLE hCard;
    DWORD dwReaders, dwActiveProtocol, dwRecvLength;

    SCARD_IO_REQUEST pioSendPci;
    BYTE pbRecvBuffer[258];
    BYTE MS2_BIN[8192];
    BYTE cmd1[] = { 0xFF, 0xA4, 0x00, 0x00, 0x01, 0x02 };
    BYTE cmd2[] = { 0xFF, 0xB0, 0x00, 0x00, 0x10 };

    unsigned int i;

    rv = SCardEstablishContext(SCARD_SCOPE_USER, NULL, NULL, &hContext);
    CHECK("SCardEstablishContext", rv)
#ifdef SCARD_AUTOALLOCATE
	dwReaders = SCARD_AUTOALLOCATE;

    rv = SCardListReaders(hContext, NULL, (LPTSTR) & mszReaders, &dwReaders);
    CHECK("SCardListReaders", rv)
#else
	rv = SCardListReaders(hContext, NULL, NULL, &dwReaders);
    CHECK("SCardListReaders", rv)
	mszReaders = calloc(dwReaders, sizeof(char));
    rv = SCardListReaders(hContext, NULL, mszReaders, &dwReaders);
    CHECK("SCardListReaders", rv)
#endif
    printf("reader name: %s\n", mszReaders);

    rv = SCardConnect(hContext, mszReaders, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
    CHECK("SCardConnect", rv)
	switch (dwActiveProtocol) {
    case SCARD_PROTOCOL_T0:
	pioSendPci = *SCARD_PCI_T0;
	break;

    case SCARD_PROTOCOL_T1:
	pioSendPci = *SCARD_PCI_T1;
	break;
    }
    dwRecvLength = sizeof(pbRecvBuffer);
    rv = SCardTransmit(hCard, &pioSendPci, cmd1, sizeof(cmd1), NULL, pbRecvBuffer, &dwRecvLength);
    CHECK("SCardTransmit", rv)
	printf("response: ");
    printbuffer(pbRecvBuffer,dwRecvLength);

    dwRecvLength = sizeof(pbRecvBuffer);
    for (i = 0; i < 8192; i += CHUNK) {
	cmd2[2] = i >> 8;
	cmd2[3] = i & 255;
	rv = SCardTransmit(hCard, &pioSendPci, cmd2, sizeof(cmd2), NULL, pbRecvBuffer, &dwRecvLength);
	CHECK("SCardTransmit", rv);
	memcpy(&MS2_BIN[i], pbRecvBuffer, CHUNK);

	printf("response: ");
	printbuffer(pbRecvBuffer, dwRecvLength);
    }

    rv = SCardDisconnect(hCard, SCARD_LEAVE_CARD);
    CHECK("SCardDisconnect", rv)
#ifdef SCARD_AUTOALLOCATE
    rv = SCardFreeMemory(hContext, mszReaders);
    CHECK("SCardFreeMemory", rv)
#else
    free(mszReaders);
#endif

    rv = SCardReleaseContext(hContext);

    CHECK("SCardReleaseContext", rv)
    return 0;
}
