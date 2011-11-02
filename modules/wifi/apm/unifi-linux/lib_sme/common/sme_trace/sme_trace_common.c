/** @file sme_trace_common.c
 *
 * Provides a port for the SME trace prototypes
 *
 * @section LEGAL
 *   CONFIDENTIAL
 *
 *   Copyright (C) Cambridge Silicon Radio Ltd 2008. All rights reserved.
 *
 *   Refer to LICENSE.txt included with this source for details on the
 *   license terms.
 *
 * @section DESCRIPTION
 *   stdout implementation for the sme_trace.h prototypes
 *
 ****************************************************************************
 *
 * @section REVISION
 *   $Id: //depot/dot11/v7.0p/host/lib_sme/common/sme_trace/sme_trace_common.c#1 $
 *
 ****************************************************************************/

#include "abstractions/osa.h"
#include "sme_trace/sme_trace.h"
#include "sme_trace/sme_trace_port.h"

#ifdef SME_TRACE_ENABLE

static const char* const levelcfg[TR_LVL_OFF + 1] = { "TR_LVL_ENTRY",
                                                      "TR_LVL_DEBUG",
                                                      "TR_LVL_INFO",
                                                      "TR_LVL_WARN",
                                                      "TR_LVL_ERROR",
                                                      "TR_LVL_CRIT",
                                                      "TR_LVL_OFF" };

sme_trace_moduleconfig modulecfg[TR_LAST_MODULE_ID] = {
    /*  ----------------------------------- */
    /*        SME TRACE CODE                */
    /*  ----------------------------------- */
    { "SMETRACE",         TR_LVL_ERROR },
    /*  ----------------------------------- */

    /*  ----------------------------------- */
    /*                  SME LIBS            */
    /*  ----------------------------------- */
    { "CSRLIST",         TR_LVL_ERROR },
    { "IEACCSS",         TR_LVL_ERROR },
    { "IE_PRT ",         TR_LVL_ERROR },
    { "PLD    ",         TR_LVL_ERROR },
    { "OSA    ",         TR_LVL_ERROR },
    { "IPC    ",         TR_LVL_ERROR },
    /*  ----------------------------------- */

    /*  ----------------------------------- */
    /*                  SME SAPS            */
    /*  ----------------------------------- */
    { "STARTUP",         TR_LVL_ERROR },
    { "SYS_SAP ",        TR_LVL_ERROR },
    { "MGT_SAP",         TR_LVL_ERROR },
    { "DBG_SAP",         TR_LVL_ERROR },
    { "BT_SAP",          TR_LVL_ERROR },
    /*  ----------------------------------- */

    /*  ----------------------------------- */
    /*          SME PROCESSES / MODULES     */
    /*  ----------------------------------- */
    { "SCANSTR",         TR_LVL_ERROR },
    { "CORE   ",         TR_LVL_ERROR },
    { "CONMGR ",         TR_LVL_ERROR },
    { "SEC    ",         TR_LVL_ERROR },
    { "MIBACC ",         TR_LVL_ERROR },
    { "SCANMGR",         TR_LVL_ERROR },
    { "LINKQ  ",         TR_LVL_ERROR },
    { "DBG    ",         TR_LVL_ERROR },
    { "HIPPRXY",         TR_LVL_ERROR },
    { "MIBFSM ",         TR_LVL_ERROR },
    { "NETSEL ",         TR_LVL_ERROR },
    { "UNIFI  ",         TR_LVL_ERROR },
    { "CONFIG ",         TR_LVL_ERROR },
    { "MEASURE",         TR_LVL_ERROR },
    { "POWER  ",         TR_LVL_ERROR },
    { "COEX   ",         TR_LVL_ERROR },
    { "CCX    ",         TR_LVL_ERROR },
    { "DOT11N ",         TR_LVL_ERROR },
    { "REGDOM ",         TR_LVL_ERROR },
    { "QOS    ",         TR_LVL_ERROR },
    { "CRYPTO ",         TR_LVL_ERROR },
    /*  ----------------------------------- */

    /*  ----------------------------------- */
    /* FSM                                  */
    /*  ----------------------------------- */
    { "FSM    ",         TR_LVL_ERROR },
    { "FSMDUMP",         TR_LVL_ERROR },
    { "MSC    ",         TR_LVL_ERROR },
    /*  ----------------------------------- */

    /*  ----------------------------------- */
    /* SECURITY LIBRARY                     */
    /*  ----------------------------------- */
    { "SECLIB ",         TR_LVL_ERROR },
    { "SECWAPI",         TR_LVL_ERROR },



    /*  ----------------------------------------- */
    /* PAL                                        */
    /*  ----------------------------------------- */
#ifdef CSR_AMP_ENABLE
    { "HCI_SAP",         TR_LVL_ERROR  },
    { "ACL_SAP",         TR_LVL_ERROR  },
    { "CTRL_SAP",        TR_LVL_ERROR  },
    { "PAL_SYS_SAP",     TR_LVL_ERROR  },
    { "PAL_MGR",         TR_LVL_ERROR  },
    { "PAL_LM ",         TR_LVL_ERROR  },
    { "PAL_DM ",         TR_LVL_ERROR  },
    { "PAL_DAM",         TR_LVL_ERROR  },
    { "PAL_HIP",         TR_LVL_ERROR  },
    { "PAL_COEX",        TR_LVL_ERROR  },
#endif

    /*  ----------------------------------------- */
    /* NME                                        */
    /*  ----------------------------------------- */
#ifdef CSR_WIFI_NME_ENABLE
    { "NNMESAP",         TR_LVL_ERROR  },
    { "NMGTSAP",         TR_LVL_ERROR  },
    { "NMESIG",          TR_LVL_ERROR  },
    { "NMECORE",         TR_LVL_ERROR  },
    { "NMEPMGR",         TR_LVL_ERROR  },
    { "NMECMGR",         TR_LVL_ERROR  },
    { "NMESMGR",         TR_LVL_ERROR  },
    { "NME_CCX",         TR_LVL_ERROR  },
    { "NME_WPS",         TR_LVL_ERROR  },
    { "NME_NS ",         TR_LVL_ERROR  },
    { "NMESIGR",         TR_LVL_ERROR  },
#endif

    /*  ----------------------------------------- */
};

void sme_trace_initialise(CsrUint32 argc, char **argv)
{
    CsrUint32 i;
    char* parsestring = NULL;
    sme_trace_entry((TR_SME_TRACE, "sme_trace_initialise(%d)", argc));

    sme_trace_initialise_port(argc, argv);

    for (i = 0; i < argc; i++)
    {
        /* Test Each arg */
        if (CsrStrNCmp(argv[i], "-sme_trace:", 11) == 0)
        {
            sme_trace_module_id moduleid = TR_LAST_MODULE_ID + 1;

            /* Check for multiple trace is the single string */
            CsrUint32 strIndex = 0;
            CsrUint32 strLen = CsrStrLen(&argv[i][11]);
            char* strarg = (char*) CsrPmalloc(strLen + 1);

            while (strIndex < strLen)
            {
                CsrUint32 tempIndex = 0;
                CsrStrCpy(strarg, &argv[i][strIndex + 11]);
                /* Find end of string or space */

                while (strIndex < strLen)
                {
                    if (strarg[tempIndex] == ',')
                    {
                        strarg[tempIndex] = 0;
                        strIndex++;
                        break;
                    }
                    tempIndex++;
                    strIndex++;
                }
                parsestring = strarg;

                /* ----------------------------------------------------------- */
                /* Parse for the module                                        */
                /* ----------------------------------------------------------- */
                if (CsrStrNCmp(parsestring, "ALL=", 4) == 0)
                {
                    moduleid = TR_LAST_MODULE_ID;
                    parsestring += 4;

                }
                else
                {
                    sme_trace_module_id j;
                    for (j = (sme_trace_module_id)0; j < TR_LAST_MODULE_ID; j++)
                    {
                        CsrUint32 modulelen = CsrStrLen(modulecfg[j].name);
                        while(modulelen && modulecfg[j].name[modulelen-1] == ' ')
                        {
                            modulelen--;
                        }
                        if (CsrStrNCmp(parsestring, modulecfg[j].name, modulelen) == 0)
                        {
                            if (parsestring[modulelen] == '=')
                            {
                                moduleid = j;
                                parsestring += modulelen + 1;
                                break;
                            }
                        }
                    }
                }
                /* ----------------------------------------------------------- */

                /* ----------------------------------------------------------- */
                /* Parse for the level                                         */
                /* ----------------------------------------------------------- */
                if (moduleid != TR_LAST_MODULE_ID + 1)
                {
                    sme_trace_level k;
                    sme_trace_level level = (sme_trace_level)(TR_LVL_OFF + 1);
                    /* Check Eack Level */
                    for (k = (sme_trace_level)0; k <= TR_LVL_OFF; k++)
                    {
                        CsrUint32 levellen = CsrStrLen(levelcfg[k]);
                        if (CsrStrNCmp(parsestring, levelcfg[k], levellen) == 0)
                        {
                            level = k;
                            break;
                        }
                    }
                    if (level == TR_LVL_OFF + 1)
                    {
                        sme_trace_error((TR_SME_TRACE, "sme_trace_initialise(%s) :: Unknown Trace level", argv[i]));
                    }
                    else if (moduleid == TR_LAST_MODULE_ID)
                    {
                        /* ----------------------------------------------------------- */
                        /* SET ALL THE LEVELS */
                        /* ----------------------------------------------------------- */
                        sme_trace_set_all_levels(level);
                        /* ----------------------------------------------------------- */
                    }
                    else
                    {
                        /* ----------------------------------------------------------- */
                        /* SET THE LEVEL */
                        /* ----------------------------------------------------------- */
                        sme_trace_set_module_level(moduleid, level);
                        /* ----------------------------------------------------------- */
                    }

                }
                else
                {
                    sme_trace_error((TR_SME_TRACE, "sme_trace_initialise(%s) :: Unknown Trace module", argv[i]));
                }
                /* ----------------------------------------------------------- */
            }
            CsrPfree(strarg);
        }
    }
}

void sme_trace_set_all_levels(sme_trace_level level)
{
    CsrUint32 i;
    if (level > TR_LVL_OFF)
    {
        sme_trace_error((TR_SME_TRACE, "sme_trace_set_all_levels(%d) :: Unknown trace level", level));
        return;
    }
    for (i = 0; i < TR_LAST_MODULE_ID; i++)
    {
        modulecfg[i].level = level;
    }
}

void sme_trace_set_module_level(sme_trace_module_id id, sme_trace_level level)
{
    if (id >= TR_LAST_MODULE_ID)
    {
        sme_trace_error((TR_SME_TRACE, "sme_trace_set_module_level(%d,%d) :: Unknown module id", id, level));
        return;
    }
    if (level > TR_LVL_OFF)
    {
        sme_trace_error((TR_SME_TRACE, "sme_trace_set_module_level(%d) :: Unknown trace level", id, level));
        return;
    }
    modulecfg[id].level = level;
}

sme_trace_level sme_trace_get_module_level(sme_trace_module_id id)
{
    if (id >= TR_LAST_MODULE_ID)
    {
        sme_trace_error((TR_SME_TRACE, "sme_trace_get_module_level(%d) :: Unknown module id", id));
        return TR_LVL_OFF;
    }
    return modulecfg[id].level;
}
#endif


