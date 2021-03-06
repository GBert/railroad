#include <stddef.h>
#include <string.h>
#include "mr_ipc.h"

void MrIpcCmdGetStatusData(MrIpcCmdType *Data, unsigned char *Buf)
{
   if (Data != (MrIpcCmdType *)NULL)
   {
      memcpy(Buf, MrIpcGetRawData(Data), MR_CS2_NUM_CAN_BYTES);
   }
}
