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
 *
 * @file serverthread.c
 *
 * Copyright 2007 Nokia Corporation
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <glib.h>
#include <string.h>

#include <whiteboard_log.h>
#include <whiteboard_util.h>
#include <sibmsg.h>

#include "serverthread.h"
#include "sib_server.h"
#include "sib_service.h"
#include "sib_nota_access.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/** Number of items to get on a single UPnP browse pass */
#define INC_COUNT 20
#define BACKLOG 5

/** Maximum number of browse/metadata threads */
#define SERVERTHREAD_MAX_THREADS 10
#define NODEPORT 10011
/** The thread pool object */
static GThreadPool* serverthread_pool = NULL;

/*****************************************************************************
 * Type definitions
 *****************************************************************************/

/** Server thread actions */
typedef enum _ServerThreadAction
  {
    ServerThreadActionJoin,
    ServerThreadActionLeave,
    ServerThreadActionInsert,
    ServerThreadActionUpdate,
    ServerThreadActionRemove,
    ServerThreadActionQuery,
    ServerThreadActionSubscribe,
    ServerThreadActionUnsubscribe,
  } ServerThreadAction;

/** Server thread action arguments */
typedef struct _ServerThreadArgs
{
  ServerThreadAction action;
  
  SIBServer* server;
  WhiteBoardSIBAccessHandle* handle;
  gint access_id;
  guchar* itemid;

  guchar *nodeid;
  gint msgnumber;
  //	WhiteBoardMetadataRequest* request;
  gint q_type;
  guchar *insert_request;
  guchar *remove_request; /* used only with update */
  EncodingType encoding;
  //  gint start;
  //gint count;
  //int sockfd;
  
} ServerThreadArgs;



/*****************************************************************************
 * Static function declarations
 *****************************************************************************/

/**
 * The worker thread for CDS server browsing.
 *
 * @param data ServerTest1Args*
 * @param user_data SIBServer*
 */
static void serverthread(gpointer data, gpointer user_data);
//static gpointer serverthread_receive_thread( gpointer userdata);

static void serverthread_join_thread(SIBService* service, 
				     SIBServer* server,
				     WhiteBoardSIBAccessHandle* handle,
				     gint access_id,
				     guchar *nodeid,
				     guchar *itemid,
				     gint msgnumber);

static void serverthread_leave_thread(SIBService* service,
				      SIBServer* server,
				      WhiteBoardSIBAccessHandle* handle,
				      guchar *nodeid,
				      guchar *udn,
				      gint msgnumber);

static void serverthread_insert_thread(SIBService* service,
				       SIBServer* server,
				       WhiteBoardSIBAccessHandle* handle,
				       guchar *nodeid,
				       guchar *sibid,
				       gint msgnumber,
				       EncodingType encoding,
				       guchar *request);

static void serverthread_update_thread(SIBService* service,
				       SIBServer* server,
				       WhiteBoardSIBAccessHandle* handle,
				       guchar *nodeid,
				       guchar *sibid,
				       gint msgnumber,
				       EncodingType encoding,
				       guchar *irequest,
				       guchar *rrequest);

static void serverthread_remove_thread(SIBService* service,
				       SIBServer* server,
				       WhiteBoardSIBAccessHandle* handle,
				       guchar *nodeid,
				       guchar *sibid,
				       gint msgnumber,
				       EncodingType encoding,
				       guchar *request);

static void serverthread_query_thread(SIBService* service,
				      SIBServer* server,
				      WhiteBoardSIBAccessHandle* handle,
				      gint access_id,
				      guchar *nodeid,
				      guchar *sibid,
				      gint msgnumber,
				      gint type,
				      guchar *request);

static void serverthread_subscribe_thread(SIBService* service,
					  SIBServer* server,
					  WhiteBoardSIBAccessHandle* handle,
					  gint access_id,
					  guchar *nodeid,
					  guchar *sibid,
					  gint msgnumber,
					  gint type,
					  guchar *request);

static void serverthread_unsubscribe_thread(SIBService* service,
					    SIBServer* server,
					    WhiteBoardSIBAccessHandle* handle,
					    gint access_id,
					    guchar *nodeid,
					    guchar *sibid,
					    gint msgnumber,
					    guchar *request);



/*****************************************************************************
 * Public function implementations
 *****************************************************************************/

gint serverthread_init(SIBService* service)
{
  g_return_val_if_fail(service != NULL, FALSE);
	
  /* Create a server thread pool */
  serverthread_pool = g_thread_pool_new(serverthread,
					service,
					SERVERTHREAD_MAX_THREADS,
					FALSE,
					NULL);
	
  g_return_val_if_fail(serverthread_pool != NULL, FALSE);

  return TRUE;
}
#if 0
gint serverthread_receive(SIBServer *server)
{
  ServerThreadArgs* sta = NULL;
  GThread *rt = NULL;
  GError *err = NULL;
  whiteboard_log_debug_fb();
  
  g_return_val_if_fail(server != NULL, -1);
  sta = g_new0(ServerThreadArgs, 1);
  //  sta->action = ServerThreadActionReceive;
  sta->server = server;
  //  g_thread_pool_push(serverthread_pool, sta, NULL);
  rt = g_thread_create( serverthread_receive_thread, sta, FALSE, &err);

  if(rt == NULL)
    {
      whiteboard_log_debug("Could not create receiver thread: %s", err->message);
      g_error_free(err);
      
    }

  whiteboard_log_debug_fe();
  return (rt == NULL ? -1: 0);
}
#endif

gint serverthread_join(SIBServer* server,
		       WhiteBoardSIBAccessHandle* handle,
		       gint access_id,
		       guchar *nodeid,
		       guchar* udn,
		       gint msgnumber )
{
  ServerThreadArgs* sta = NULL;

  whiteboard_log_debug_fb();
	
  g_return_val_if_fail(server != NULL, -1);
  g_return_val_if_fail(handle != NULL, -1);
  /* 	g_return_val_if_fail(itemid != NULL, -1); */

  sib_server_ref(server);
  whiteboard_sib_access_handle_ref(handle);

  sta = g_new0(ServerThreadArgs, 1);
  sta->action = ServerThreadActionJoin;
  sta->server = server;
  sta->itemid = (guchar *) g_strdup((gchar *)udn);
  sta->nodeid = (guchar *) g_strdup((gchar *)nodeid);
  /* 	sta->start = start; */
  /* 	sta->count = count; */
  sta->handle = handle; 
  sta->access_id = access_id; 
  sta->msgnumber = msgnumber;
  g_thread_pool_push(serverthread_pool, sta, NULL);

  whiteboard_log_debug_fe();

  return 0;
}


gint serverthread_leave(SIBServer* server,
			WhiteBoardSIBAccessHandle* handle,
			guchar *nodeid,
			guchar *udn,
			gint msgnumber)
{
  ServerThreadArgs* sta = NULL;

  whiteboard_log_debug_fb();
	
  g_return_val_if_fail(server != NULL, -1);
  g_return_val_if_fail(handle != NULL, -1);
  /* 	g_return_val_if_fail(itemid != NULL, -1); */

  sib_server_ref(server);
  whiteboard_sib_access_handle_ref(handle);

  sta = g_new0(ServerThreadArgs, 1);
  sta->action = ServerThreadActionLeave;
  sta->server = server;
  sta->itemid = (guchar *) g_strdup((gchar *)udn);
  sta->nodeid = (guchar *) g_strdup((gchar *)nodeid);
  /* 	sta->start = start; */
  /* 	sta->count = count; */
  sta->handle = handle; 
  sta->msgnumber = msgnumber;
  g_thread_pool_push(serverthread_pool, sta, NULL);

  whiteboard_log_debug_fe();

  return 0;
}



gint serverthread_insert(SIBServer* server,
			 WhiteBoardSIBAccessHandle* handle,
			 guchar *nodeid,
			 guchar *sibid,
			 gint msgnumber,
			 EncodingType encoding,
			 guchar *request)
{
  ServerThreadArgs* sta = NULL;

  whiteboard_log_debug_fb();
	
  g_return_val_if_fail(server != NULL, -1);
  g_return_val_if_fail(request != NULL, -1);
	
  /* 	g_return_val_if_fail(itemid != NULL, -1); */

  sib_server_ref(server);
	
  sta = g_new0(ServerThreadArgs, 1);
  sta->action = ServerThreadActionInsert;
  sta->server = server;
  sta->insert_request = (guchar *) g_strdup((gchar *)request);
  sta->itemid = (guchar *) g_strdup((gchar *)sibid);
  sta->nodeid = (guchar *) g_strdup((gchar *)nodeid);
  sta->msgnumber = msgnumber;
  sta->handle = handle;
  sta->encoding = encoding;
  /* 	sta->browse_id = browse_id; */
  whiteboard_sib_access_handle_ref(sta->handle);
  g_thread_pool_push(serverthread_pool, sta, NULL);

  whiteboard_log_debug_fe();

  return 0;
}

gint serverthread_update(SIBServer* server,
                         WhiteBoardSIBAccessHandle* handle,
                         guchar *nodeid,
                         guchar *sibid,
                         gint msgnumber,
			 EncodingType encoding,
                         guchar *insert_request,
                         guchar *remove_request)
{
    ServerThreadArgs* sta = NULL;

  whiteboard_log_debug_fb();
        
  g_return_val_if_fail(server != NULL, -1);
  g_return_val_if_fail(insert_request != NULL, -1);
  g_return_val_if_fail(remove_request != NULL, -1);
  
  /*    g_return_val_if_fail(itemid != NULL, -1); */

  sib_server_ref(server);
        
  sta = g_new0(ServerThreadArgs, 1);
  sta->action = ServerThreadActionUpdate;
  sta->server = server;
  sta->insert_request = (guchar *) g_strdup((gchar *)insert_request);
  sta->remove_request = (guchar *) g_strdup((gchar *)remove_request);
  sta->encoding = encoding;
  sta->itemid = (guchar *) g_strdup((gchar *)sibid);
  sta->nodeid = (guchar *) g_strdup((gchar *)nodeid);
  sta->msgnumber = msgnumber;
  sta->handle = handle;
  /*    sta->browse_id = browse_id; */
  whiteboard_sib_access_handle_ref(sta->handle);
  g_thread_pool_push(serverthread_pool, sta, NULL);

  whiteboard_log_debug_fe();

  return 0;
}


gint serverthread_remove(SIBServer* server,
			 WhiteBoardSIBAccessHandle* handle,
			 guchar *nodeid,
			 guchar *sibid,
			 gint msgnumber,
			 EncodingType encoding,
			 guchar *request)
{
  ServerThreadArgs* sta = NULL;

  whiteboard_log_debug_fb();
	
  g_return_val_if_fail(server != NULL, -1);
  g_return_val_if_fail(request != NULL, -1);
	
  /* 	g_return_val_if_fail(itemid != NULL, -1); */

  sib_server_ref(server);
	
  sta = g_new0(ServerThreadArgs, 1);
  sta->action = ServerThreadActionRemove;
  sta->server = server;
  sta->insert_request = (guchar *) g_strdup((gchar *)request);
  sta->encoding = encoding;
  sta->itemid = (guchar *) g_strdup((gchar *)sibid);
  sta->nodeid = (guchar *) g_strdup((gchar *)nodeid);
  sta->msgnumber = msgnumber;
  sta->encoding = encoding;
  sta->handle = handle;
  /* 	sta->browse_id = browse_id; */
  whiteboard_sib_access_handle_ref(sta->handle);
  g_thread_pool_push(serverthread_pool, sta, NULL);

  whiteboard_log_debug_fe();

  return 0;
}

gint serverthread_query(SIBServer* server,
			WhiteBoardSIBAccessHandle* handle,
			gint access_id,
			guchar *nodeid,
			guchar *sibid,
			gint msgnumber,
			gint type,
			guchar *request)
{
  ServerThreadArgs* sta = NULL;

  whiteboard_log_debug_fb();
	
  g_return_val_if_fail(server != NULL, -1);
  g_return_val_if_fail(request != NULL, -1);
	
  /* 	g_return_val_if_fail(itemid != NULL, -1); */

  sib_server_ref(server);
	
  sta = g_new0(ServerThreadArgs, 1);
  sta->action = ServerThreadActionQuery;
  sta->server = server;
  sta->insert_request = (guchar *) g_strdup((gchar *)request);
  sta->itemid = (guchar *) g_strdup((gchar *)sibid);
  sta->nodeid = (guchar *) g_strdup((gchar *)nodeid);
  sta->handle = handle;
  sta->msgnumber = msgnumber;
  sta->access_id = access_id;
  sta->q_type = type;
  whiteboard_sib_access_handle_ref(sta->handle);
  g_thread_pool_push(serverthread_pool, sta, NULL);

  whiteboard_log_debug_fe();

  return 0;
}


gint serverthread_subscribe(SIBServer* server,
			    WhiteBoardSIBAccessHandle* handle,
			    gint access_id,
			    guchar *nodeid,
			    guchar *sibid,
			    gint msgnumber,
			    gint type,
			    guchar *request)
{
  ServerThreadArgs* sta = NULL;

  whiteboard_log_debug_fb();
	
  g_return_val_if_fail(server != NULL, -1);
  g_return_val_if_fail(request != NULL, -1);
	
  /* 	g_return_val_if_fail(itemid != NULL, -1); */

  sib_server_ref(server);
	
  sta = g_new0(ServerThreadArgs, 1);
  sta->action = ServerThreadActionSubscribe;
  sta->server = server;
  sta->insert_request = (guchar *) g_strdup((gchar *)request);
  sta->itemid = (guchar *) g_strdup((gchar *)sibid);
  sta->nodeid = (guchar *) g_strdup((gchar *)nodeid);
  sta->handle = handle;
  sta->msgnumber = msgnumber;
  sta->access_id = access_id;
  sta->q_type = type;
  whiteboard_sib_access_handle_ref(sta->handle);
  g_thread_pool_push(serverthread_pool, sta, NULL);

  whiteboard_log_debug_fe();

  return 0;
}
gint serverthread_unsubscribe(SIBServer* server,
			      WhiteBoardSIBAccessHandle* handle,
			      gint access_id,
			      guchar *nodeid,
			      guchar *sibid,
			      gint msgnumber,
			      guchar *request)
{
  ServerThreadArgs* sta = NULL;

  whiteboard_log_debug_fb();
	
  g_return_val_if_fail(server != NULL, -1);
  g_return_val_if_fail(request != NULL, -1);
	
  /* 	g_return_val_if_fail(itemid != NULL, -1); */

  sib_server_ref(server);
	
  sta = g_new0(ServerThreadArgs, 1);
  sta->action = ServerThreadActionUnsubscribe;
  sta->server = server;
  sta->insert_request = (guchar *) g_strdup((gchar *)request);
  sta->itemid = (guchar *) g_strdup((gchar *)sibid);
  sta->nodeid = (guchar *) g_strdup((gchar *)nodeid);
  sta->handle = handle;
  sta->msgnumber = msgnumber;
  sta->access_id = access_id;
  whiteboard_sib_access_handle_ref(sta->handle);
  g_thread_pool_push(serverthread_pool, sta, NULL);

  whiteboard_log_debug_fe();

  return 0;
}


/*****************************************************************************
 * Static thread functions
 *****************************************************************************/

static void serverthread(gpointer data, gpointer user_data)
{
  ServerThreadArgs* sta = NULL;
  SIBService* service = NULL;
	
  whiteboard_log_debug_fb();
	
  service = (SIBService*) user_data;
  g_return_if_fail(service != NULL);

  sta = (ServerThreadArgs*) data;
  g_return_if_fail(sta != NULL);

  switch (sta->action)
    {
	  	  
    case ServerThreadActionJoin:
      serverthread_join_thread(service,
			       sta->server,
			       sta->handle,
			       sta->access_id,
			       sta->nodeid,
			       sta->itemid,
			       sta->msgnumber);
      break;

    case ServerThreadActionLeave:
      serverthread_leave_thread(service,
				sta->server,
				sta->handle,
				sta->nodeid,
				sta->itemid,
				sta->msgnumber);
      break;

    case ServerThreadActionInsert:
      serverthread_insert_thread(service,
				 sta->server,
				 sta->handle,
				 sta->nodeid,
				 sta->itemid,
				 sta->msgnumber,
				 sta->encoding,
				 sta->insert_request);
      break;
      
  case ServerThreadActionUpdate:
      serverthread_update_thread(service,
                                 sta->server,
                                 sta->handle,
                                 sta->nodeid,
                                 sta->itemid,
                                 sta->msgnumber,
				 sta->encoding,
                                 sta->insert_request,
                                 sta->remove_request);
      break;
      
    case ServerThreadActionRemove:
      serverthread_remove_thread(service,
				 sta->server,
				 sta->handle,
				 sta->nodeid,
				 sta->itemid,
				 sta->msgnumber,
				 sta->encoding,
				 sta->insert_request);
      break;
		
    case ServerThreadActionQuery:
      serverthread_query_thread(service,
				sta->server,
				sta->handle,
				sta->access_id,
				sta->nodeid,
				sta->itemid,
				sta->msgnumber,
				sta->q_type,
				sta->insert_request);
      break;

    case ServerThreadActionSubscribe:
      serverthread_subscribe_thread(service,
				    sta->server,
				    sta->handle,
				    sta->access_id,
				    sta->nodeid,
				    sta->itemid,
				    sta->msgnumber,
				    sta->q_type,
				    sta->insert_request);
      break;
    case ServerThreadActionUnsubscribe:
      serverthread_unsubscribe_thread(service,
				      sta->server,
				      sta->handle,
				      sta->access_id,
				      sta->nodeid,
				      sta->itemid,
				      sta->msgnumber,
				      sta->insert_request);
      break;

    }
  
  if(sta->server)
    sib_server_unref(sta->server);
	
  if(sta->handle)
    {
      whiteboard_log_debug("Unreferencing handle\n");
      whiteboard_sib_access_handle_unref(sta->handle);
      sta->handle = NULL;
    }
	
  if(sta->insert_request)
    {
      whiteboard_log_debug("Deleting insert_request\n");
      g_free(sta->insert_request);
      sta->insert_request = NULL;
    }
  
 if(sta->remove_request)
    {
      whiteboard_log_debug("Deleting remove_request\n");
      g_free(sta->remove_request);
      sta->remove_request = NULL;
    }
  

	
  if(sta->itemid)
    g_free(sta->itemid);
  if(sta->nodeid)
    g_free(sta->nodeid);
	
  g_free(sta);

  whiteboard_log_debug_fe();
}

#if 0
static gpointer serverthread_receive_thread( gpointer userdata)
{
  gint err = 1;
  ServerThreadArgs* sta = (ServerThreadArgs *) userdata;
  whiteboard_log_debug_fb();
  sib_server_ref(sta->server);
  while( err >= 0 )
    {
      err = sib_access_receive( sib_server_get_sib_access(sta->server) );
      //      usleep(10); // sleep for a while
    }
  sib_server_unref(sta->server);
  g_free(sta);
  whiteboard_log_debug_fe();
  return NULL;
}
#endif
static void serverthread_insert_thread(SIBService* service,
				       SIBServer* server,
				       WhiteBoardSIBAccessHandle* handle,
				       guchar *nodeid,
				       guchar *sibid,
				       gint msgnumber,
				       EncodingType encoding,
				       guchar *request)
{
  
  g_return_if_fail(server != NULL);
  gint success=0;
  SIBController *ctrl = NULL;
  const guchar *udn = sib_server_get_udn( server );
  NodeMsgContent_t *response;
  g_return_if_fail(udn != NULL );

  whiteboard_log_debug_fb();
  
  g_return_if_fail(service != NULL);
  g_return_if_fail(handle != NULL);
  g_return_if_fail(request != NULL);
  
  ctrl = sib_service_get_controller(service);
  
  whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "insert_thread, node: %s, UDN: %s\n", nodeid, udn);
  
  success =  sib_access_insert(sib_server_get_sib_access(server), nodeid, msgnumber, encoding, request, &response);
  if( success < 0)
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "insert_thread: success: %d\n", success);
      sib_server_send_insert_response(handle, success, (guchar *)"sib:invalidTripleId");
    }
  else
    {
      if( response && parseSSAPmsg_get_msg_status(response) == MSG_E_OK )
	{
	  if( parseSSAPmsg_get_M3XML(response) != NULL)
	    {
	      whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "insert_thread: success: %d, response %s\n", 
				    success, parseSSAPmsg_get_M3XML(response));
	      sib_server_send_insert_response(handle, 0, (guchar *)parseSSAPmsg_get_M3XML(response));
	    }
	  else
	    {
	      whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "insert_thread: success: %d, no tripleid in response\n", success);
	      sib_server_send_insert_response(handle, -1,(guchar *) "sib:invalidTripleId");
	    }
	}
      else
	{
	  whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "insert_thread: parse error\n");
	  sib_server_send_insert_response(handle,-1, (guchar *)"sib:invalidTripleId");
	}
      if(response)
	parseSSAPmsg_free(&response);
      
    }
  
  whiteboard_log_debug_fe();
}


static void serverthread_update_thread(SIBService* service,
                                       SIBServer* server,
                                       WhiteBoardSIBAccessHandle* handle,
                                       guchar *nodeid,
                                       guchar *sibid,
                                       gint msgnumber,
				       EncodingType encoding,
                                       guchar *insert_request,
                                       guchar *remove_request)
{
  
  g_return_if_fail(server != NULL);
  gint success=0;
  SIBController *ctrl = NULL;
  const guchar *udn = sib_server_get_udn( server );
  NodeMsgContent_t *response;
  g_return_if_fail(udn != NULL );

  whiteboard_log_debug_fb();
  
  g_return_if_fail(service != NULL);
  g_return_if_fail(handle != NULL);

  g_return_if_fail(insert_request != NULL);
  g_return_if_fail(remove_request != NULL);
  
  
  ctrl = sib_service_get_controller(service);
  
  whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "update_thread, node: %s, UDN: %s\n", nodeid, udn);

  response = parseSSAPmsg_new();
  
  success =  sib_access_update(sib_server_get_sib_access(server), nodeid, msgnumber, encoding, insert_request, remove_request,  &response);
  if( success < 0)
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "update_thread: success: %d\n", success);
      sib_server_send_update_response(handle, success,(guchar *) "sib:invalidTripleId");
    }
  else
    {
      if( response && (parseSSAPmsg_get_msg_status(response) == MSG_E_OK ))
        {
          if( parseSSAPmsg_get_M3XML(response) != NULL)
            {
              whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "update_thread: success: %d, response %s\n",
				    success, parseSSAPmsg_get_M3XML(response));
              sib_server_send_update_response(handle, 0,(guchar *) parseSSAPmsg_get_M3XML(response));
            }
          else
            {
              whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "update_thread: success: %d, no tripleid in response\n", success);
              sib_server_send_update_response(handle, -1, (guchar *)"sib:invalidTripleId");
            }
        }
      else
        {
          whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "update_thread: parse error (%d) or no response: %p\n",
				(response ? parseSSAPmsg_get_msg_status(response): -1),
				response);
          sib_server_send_update_response(handle,-1,(guchar *) "sib:invalidTripleId");
        }
      if(response)
	parseSSAPmsg_free(&response);
    }
  
  whiteboard_log_debug_fe();
}


static void serverthread_remove_thread(SIBService* service,
				       SIBServer* server,
				       WhiteBoardSIBAccessHandle* handle,
				       guchar *nodeid,
				       guchar *sibid,
				       gint msgnumber,
				       EncodingType encoding,
				       guchar *request)
{
  g_return_if_fail(server != NULL);
  gint success=0;
  NodeMsgContent_t *response = NULL;
  SIBController *ctrl = NULL;
  const guchar *udn = sib_server_get_udn( server );
  g_return_if_fail(udn != NULL );
  
  whiteboard_log_debug_fb();
  
  g_return_if_fail(service != NULL);
  g_return_if_fail(handle != NULL);
  g_return_if_fail(request != NULL);
  
  ctrl = sib_service_get_controller(service);
  
  whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "remove_thread, node: %s, UDN: %s\n", nodeid, udn);

  success =  sib_access_remove(sib_server_get_sib_access(server), nodeid, msgnumber, encoding, request, &response);
  if( success < 0)
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "remove_thread: Fail: %d\n", success);
      sib_server_send_remove_response(handle, ss_OperationFailed,(guchar *) "connection failure or bad response from sib");
    }
  else if( response)
    {
      if( parseSSAPmsg_get_msg_status(response) == MSG_E_OK )
	{
	  sib_server_send_remove_response(handle, ss_StatusOK ,(guchar *) "");
	}
      else
	{
	  whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "remove_thread: operation failed\n");
	  sib_server_send_remove_response(handle, ss_OperationFailed, (guchar *)"sib:reported error");
	}
    }
  else
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "remove_thread: no response\n");
      sib_server_send_remove_response(handle, ss_OperationFailed, (guchar *)"no reponse");
    }
  
  if(response)
    parseSSAPmsg_free(&response);
  
  whiteboard_log_debug_fe();
}

static void serverthread_join_thread(SIBService* service, 
				     SIBServer* server,
				     WhiteBoardSIBAccessHandle* handle,
				     gint access_id,
				     guchar *nodeid,
				     guchar *itemid,
				     gint msgnumber)
{
  g_return_if_fail(server != NULL);
  SIBController *ctrl = NULL;
  const guchar *udn = sib_server_get_udn( server );
  g_return_if_fail(udn != NULL);
  gint success = 0;
  NodeMsgContent_t *response = NULL;
  whiteboard_log_debug_fb();
  
  g_return_if_fail(service != NULL);
  g_return_if_fail(server != NULL);
  g_return_if_fail(handle != NULL);
  g_return_if_fail(nodeid != NULL);
  
  
  ctrl = sib_service_get_controller(service);
  
  whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "join_thread, node: %s, UDN: %s\n", nodeid, udn);
  //  response = parseSSAPmsg_new();
  success =  sib_access_join(sib_server_get_sib_access(server), nodeid, msgnumber, (guchar *)"1234567890", &response );
  if( success < 0) 
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB,"Join failed for SIB (%s), err: %d\n", udn, success);
      sib_server_send_join_complete(handle, access_id, ss_OperationFailed);
    }
  else if( response != NULL )
    {
      if( parseSSAPmsg_get_msg_status(response) == MSG_E_OK)
	{
	  sib_server_send_join_complete(handle,access_id, ss_StatusOK );
	}
      else
	{
	  sib_server_send_join_complete(handle,access_id, ss_OperationFailed );
	}
    }
  else
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB,"Join failed for SIB (%s), no response\n", udn);
      sib_server_send_join_complete(handle,access_id, ss_OperationFailed);
    }
  if(response)
    parseSSAPmsg_free(&response);
  
  whiteboard_log_debug_fe();
}

static void serverthread_leave_thread(SIBService* service, 
				      SIBServer* server,
				      WhiteBoardSIBAccessHandle* handle,
				      guchar *nodeid,
				      guchar *itemid,
				      gint msgnumber)
{
  g_return_if_fail(server != NULL );
  SIBController *ctrl = NULL;
  NodeMsgContent_t *response = NULL;
  const guchar *udn = sib_server_get_udn( server );
  g_return_if_fail(udn != NULL);
  gint success = 0;
  whiteboard_log_debug_fb();
  
  g_return_if_fail(service != NULL);
  g_return_if_fail(server != NULL);
  g_return_if_fail(handle != NULL);
  g_return_if_fail(nodeid != NULL);
  
  ctrl = sib_service_get_controller(service);
  
  whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "leave_thread, node: %s, UDN: %s\n", nodeid, udn);
  success =  sib_access_leave(sib_server_get_sib_access(server), nodeid, msgnumber, &response );
  if(success < 0)
    {
      whiteboard_log_error("leave failed for SIB (%s)\n", udn);
    }
  
  if(response)
    parseSSAPmsg_free(&response);
  
  whiteboard_log_debug_fe();
}

static void serverthread_query_thread(SIBService* service,
				      SIBServer* server,
				      WhiteBoardSIBAccessHandle* handle,
				      gint access_id,
				      guchar *nodeid,
				      guchar *sibid,
				      gint msgnumber,
				      gint type,
				      guchar *request)
{
  g_return_if_fail(server != NULL);
  gint success=0;
  NodeMsgContent_t *response = NULL;
  SIBController *ctrl = NULL;
  const guchar *udn = sib_server_get_udn( server );
  g_return_if_fail(udn != NULL );
  whiteboard_log_debug_fb();
  
  g_return_if_fail(service != NULL);
  g_return_if_fail(handle != NULL);
  g_return_if_fail(request != NULL);
  
  ctrl = sib_service_get_controller(service);
  
  whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "query_thread, node: %s, UDN: %s\n", nodeid, udn);

  success =  sib_access_query(sib_server_get_sib_access(server), nodeid, msgnumber, type, request, &response);
  if( success <= 0)
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "query_thread: success: %d\n", success);
      sib_server_send_query_response(handle,access_id, success, (guchar *)"InvalidResults");
    }
  else
    {
      //whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "query_thread: success: %d, response %s\n", success, parseSSAPmsg_get_M3XML(response));
      if( response && parseSSAPmsg_get_msg_status(response) == MSG_E_OK )
	{
	  if( NULL != parseSSAPmsg_get_M3XML(response) )
	    sib_server_send_query_response(handle, access_id, ss_StatusOK,(guchar *) parseSSAPmsg_get_M3XML(response) );
	  else
	    {
	      whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB,"No results");
	      sib_server_send_query_response(handle, access_id, ss_OperationFailed,(guchar *)"InvalidResults");
	    }
	}
      else
	{
	  whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "Parse error\n");
	  sib_server_send_query_response(handle, access_id,ss_OperationFailed,(guchar *)"InvalidResults");
	}
      if(response)
	parseSSAPmsg_free(&response);
      
      whiteboard_log_debug_fe();
    }
}

static void serverthread_subscribe_thread(SIBService* service,
					  SIBServer* server,
					  WhiteBoardSIBAccessHandle* handle,
					  gint access_id,
					  guchar *nodeid,
					  guchar *sibid,
					  gint msgnumber,
					  gint type,
					  guchar *request)
{
  g_return_if_fail(server != NULL);
  gint success=0;
  NodeMsgContent_t *response = NULL;
  SIBController *ctrl = NULL;
  SIBAccess *sa=NULL;
  const guchar *udn = (guchar *)sib_server_get_udn( server );
  guchar *subscriptionid=NULL;


  gint err = 0;

  g_return_if_fail(udn != NULL );
  
  whiteboard_log_debug_fb();
  
  g_return_if_fail(service != NULL);
  g_return_if_fail(handle != NULL);
  g_return_if_fail(request != NULL);
  
  ctrl = sib_service_get_controller(service);
  
  whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "subscribe_thread, node: %s, UDN: %s\n", nodeid, udn);
  sa = sib_server_get_sib_access(server);
  success =  sib_access_subscribe(sa, nodeid, msgnumber, type, request, &response);
  if( success <= 0)
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "subscribe_thread: success: %d\n", success);
      sib_server_send_subscribe_response(handle, access_id, ss_OperationFailed,(guchar *)"sib:InvalidSubscriptionID",(guchar *) "sib:InvalidResults");
    }
  else
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "subscribe_thread: success: %d\n", success);
      if( response && parseSSAPmsg_get_msg_status(response) == MSG_E_OK )
	{
	  subscriptionid = (guchar *) g_strdup((gchar *)parseSSAPmsg_get_subscriptionid(response));
	  sib_server_send_subscribe_response(handle,
					     access_id,
					     ss_StatusOK,
					     (guchar *) parseSSAPmsg_get_subscriptionid(response),
					     (guchar *)parseSSAPmsg_get_M3XML(response));
	}
      else
	{
	  whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "Creating of subscription failed\n");
	  sib_server_send_subscribe_response(handle, access_id, ss_OperationFailed,
					     (guchar *)"InvalidSubscriptionID", (guchar *)"InvalidResults");
	  success = -1;
	}
    }
  if(response)
    parseSSAPmsg_free(&response);
  
  if( success > 0)
    {
      while( (err=sib_access_wait_for_subscription_ind(sa, subscriptionid, &response)) >= 0)
	{
	  if( (parseSSAPmsg_get_name(response) == MSG_N_SUBSCRIBE) &&
	      (parseSSAPmsg_get_type(response) == MSG_T_IND) )
	    {
	      sib_server_send_subscription_indication(handle, access_id, 
						      parseSSAPmsg_get_update_sequence(response),
						      (guchar *)parseSSAPmsg_get_subscriptionid(response), 
						      (guchar *)parseSSAPmsg_get_results_added(response),
						      (guchar *)parseSSAPmsg_get_results_removed(response) );
	    }
	  else if( (parseSSAPmsg_get_name(response) == MSG_N_UNSUBSCRIBE) &&
		   (parseSSAPmsg_get_type(response) == MSG_T_CNF) )
	    {
	      sib_server_send_unsubscribe_complete(handle, access_id, 
						   (gint)parseSSAPmsg_get_msg_status(response), 
						   (guchar *)parseSSAPmsg_get_subscriptionid(response));
	      break;
	    }
	  else
	    {
	      whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB,
				    "Received msg not subscibe_ind of unsubscribe_cnf, ignoring\n");
	    }
	  
	  if(response)
	    {
	      parseSSAPmsg_free(&response);
	    }
	  
	}

      
      if(err<0)
	{
	  sib_server_send_unsubscribe_complete(handle, access_id, ss_OperationFailed, subscriptionid);
	}
    }
  
  if(subscriptionid)
    {
      g_free(subscriptionid);
    }
  
  whiteboard_log_debug_fe();
}


static void serverthread_unsubscribe_thread(SIBService* service,
					    SIBServer* server,
					    WhiteBoardSIBAccessHandle* handle,
					    gint access_id,
					    guchar *nodeid,
					    guchar *sibid,
					    gint msgnumber,
					    guchar *request)
{
  g_return_if_fail(server != NULL);
  gint success=0;
  SIBController *ctrl = NULL;
  SIBAccess *sa=NULL;
  const guchar *udn = sib_server_get_udn( server );

  g_return_if_fail(udn != NULL );
  
  whiteboard_log_debug_fb();
  
  g_return_if_fail(service != NULL);
  g_return_if_fail(handle != NULL);
  g_return_if_fail(request != NULL);
  
  ctrl = sib_service_get_controller(service);
  
  whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "unsubscribe_thread, node: %s, UDN: %s\n", nodeid, udn);

  sa = sib_server_get_sib_access(server);
  success =  sib_access_unsubscribe(sa, nodeid, msgnumber, request);
  if( success < 0)
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "unsubscribe_thread: success: %d\n", success);
      sib_server_send_unsubscribe_complete(handle, access_id, success, request);
    }
  whiteboard_log_debug_fe();
}

