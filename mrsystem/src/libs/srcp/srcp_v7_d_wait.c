#include "srcp_parse.h"
#include "srcp.h"

int SrcpV7DecWait(char *Line, int Length, SrcpV7ParamType *Params)
{  int Token, Ret;

   if (SrcpParserInternalInit(Line, Length))
   {
      Ret = SRCP_V7_TARGET_ERROR;
      do {
         Token = SrcpParserInternalParse();
         switch (Token)
         {
            case PARSER_EOF:
               break;
            case PARSER_WAIT:
               break;
            case PARSER_GL:
               Ret = SRCP_V7_TARGET_GL_GENERIC_LOCO;
               break;
            case PARSER_GA:
               Ret = SRCP_V7_TARGET_GA_GENERIC_ACCESSORY;
               break;
            case PARSER_TIME:
               Token = SrcpParserInternalParse();
               Params->TargetParms.TimeParams.JulDay = SrcpParserInternalGetGanz();
               Token = SrcpParserInternalParse();
               Params->TargetParms.TimeParams.Hour = SrcpParserInternalGetGanz();
               Token = SrcpParserInternalParse();
               Params->TargetParms.TimeParams.Minute = SrcpParserInternalGetGanz();
               Token = SrcpParserInternalParse();
               Params->TargetParms.TimeParams.Second = SrcpParserInternalGetGanz();
               Ret = SRCP_V7_TARGET_TIME;
               break;
            case PARSER_POWER:
               Ret = SRCP_V7_TARGET_POWER;
               break;
            case PARSER_FB:
               Token = SrcpParserInternalParse();
               strcpy(Params->TargetParms.FbParams.ModuleType,
                      SrcpParserInternalGetString());
               Token = SrcpParserInternalParse();
               Params->TargetParms.FbParams.Addr = SrcpParserInternalGetGanz();
               Token = SrcpParserInternalParse();
               Params->TargetParms.FbParams.Value = SrcpParserInternalGetGanz();
               Token = SrcpParserInternalParse();
               Params->TargetParms.FbParams.Timeout = SrcpParserInternalGetGanz();
               Ret = SRCP_V7_TARGET_FB_FEEDBACK;
               break;
            default:
               Ret = SRCP_V7_TARGET_UNKNOWN;
               break;
         }
      } while (Token != PARSER_EOF);
      SrcpParserInternalExit();
   }
   else
   {
      Ret = SRCP_V7_TARGET_ERROR;
   }
   return(Ret);
}
