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
/*
 * WhiteBoard SIB Access Component
 *
 * sib_server.c
 *
 * Copyright 2007 Nokia Corporation
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <glib.h>
#include <whiteboard_sib_access.h>
#include <whiteboard_log.h>

#include "serverthread.h"
#include "sib_server.h"
#include "sib_service.h"


struct _SIBServer
{
  SIBService* service;

  SIBAccess* access;
  guchar* udn;
  guchar* name;

  // Connection to whiteboard daemon
  WhiteBoardSIBAccess* whiteboard_sib_access;

  // subscription id -> WhiteboardNodeHandle
  GHashTable *subscription_map;
  
  //  GMutex* mutex;
  gint refcount;
};

/*****************************************************************************
 * Static function prototypes
 *****************************************************************************/

/**
 * Create the WhiteBoard Source that communicates with the Media Control Daemon
 *
 * @param server The server, that wishes to register itself as a source
 */
static WhiteBoardSIBAccess* sib_server_create_whiteboard_sib_access(SIBServer* server,
								    SIBService* service);

/*****************************************************************************
 * Construction/destruction
 *****************************************************************************/

SIBServer* sib_server_new(SIBService* service,
			  guchar* udn,
			  guchar *name,
			  gint sid,
			  gint port)
{
  SIBServer* server = NULL;
  SIBAccess* access = NULL;
  int count = 0;
  whiteboard_log_debug_fb();

  g_return_val_if_fail(service != NULL, NULL);
  g_return_val_if_fail(udn != NULL, NULL);

  server = g_new0(SIBServer, 1);
  
  while( (++count < 4) &&  ( access == NULL) )
    {
      access = sib_access_new( sib_service_get_controller(service),udn, sid, port) ;
      if(!access)
	{
	  usleep(100);
	  whiteboard_log_warning("Could not create SibAccess to sid:%d\n",sid);
	}
    }
  g_return_val_if_fail(access != NULL, NULL);
	
  server->service = service;
  server->access = access;
  server->udn = (guchar *)g_strdup((gchar *)udn);
  server->name =(guchar *) g_strdup((gchar *)name);
  server->whiteboard_sib_access = sib_server_create_whiteboard_sib_access(server,
									  service);

  server->refcount = 1;
  
  //  serverthread_receive(server);
  //	server->mutex = g_mutex_new();

  whiteboard_log_debug("New SIBServer created, sid: %d\n", sid);
	
  whiteboard_log_debug_fe();

  return server;
}

/**
 * Create the WhiteBoard Source that communicates with the Media Control Daemon
 *
 * @param server The server, that wishes to register itself as a source
 * @param service The UPnP service instance
 */
static WhiteBoardSIBAccess* sib_server_create_whiteboard_sib_access(SIBServer* server,
								    SIBService* service)
{
  WhiteBoardSIBAccess* whiteboard_sib_access = NULL;

  whiteboard_log_debug_fb();

  whiteboard_sib_access = WHITEBOARD_SIB_ACCESS(whiteboard_sib_access_new( server->udn, NULL, NULL,
									  server->name,
									   (guchar *)"Not yet implemented"));
  g_return_val_if_fail(whiteboard_sib_access != NULL, NULL);

  g_signal_connect(G_OBJECT(whiteboard_sib_access),
		   WHITEBOARD_SIB_ACCESS_SIGNAL_JOIN,
		   (GCallback) sib_server_join_cb,
		   service);

  g_signal_connect(G_OBJECT(whiteboard_sib_access),
		   WHITEBOARD_SIB_ACCESS_SIGNAL_LEAVE,
		   (GCallback) sib_server_leave_cb,
		   service);
	
  g_signal_connect(G_OBJECT(whiteboard_sib_access),
		   WHITEBOARD_SIB_ACCESS_SIGNAL_INSERT,
		   (GCallback) sib_server_insert_cb,
		   service);

  g_signal_connect(G_OBJECT(whiteboard_sib_access),
		   WHITEBOARD_SIB_ACCESS_SIGNAL_UPDATE,
		   (GCallback) sib_server_update_cb,		   
		   service);
		   
  g_signal_connect(G_OBJECT(whiteboard_sib_access),
		   WHITEBOARD_SIB_ACCESS_SIGNAL_REMOVE,
		   (GCallback) sib_server_remove_cb,
		   service);

  g_signal_connect(G_OBJECT(whiteboard_sib_access),
		   WHITEBOARD_SIB_ACCESS_SIGNAL_QUERY,
		   (GCallback) sib_server_query_cb,
		   service);

  g_signal_connect(G_OBJECT(whiteboard_sib_access),
		   WHITEBOARD_SIB_ACCESS_SIGNAL_SUBSCRIBE,
		   (GCallback) sib_server_subscribe_cb,
		   service);
	
  g_signal_connect(G_OBJECT(whiteboard_sib_access),
		   WHITEBOARD_SIB_ACCESS_SIGNAL_UNSUBSCRIBE,
		   (GCallback) sib_server_unsubscribe_cb,
		   service);
	
  whiteboard_log_debug_fe();

  return whiteboard_sib_access;
}

/**
 * Destroy a UPnP server struct
 *
 * @param server The server struct to destroy
 * @return FALSE for error, TRUE for success.
 */
static gboolean sib_server_destroy(SIBServer* server)
{
  whiteboard_log_debug_fb();

  g_return_val_if_fail(server != NULL, FALSE);
  //	g_return_val_if_fail(server->mutex != NULL, FALSE);

  
  /* Free the UDN string */
  if (server->udn)
    g_free(server->udn);
  server->udn = NULL;
	
  if(server->access)
    {
      sib_access_unref(server->access);
      server->access = NULL;
    }

  if(server->name)
    g_free(server->name);
	
  /* Destroy the Whiteboard_Sib_Access instance */
  if (server->whiteboard_sib_access)
    g_object_unref(server->whiteboard_sib_access);
	
  server->whiteboard_sib_access = NULL;

  /* Destroy the mutex */
  //g_mutex_unlock(server->mutex);
  //g_mutex_free(server->mutex);
  //server->mutex = NULL;
	
  /* Finally destroy self */
  g_free(server);
	
  whiteboard_log_debug_fe();

  return TRUE;
}

/**
 * Increase the server's reference count
 *
 * @param server The server, whose reference count to increase
 */
void sib_server_ref(SIBServer* server)
{
  whiteboard_log_debug_fb();

  g_return_if_fail(server != NULL);

  if (g_atomic_int_get(&server->refcount) > 0)
    {
      /* Refcount is something sensible */
      g_atomic_int_inc(&server->refcount);

      whiteboard_log_debug("Server (%s) refcount: %d\n",
			   server->udn, server->refcount);
    }
  else
    {
      /* Refcount is already zero */
      whiteboard_log_debug("Server (%s) refcount already %d!\n",
			   server->udn, server->refcount);
    }

  whiteboard_log_debug_fe();
}

/**
 * Decrease the server's reference count. If the counter reaches zero,
 * the server is destroyed.
 *
 * @param server The server, whose reference count to increase
 */
void sib_server_unref(SIBServer* server)
{
  whiteboard_log_debug_fb();

  g_return_if_fail(server != NULL);

  if (g_atomic_int_dec_and_test(&server->refcount) == FALSE)
    {
      whiteboard_log_debug("Server (%s) refcount dec: %d\n", 
			   server->udn, server->refcount);
    }
  else
    {
      whiteboard_log_debug("Server %s) refcount zeroed. Destroy.\n",
			   server->udn);
      sib_server_destroy(server);
    }

  whiteboard_log_debug_fe();
}

void sib_server_lock(SIBServer* self)
{
  //	g_mutex_lock(self->mutex);
}

void sib_server_unlock(SIBServer* self)
{
  //g_mutex_unlock(self->mutex);
}

/**
 * Get the UDN of the given SIBServer instance
 *
 * @param self An SIBServer instance
 * @return The server's UDN (don't free the pointer!)
 */
const guchar* sib_server_get_udn(SIBServer* self)
{
  g_return_val_if_fail(self != NULL, NULL);
  return (const guchar*) self->udn;
}

/**
 * Compare the given SIBServer pointer with the given UDN.
 *
 * @param server The SIBServer to compare with
 * @param udn The UDN to try and match
 * @return 0 if server and udn match, !0 if they don't match
 */
gint sib_server_compare_udn(gconstpointer server, gconstpointer udn)
{
  SIBServer* srv = (SIBServer*) server;
  
  g_return_val_if_fail(srv != NULL, 1);
  g_return_val_if_fail(udn != NULL, -1);
  
  return g_ascii_strcasecmp( (gchar *)srv->udn, (gchar *)udn);
}

SIBAccess *sib_server_get_sib_access(SIBServer* self)
{
  g_return_val_if_fail(self != NULL, NULL);
  return self->access;
}


void sib_server_join_cb(WhiteBoardSIBAccess* source,
			WhiteBoardSIBAccessHandle* handle,
			gint join_id,
			guchar *nodeid,
			guchar *udn,
			gint msgnumber,
			gpointer userdata)
{
  g_return_if_fail(handle != NULL);
  g_return_if_fail(userdata != NULL);
  
  SIBServer* server = NULL;
  SIBService* service = NULL;
  WhiteBoardSIBAccess*  context = whiteboard_sib_access_handle_get_context(handle);
  g_return_if_fail(context != NULL);
  
  const guchar* serviceid = (guchar *)whiteboard_sib_access_get_uuid(context);
  
  whiteboard_log_debug_fb();

  service = (SIBService*) userdata;
  g_return_if_fail(service != NULL);

  whiteboard_log_debug("service: %s \n",serviceid);
  whiteboard_log_debug("udn: %s \n",udn);
  /* Acquire the server reference inside service lock so that the
     server doesn't disappear during the operation. This could
     happen if the server sends a byebye message. */
  sib_service_lock(service);
  server = sib_service_find_server(service, serviceid );
  sib_server_ref(server);
  sib_service_unlock(service);

  serverthread_join(server, handle, join_id, nodeid, udn, msgnumber);
  sib_server_unref(server);

  whiteboard_log_debug_fe();
}


void sib_server_leave_cb(WhiteBoardSIBAccess* source,
			 WhiteBoardSIBAccessHandle* handle,
			 guchar *nodeid,
			 guchar *udn,
			 gint msgnumber,
			 gpointer userdata)
{
  g_return_if_fail(handle != NULL);
  WhiteBoardSIBAccess*   context = whiteboard_sib_access_handle_get_context(handle);
  g_return_if_fail(context != NULL);
  const guchar *serviceid = (guchar *)whiteboard_sib_access_get_uuid(context);
  
  SIBServer* server = NULL;
  SIBService* service = NULL;

  whiteboard_log_debug_fb();
  
  g_return_if_fail(userdata != NULL);
  service = (SIBService*) userdata;
  g_return_if_fail(service != NULL);

  whiteboard_log_debug("service: %s \n",serviceid);
  whiteboard_log_debug("udn: %s \n",udn);
  /* Acquire the server reference inside service lock so that the
     server doesn't disappear during the operation. This could
     happen if the server sends a byebye message. */
  sib_service_lock(service);
  server = sib_service_find_server(service, serviceid );
  sib_server_ref(server);
  sib_service_unlock(service);

  serverthread_leave(server, handle, nodeid, udn, msgnumber);
  sib_server_unref(server);

  whiteboard_log_debug_fe();
}

void sib_server_insert_cb(WhiteBoardSIBAccess* source,
			  WhiteBoardSIBAccessHandle* handle,
			  guchar *nodeid,
			  guchar *siburi,
			  gint msgnumber,
			  EncodingType encoding,
			  guchar* request,
			  gpointer userdata)
{
  g_return_if_fail(handle != NULL);
  g_return_if_fail(nodeid != NULL);
  g_return_if_fail(siburi != NULL);
  g_return_if_fail(request != NULL);
  g_return_if_fail(userdata != NULL);

  
  SIBServer* server = NULL;
  SIBService* service = NULL;

  whiteboard_log_debug_fb();
  
  service = (SIBService*) userdata;
  g_return_if_fail(service != NULL);
  

  /* Acquire the server reference inside service lock so that the
     server doesn't disappear during the operation. This could
     happen if the server sends a byebye message. */
  sib_service_lock(service);
  server = sib_service_find_server(service, siburi );
  sib_server_ref(server);
  sib_service_unlock(service);
  
  serverthread_insert(server, handle, nodeid, siburi, msgnumber, encoding, request);
  sib_server_unref(server);
  
  
  whiteboard_log_debug_fe();
}

void sib_server_update_cb(WhiteBoardSIBAccess* source,
			  WhiteBoardSIBAccessHandle* handle,
			  guchar *nodeid,
			  guchar *siburi,
			  gint msgnumber,
			  EncodingType encoding,
			  guchar *insert_request,
			  guchar *remove_request,
			  gpointer userdata)
{
  g_return_if_fail(handle != NULL);
  g_return_if_fail(nodeid != NULL);
  g_return_if_fail(siburi != NULL);
  g_return_if_fail(insert_request != NULL);
  g_return_if_fail(remove_request != NULL);
  g_return_if_fail(userdata != NULL);

  
  SIBServer* server = NULL;
  SIBService* service = NULL;

  whiteboard_log_debug_fb();
  
  service = (SIBService*) userdata;
  g_return_if_fail(service != NULL);
  

  /* Acquire the server reference inside service lock so that the
     server doesn't disappear during the operation. This could
     happen if the server sends a byebye message. */
  sib_service_lock(service);
  server = sib_service_find_server(service, siburi );
  sib_server_ref(server);
  sib_service_unlock(service);
  
  serverthread_update(server, handle, nodeid, siburi, msgnumber, encoding, insert_request, remove_request);
  sib_server_unref(server);
  
  
  whiteboard_log_debug_fe();
}

void sib_server_remove_cb(WhiteBoardSIBAccess* source,
			  WhiteBoardSIBAccessHandle* handle,
			  guchar *nodeid,
			  guchar *siburi,
			  gint msgnumber,
			  EncodingType encoding,
			  guchar* request,
			  gpointer userdata)
{
  g_return_if_fail(handle != NULL);
  g_return_if_fail(nodeid != NULL);
  g_return_if_fail(siburi != NULL);
  g_return_if_fail(request != NULL);
  g_return_if_fail(userdata != NULL);

  SIBServer* server = NULL;
  SIBService* service = NULL;

  whiteboard_log_debug_fb();
  
  service = (SIBService*) userdata;
  g_return_if_fail(service != NULL);
  

  /* Acquire the server reference inside service lock so that the
     server doesn't disappear during the operation. This could
     happen if the server sends a byebye message. */
  sib_service_lock(service);
  server = sib_service_find_server(service, siburi );
  sib_server_ref(server);
  sib_service_unlock(service);
  
  serverthread_remove(server, handle, nodeid, siburi, msgnumber, encoding, request);
  sib_server_unref(server);
  
  whiteboard_log_debug_fe();
}

void sib_server_query_cb(WhiteBoardSIBAccess* source,
			 WhiteBoardSIBAccessHandle* handle,
			 gint access_id,
			 guchar *nodeid,
			 guchar *siburi,
			 gint msgnumber,
			 gint type,
			 guchar* request,
			 gpointer userdata)
{
  g_return_if_fail(handle != NULL);
  g_return_if_fail(nodeid != NULL);
  g_return_if_fail(siburi != NULL);
  g_return_if_fail(request != NULL);
  g_return_if_fail(userdata != NULL);
  
  SIBServer* server = NULL;
  SIBService* service = NULL;
  
  whiteboard_log_debug_fb();
  
  service = (SIBService*) userdata;
  g_return_if_fail(service != NULL);
  
  
  /* Acquire the server reference inside service lock so that the
     server doesn't disappear during the operation. This could
     happen if the server sends a byebye message. */
  sib_service_lock(service);
  server = sib_service_find_server(service, siburi );
  sib_server_ref(server);
  sib_service_unlock(service);
  
  serverthread_query(server, handle, access_id,nodeid, siburi, msgnumber, type, request);
  sib_server_unref(server);
  
  whiteboard_log_debug_fe();
}

void sib_server_subscribe_cb(WhiteBoardSIBAccess* source,
			     WhiteBoardSIBAccessHandle* handle,
			     gint access_id,
			     guchar *nodeid,
			     guchar *siburi,
			     gint msgnumber,
			     gint type,
			     guchar* request,
			     gpointer userdata)
{
  g_return_if_fail(handle != NULL);
  g_return_if_fail(nodeid != NULL);
  g_return_if_fail(siburi != NULL);
  g_return_if_fail(request != NULL);
  g_return_if_fail(userdata != NULL);
  
  SIBServer* server = NULL;
  SIBService* service = NULL;
  
  whiteboard_log_debug_fb();
  
  service = (SIBService*) userdata;
  g_return_if_fail(service != NULL);
  
  
  /* Acquire the server reference inside service lock so that the
     server doesn't disappear during the operation. This could
     happen if the server sends a byebye message. */
  sib_service_lock(service);
  server = sib_service_find_server(service, siburi );
  sib_server_ref(server);
  sib_service_unlock(service);
  
  serverthread_subscribe(server, handle, access_id, nodeid, siburi, msgnumber, type, request);
  sib_server_unref(server);
  
  whiteboard_log_debug_fe();
}

void sib_server_unsubscribe_cb(WhiteBoardSIBAccess* source,
			       WhiteBoardSIBAccessHandle* handle,
			       gint access_id,
			       guchar *nodeid,
			       guchar *siburi,
			       gint msgnumber,
			       guchar* request,
			       gpointer userdata)
{
  g_return_if_fail(handle != NULL);
  g_return_if_fail(nodeid != NULL);
  g_return_if_fail(siburi != NULL);
  g_return_if_fail(request != NULL);
  g_return_if_fail(userdata != NULL);
  
  SIBServer* server = NULL;
  SIBService* service = NULL;
  
  whiteboard_log_debug_fb();
  
  service = (SIBService*) userdata;
  g_return_if_fail(service != NULL);
  
  
  /* Acquire the server reference inside service lock so that the
     server doesn't disappear during the operation. This could
     happen if the server sends a byebye message. */
  sib_service_lock(service);
  server = sib_service_find_server(service, siburi );
  sib_server_ref(server);
  sib_service_unlock(service);
  
  serverthread_unsubscribe(server, handle, access_id,nodeid, siburi, msgnumber, request);
  sib_server_unref(server);
  
  whiteboard_log_debug_fe();
}

void sib_server_send_join_complete(WhiteBoardSIBAccessHandle* handle, gint access_id, gint status)
{
  //WhiteBoardSIBAccess *node= NULL;
  
  whiteboard_log_debug_fb();
  g_return_if_fail(handle != NULL);
  
  //  node = whiteboard_sib_access_handle_get_context(handle);
  //g_return_if_fail(node != NULL);
  
  whiteboard_sib_access_send_join_complete(handle,access_id, status);
  whiteboard_log_debug_fe();
}

void sib_server_send_insert_response(WhiteBoardSIBAccessHandle* handle,
				     gint success,
				     const guchar *response)
{
  whiteboard_log_debug_fb();
  g_return_if_fail(handle!=NULL);
  g_return_if_fail(response!=NULL);
  whiteboard_sib_access_send_insert_response(handle, success, response);
  whiteboard_log_debug_fe();
}

void sib_server_send_update_response(WhiteBoardSIBAccessHandle* handle,
                                     gint success,
                                     const guchar *response)
{
  whiteboard_log_debug_fb();
  g_return_if_fail(handle!=NULL);
  g_return_if_fail(response!=NULL);
  whiteboard_sib_access_send_update_response(handle, success, response);
  whiteboard_log_debug_fe();
}



void sib_server_send_remove_response(WhiteBoardSIBAccessHandle* handle,
				     gint success,
				     const guchar *response)
{
  whiteboard_log_debug_fb();
  g_return_if_fail(handle!=NULL);
  whiteboard_sib_access_send_remove_response(handle, success, response);
  whiteboard_log_debug_fe();
}

void sib_server_send_query_response(WhiteBoardSIBAccessHandle* handle,
				    gint access_id,
				    gint status,
				    const guchar *response)
{
  whiteboard_log_debug_fb();
  g_return_if_fail(handle!=NULL);
  g_return_if_fail(response!=NULL);
  whiteboard_sib_access_send_query_response(handle, access_id, status, response);
  whiteboard_log_debug_fe();
}


void sib_server_send_subscribe_response(WhiteBoardSIBAccessHandle* handle,
					gint accessid,
					gint status,
					const guchar *subscription_id,
					const guchar *results)
{
  whiteboard_log_debug_fb();
  g_return_if_fail(handle!=NULL);
  g_return_if_fail(subscription_id!=NULL);
  g_return_if_fail(results!=NULL);
  whiteboard_sib_access_send_subscribe_response(handle, accessid, status, subscription_id, results);
  whiteboard_log_debug_fe();
}

void sib_server_send_subscription_indication(WhiteBoardSIBAccessHandle* handle,
					     gint access_id, 
					     gint seqnum,
					     const guchar *subscriptionid,
					     const guchar *results_added,
					     const guchar *results_removed)
{
  whiteboard_log_debug_fb();
  g_return_if_fail(handle!=NULL);
  g_return_if_fail(subscriptionid!=NULL);
  g_return_if_fail(results_added!=NULL);
  g_return_if_fail(results_removed!=NULL);
   
  whiteboard_sib_access_send_subscription_indication(handle, access_id, seqnum, subscriptionid, results_added, results_removed);
  whiteboard_log_debug_fe();
}

void sib_server_send_unsubscribe_complete(WhiteBoardSIBAccessHandle* handle, gint access_id, gint status, const guchar *subscription_id)
{
  whiteboard_log_debug_fb();
  g_return_if_fail(handle!=NULL);
  g_return_if_fail(subscription_id!=NULL);
  whiteboard_log_debug("unsubscribe_complete: access_id: %d, status: %d, subscription_id: %s\n", access_id, status, subscription_id);
  whiteboard_sib_access_send_unsubscribe_complete(handle, access_id, status, subscription_id);
  whiteboard_log_debug_fe();
}

