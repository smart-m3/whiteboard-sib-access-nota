/*
 * SibAccessNota_user.h
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


#ifndef __SIBACCESSNOTA_USER_H___
#define __SIBACCESSNOTA_USER_H___


/* include H_IF BSDAPI definitions */
#include <h_in/h_bsdapi.h>


/* Include general support code */
#include <stubgen/hin3_stubadapter_if.h>


#include "SibAccessNota_common.h"

/* These functions are for the user */
/* This function can be used to attach new socket. Library will make a copy of context pointer.*/
int SibAccessNota_user_new_connection(struct context_pointer* context);
/* This function can be used to detach socket. */
int SibAccessNota_user_remove_connection(struct context_pointer* context);


/* these hooks must be implemented by user. */
/* This function is called when connection was closed */
void SibAccessNota_user_handler_disconnected(struct context_pointer* context);


/* This function is called when there was out-of-memory or other not so critical error (connection is still usable) */
void SibAccessNota_user_handler_error(struct context_pointer* context, int error_type);


/* Stubs that can be used to send messages */

int SibAccessNota_Join_req( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, uint8_t* credentials, uint16_t credentials_len );

int SibAccessNota_Insert_req( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, encoding_t encoding, uint8_t* insert_graph, uint16_t insert_graph_len, confirm_t confirm );

int SibAccessNota_Remove_req( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, encoding_t encoding, uint8_t* remove_graph, uint16_t remove_graph_len, confirm_t confirm );

int SibAccessNota_Update_req( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, encoding_t encoding, uint8_t* insert_graph, uint16_t insert_graph_len, uint8_t* remove_graph, uint16_t remove_graph_len, confirm_t confirm );

int SibAccessNota_Query_req( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, query_format_t format, uint8_t* query, uint16_t query_len, confirm_t confirm );

int SibAccessNota_Subscribe_req( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, query_format_t format, uint8_t* query, uint16_t query_len, confirm_t results );

int SibAccessNota_Unsubscribe_req( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, uint8_t* subscription_id, uint16_t subscription_id_len );

int SibAccessNota_Leave_req( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum );



/* Skeleton prototypes; user must implement them. */
void SibAccessNota_Join_cnf_process( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, sibstatus_t status, uint8_t* credentials, uint16_t credentials_len );
void SibAccessNota_Insert_cnf_process( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, sibstatus_t status, uint8_t* bNodes, uint16_t bNodes_len );
void SibAccessNota_Remove_cnf_process( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, sibstatus_t status );
void SibAccessNota_Update_cnf_process( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, sibstatus_t status, uint8_t* bNodes, uint16_t bNodes_len );
void SibAccessNota_Query_cnf_process( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, sibstatus_t status, uint8_t* results, uint16_t results_len );
void SibAccessNota_Subscribe_cnf_process( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, sibstatus_t status, uint8_t* subscription_id, uint16_t subscription_id_len, uint8_t* results, uint16_t results_len );
void SibAccessNota_Unsubscribe_cnf_process( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, sibstatus_t status );
void SibAccessNota_Leave_cnf_process( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, sibstatus_t status );
void SibAccessNota_Subscription_ind_process( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, uint32_t seqnum, uint8_t* subscription_id, uint16_t subscription_id_len, uint8_t* new_results, uint16_t new_results_len, uint8_t* obsolete_results, uint16_t obsolete_results_len );
void SibAccessNota_Unsubscribe_ind_process( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, sibstatus_t status, uint8_t* subscription_id, uint16_t subscription_id_len );
void SibAccessNota_Leave_ind_process( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, sibstatus_t status );


#endif /*  __SIBACCESSNOTA_USER_H___ */