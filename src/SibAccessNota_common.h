/*
 * SibAccessNota_common.h
 *
 * Generated by nota-stubgen3.pl 3.0pre1
 *  Date: 24-5-2010 11:19:46
 *
 * Source: sib.xml
 *  Created: 24-5-2010 9:59:8
 *  Modified: 24-5-2010 9:59:8
 *  Accessed: 24-5-2010 11:19:46
 *
 * Stub adapter version 3.0
 *
 */


#ifndef __SIBACCESSNOTA_COMMON_H___
#define __SIBACCESSNOTA_COMMON_H___


/* include H_IF BSDAPI definitions */
#include <h_in/h_bsdapi.h>


/* Include general support code */
#include <stubgen/hin3_stubadapter_if.h>


/* Lengths of packets */
#define JOIN_REQ_PACKET_LENGTH 5
#define JOIN_CNF_PACKET_LENGTH 10
#define INSERT_REQ_PACKET_LENGTH 15
#define INSERT_CNF_PACKET_LENGTH 10
#define REMOVE_REQ_PACKET_LENGTH 15
#define REMOVE_CNF_PACKET_LENGTH 10
#define UPDATE_REQ_PACKET_LENGTH 15
#define UPDATE_CNF_PACKET_LENGTH 10
#define QUERY_REQ_PACKET_LENGTH 15
#define QUERY_CNF_PACKET_LENGTH 10
#define SUBSCRIBE_REQ_PACKET_LENGTH 15
#define SUBSCRIBE_CNF_PACKET_LENGTH 10
#define UNSUBSCRIBE_REQ_PACKET_LENGTH 5
#define UNSUBSCRIBE_CNF_PACKET_LENGTH 10
#define LEAVE_REQ_PACKET_LENGTH 5
#define LEAVE_CNF_PACKET_LENGTH 10
#define SUBSCRIPTION_IND_PACKET_LENGTH 10
#define UNSUBSCRIBE_IND_PACKET_LENGTH 10
#define LEAVE_IND_PACKET_LENGTH 10


#define JOIN_REQ_TOKEN_NUMBER 4
#define INSERT_REQ_TOKEN_NUMBER 6
#define REMOVE_REQ_TOKEN_NUMBER 6
#define UPDATE_REQ_TOKEN_NUMBER 7
#define QUERY_REQ_TOKEN_NUMBER 6
#define SUBSCRIBE_REQ_TOKEN_NUMBER 6
#define UNSUBSCRIBE_REQ_TOKEN_NUMBER 4
#define LEAVE_REQ_TOKEN_NUMBER 3


#define JOIN_CNF_TOKEN_NUMBER 5
#define INSERT_CNF_TOKEN_NUMBER 5
#define REMOVE_CNF_TOKEN_NUMBER 4
#define UPDATE_CNF_TOKEN_NUMBER 5
#define QUERY_CNF_TOKEN_NUMBER 5
#define SUBSCRIBE_CNF_TOKEN_NUMBER 6
#define UNSUBSCRIBE_CNF_TOKEN_NUMBER 4
#define LEAVE_CNF_TOKEN_NUMBER 4
#define SUBSCRIPTION_IND_TOKEN_NUMBER 7
#define UNSUBSCRIBE_IND_TOKEN_NUMBER 5
#define LEAVE_IND_TOKEN_NUMBER 4


/* Signal ids */
#define JOIN_REQ_SIGNAL_ID 0x1001
#define JOIN_CNF_SIGNAL_ID 0x1002
#define INSERT_REQ_SIGNAL_ID 0x1003
#define INSERT_CNF_SIGNAL_ID 0x1004
#define REMOVE_REQ_SIGNAL_ID 0x1005
#define REMOVE_CNF_SIGNAL_ID 0x1006
#define UPDATE_REQ_SIGNAL_ID 0x1007
#define UPDATE_CNF_SIGNAL_ID 0x1008
#define QUERY_REQ_SIGNAL_ID 0x1009
#define QUERY_CNF_SIGNAL_ID 0x100a
#define SUBSCRIBE_REQ_SIGNAL_ID 0x100b
#define SUBSCRIBE_CNF_SIGNAL_ID 0x100c
#define UNSUBSCRIBE_REQ_SIGNAL_ID 0x100d
#define UNSUBSCRIBE_CNF_SIGNAL_ID 0x100e
#define LEAVE_REQ_SIGNAL_ID 0x100f
#define LEAVE_CNF_SIGNAL_ID 0x1010
#define SUBSCRIPTION_IND_SIGNAL_ID 0x1020
#define UNSUBSCRIBE_IND_SIGNAL_ID 0x1021
#define LEAVE_IND_SIGNAL_ID 0x1022


/* User specified types */

typedef enum _sibstatus_t
{
	SSStatus_Success = 0x0000,
	SSStatus_SIB_Notification_Reset = 0x0001,
	SSStatus_SIB_Notification_Closing = 0x0002,
	SSStatus_SIB_Error = 0x0003,
	SSStatus_SIB_Error_AcessDenied = 0x0004,
	SSStatus_SIB_Failure_OutOfResources = 0x0005,
	SSStatus_SIB_Failure_NotImplemented = 0x0006,
	SSStatus_KP_Error = 0x0007,
	SSStatus_KP_Request = 0x0008,
	SSStatus_KP_Message_Incomplete = 0x0009,
	SSStatus_KP_Message_Syntax = 0x000a
} sibstatus_t;

typedef enum _encoding_t
{
	ENC_RDF_M3 = 0x0000,
	ENC_RDF_XML = 0x0001
} encoding_t;

typedef enum _query_format_t
{
	QFORMAT_RDF_XML = 0x0000,
	QFORMAT_RDF_M3 = 0x0001,
	QFORMAT_WQL_VALUES = 0x0002,
	QFORMAT_WQL_NODETYPES = 0x0003,
	QFORMAT_WQL_RELATED = 0x0004,
	QFORMAT_WQL_ISTYPE = 0x0005,
	QFORMAT_WQL_ISSUBTYPE = 0x0006,
	QFORMAT_SPARQL = 0x0007
} query_format_t;

typedef enum _confirm_t
{
	CONFIRM_NO = 0x0000,
	CONFIRM_YES = 0x0001
} confirm_t;


#endif /*  __SIBACCESSNOTA_COMMON_H___ */
