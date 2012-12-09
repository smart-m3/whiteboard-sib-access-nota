/*

  Copyright (c) 2009, Nokia Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  
    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.  
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.  
    * Neither the name of Nokia nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */
/**
 * @file sib_server.h
 * @brief SIBServer provides access to a single SIB.
 *
 * Here comes the description of SIBServer.
 *
 * Copyright 2007 Nokia Corporation
 *
 **/

#ifndef SIB_SERVER_H
#define SIB_SERVER_H

typedef struct _SIBServer SIBServer;

#include <glib.h>

#include <whiteboard_sib_access.h>

#include "sib_service.h"
#include "sib_nota_access.h"
/*****************************************************************************
 * Browse canceling related declarations
 *****************************************************************************/

/**
 * Create a new SIB server 
 *
 * @param service The SIB service that this server belongs to.
 * @param udn The UDN of the server.
 * @param name The friendly name of the server.
 * @param ip Pointer to the string containing IP address of the server.
 * @param port TCP Port number of the SIB server.
 * @return Pointer to newly created SIB server instance.
 */
SIBServer* sib_server_new(SIBService* service,
			  guchar* udn,
			  guchar * name,
			  gint sid,
			  gint port);

/**
 * Lock a SIBServer instance to prevent changes during a critical operation
 *
 * @param self The SIBServer instance to lock
 */
void sib_server_lock(SIBServer* self);

/**
 * Unlock a SIBServer instance after a critical operation
 *
 * @param self The SIBServer instance to unlock
 */
void sib_server_unlock(SIBServer* self);

/**
 * Increase the server's reference count
 *
 * @param server The server, whose reference count to increase
 */
void sib_server_ref(SIBServer* server);

/**
 * Decrease the server's reference count. If the counter reaches zero,
 * the server is destroyed.
 *
 * @param server The server, whose reference count to increase
 */
void sib_server_unref(SIBServer* server);

/**
 * Get the UDN of the given SIBServer instance
 *
 * @param self An SIBServer instance
 * @return The server's UDN (don't free the pointer!)
 */
const guchar* sib_server_get_udn(SIBServer* self);

gint sib_server_compare_udn(gconstpointer server, gconstpointer udn);

SIBAccess *sib_server_get_sib_access(SIBServer* self);

/*****************************************************************************/

/*****************************************************************************/
/* WhiteBoardSIBAccess callbacks */
/* *****************************************************************************/

void sib_server_insert_cb(WhiteBoardSIBAccess* source,
			  WhiteBoardSIBAccessHandle* handle,
			  guchar *nodeid,
			  guchar *udn,
			  gint msgnumber,
			  EncodingType encoding,
			  guchar *request,
			  gpointer userdata);


void sib_server_update_cb(WhiteBoardSIBAccess* source,
			  WhiteBoardSIBAccessHandle* handle,
			  guchar *nodeid,
			  guchar *udn,
			  gint msgnumber,
			  EncodingType encoding,			  
			  guchar *irequest,
			  guchar *rrequest,
			  gpointer userdata);

void sib_server_remove_cb(WhiteBoardSIBAccess* source,
			  WhiteBoardSIBAccessHandle* handle,
			  guchar *nodeid,
			  guchar *udn,
			  gint msgnumber,
			  EncodingType encoding,			  
			  guchar *request,
			  gpointer userdata);

void sib_server_query_cb(WhiteBoardSIBAccess* source,
			  WhiteBoardSIBAccessHandle* handle,
			 gint access_id,
			  guchar *nodeid,
			  guchar *udn,
			 gint msgnumber,
			 gint type,
			  guchar *request,
			  gpointer userdata);

void sib_server_subscribe_cb(WhiteBoardSIBAccess* source,
			     WhiteBoardSIBAccessHandle* handle,
			     gint access_id,
			     guchar *nodeid,
			     guchar *udn,
			     gint msgnumber,
			     gint type,
			     guchar *request,
			     gpointer userdata);

void sib_server_unsubscribe_cb(WhiteBoardSIBAccess* source,
			    WhiteBoardSIBAccessHandle* handle,
			       gint access_id,
			    guchar *nodeid,
			    guchar *udn,
			    gint msgnumber,
			    guchar *request,
			    gpointer userdata);

void sib_server_join_cb(WhiteBoardSIBAccess* source,
			WhiteBoardSIBAccessHandle* handle,
			gint join_id,
			guchar *nodeid,
			guchar *udn,
			gint msgnumber,
			gpointer userdata);

void sib_server_leave_cb(WhiteBoardSIBAccess* source,
			 WhiteBoardSIBAccessHandle* handle,
			 guchar *nodeid,
			 guchar *udn,
			 gint msgnumber,
			 gpointer userdata);


void sib_server_shutdown(WhiteBoardSIBAccessHandle* handle,
			 gpointer userdata);

void sib_server_send_join_complete(WhiteBoardSIBAccessHandle* handle, gint access_id, gint status);
void sib_server_send_unsubscribe_complete(WhiteBoardSIBAccessHandle* handle,
					  gint access_id,
					  gint status,
					  const guchar *subscription_id);
void sib_server_send_insert_response(WhiteBoardSIBAccessHandle* handle, gint success, const guchar *response);

void sib_server_send_update_response(WhiteBoardSIBAccessHandle* handle, gint success, const guchar *response);

void sib_server_send_remove_response(WhiteBoardSIBAccessHandle* handle, gint success, const guchar *response);
void sib_server_send_query_response(WhiteBoardSIBAccessHandle* handle,
				  gint access_id,
				  gint status,
				  const guchar *response);

void sib_server_send_subscribe_response(WhiteBoardSIBAccessHandle* handle,
					gint accessid,
					gint status,
					const guchar *subscription_id,
					const guchar *results);

void sib_server_send_subscription_indication(WhiteBoardSIBAccessHandle* handle,
					     gint access_id,
					     gint seqnum,
					     const guchar *subscriptionid,
					     const guchar *results_added,
					     const guchar *results_removed);

#endif
 
