/// @file
/// @brief General definitions used in the AERA DAQ
/// @author C. Timmermans, Nikhef/RU
#ifndef _AMSG_H_
#define _AMSG_H_


#include <stdint.h>

#define DU_BOOT     0      //!< Message tag as defined by DAQ group
#define DU_UPLOAD   1      //!< Message tag as defined by DAQ group
#define DU_RESET    2      //!< Message tag as defined by DAQ group
#define DU_INITIALIZE 3    //!< Message tag as defined by DAQ group
#define DU_SET      4      //!< Message tag as defined by DAQ group
#define DU_GET      5      //!< Message tag as defined by DAQ group
#define DU_MONITOR  6      //!< Message tag as defined by DAQ group
#define DU_START    7      //!< Message tag as defined by DAQ group
#define DU_STOP     8      //!< Message tag as defined by DAQ group
#define DU_T2       9      //!< Message tag as defined by DAQ group
#define DU_GETEVENT 10     //!< Message tag as defined by DAQ group
#define DU_NO_EVENT 11     //!< Message tag as defined by DAQ group
#define DU_EVENT    12     //!< Message tag as defined by DAQ group
#define DU_GETMEM   13     //!< Message tag as defined by DAQ group
#define DU_SETMEM   14     //!< Message tag as defined by DAQ group
#define DU_SETBIT   15     //!< Message tag as defined by DAQ group
#define DU_CLRBIT   16     //!< Message tag as defined by DAQ group
#define DU_GETFILE  17     //!< Message tag as defined by DAQ group
#define DU_SENDFILE 18     //!< Message tag as defined by DAQ group
#define DU_EXEC     19     //!< Message tag as defined by DAQ group
#define DU_CONNECT  20     //!< Message tag as defined by DAQ group
#define DU_LISTEN   21     //!< Message tag as defined by DAQ group
#define DU_SCRIPT   22     //!< Message tag as defined by DAQ group
#define DU_GET_MINBIAS_EVENT           29 //!< Message tag as defined by DAQ group
#define DU_GET_RANDOM_EVENT            32 //!< Message tag as defined by DAQ group
#define T3_EVENT_REQUEST_LIST 201  //!< Message tag as defined by DAQ group
#define GUI_UPDATE_DB 401          //!< Message tag as defined by DAQ group
#define GUI_INITIALIZE 402         //!< Message tag as defined by DAQ group
#define GUI_START_RUN 403          //!< Message tag as defined by DAQ group
#define GUI_STOP_RUN 404           //!< Message tag as defined by DAQ group
#define GUI_DELETE_RUN 406         //!< Message tag as defined by DAQ group
#define GUI_BC_SETATTENUATION  450  //!< Message tag as defined by DAQ group
#define GUI_BC_SWITCHIO  451        //!< Message tag as defined by DAQ group
#define GUI_BC_SETMODE  452         //!< Message tag as defined by DAQ group

#define GRND1 0x5247    //!< End word1 for a message (GR)
#define GRND2 0x444E    //!< End word 2 for a message (ND)
#define ALIVE 9998      //!< Alive command sent by central DAQ
#define ALIVE_ACK 9999  //!< Acknowledge alive, sent by DU

#define INTSIZE 4

typedef struct{
  uint32_t length;      //!< Message length, including the length word itself
  uint32_t tag;         //!< Message type
  uint32_t body[];      //!< the body of a message
}AMSG;

#define SOCKETS_REUSEADDR_OPT 1
#define SOCKETS_KEEPALIVE_OPT 0
#define SOCKETS_NODELAY_OPT 1
#define SOCKETS_BUFFER_SIZE  4096
#define SOCKETS_TIMEOUT      100 //!< millisec


#define AMSG_OFFSET_LENGTH  0 //!< offset for message length wrt start of msg
#define AMSG_OFFSET_TAG     1 //!< offset for message tag wrt start of msg
#define AMSG_OFFSET_BODY    2 //!< offset for message body wrt start of msg
#else
#endif
