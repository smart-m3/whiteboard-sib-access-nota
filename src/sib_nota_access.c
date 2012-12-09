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
 * Media Hub UPnP Source Component
 *
 * sib_server.c
 *
 * Copyright 2007 Nokia Corporation
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <glib.h>
#include <whiteboard_log.h>

#include <sibmsg.h>

#include "sib_controller.h"
//#include "sib_maxfd.h"
#include <h_in/h_bsdapi.h>


#include "SibAccessNota_user.h" // Stub generated file

// use timeout values 10 ms
#define SELECT_TIMEOUT_SEC 0
#define SELECT_TIMEOUT_USEC 1000

#define REQUEST_TIMEOUT_USEC 10000000 // 10 sec 

typedef struct _RequestData
{
  
  NodeMsgContent_t **msg;
  GCond *cond;
  GMutex *mutex;
  gboolean completed;
  guchar *subscription_id; // used only w/ unsubscribe
} RequestData;

struct _SIBAccess
{
  SIBController* cp;

  guchar *uri;

  int sid;
  int port;
  int fd;
  
  struct context_pointer *context;
  h_in_t *core;
  gint refcount;

  gboolean receive_cancelled;
  
  GMutex *mutex;  // lock for socket

  // msgnumber -> RequestData
  GHashTable *requests;
  
  // subscription_id -> RequestData, only for subscriptionInds
  GHashTable *subscriptions;
  
  GMutex *request_mutex;
  GMutex *sub_mutex;

  gint msgnumber;
  GMutex *msgnumber_lock;
};


static gint sib_access_create_socket(SIBAccess *self);

static void sib_access_requests_lock(SIBAccess *self);
static void sib_access_requests_unlock(SIBAccess *self);

static void sib_access_msgnumber_lock(SIBAccess *self);
static void sib_access_msgnumber_unlock(SIBAccess *self);

static gint sib_access_add_request(SIBAccess *self, gint msgNumber, NodeMsgContent_t **msg);

static NodeMsgContent_t **sib_access_get_and_lock_request(SIBAccess *self, gint msgNumber);
static RequestData *sib_access_get_and_lock_request_data(SIBAccess *self, gint msgNumber);
static void sib_access_unlock_request(SIBAccess *self, gint msgNumber);
static gint sib_access_wait_for_confirm(SIBAccess *self, gint msgNumber);
static void sib_access_complete_and_unlock_request(SIBAccess *self, gint msgNumber);
static void sib_access_remove_request(SIBAccess *self, gint msgNumber);

static void sib_access_subscriptions_lock(SIBAccess *self);
static void sib_access_subscriptions_unlock(SIBAccess *self);
static gint sib_access_add_subscription(SIBAccess *self, guchar *id,  NodeMsgContent_t **msg);
static NodeMsgContent_t **sib_access_get_and_lock_subscription(SIBAccess *self, guchar *id );
static gint sib_access_wait_for_indication(SIBAccess *self, guchar *id );
static void sib_access_complete_and_unlock_subscription(SIBAccess *self, guchar *id);
static void sib_access_remove_subscription(SIBAccess *self, guchar *id);

SIBAccess* sib_access_new(SIBController* cp, guchar *uri, gint sid, gint port)
{
  SIBAccess* self = NULL;

  whiteboard_log_debug_fb();

  g_return_val_if_fail(cp != NULL, NULL);

  self = g_new0(SIBAccess, 1);
  g_return_val_if_fail(self != NULL, NULL);

  self->cp = cp;
  self->uri=(guchar *)g_strdup((gchar *)uri);
  self->sid = sid;
  self->port = port;
  self->fd = -1;
  self->context = NULL;
  self->core = Hgetinstance();
  self->refcount=1;

  self->mutex = g_mutex_new();
  whiteboard_log_debug("mutex %p created for SibAccess %p\n", self->mutex, self);

  self->requests = g_hash_table_new( g_direct_hash, g_direct_equal );
  self->request_mutex = g_mutex_new();


  self->subscriptions = g_hash_table_new_full( g_str_hash, g_str_equal, g_free, NULL );
  self->sub_mutex = g_mutex_new();
  g_return_val_if_fail( sib_access_create_socket(self) == 0 , NULL);
  
  g_return_val_if_fail(nota_stub_handler_add_connection( sib_controller_get_stubhandler( self->cp), 
							 self->fd, self->context) == 0, NULL);

  self->msgnumber_lock = g_mutex_new();
  self->msgnumber = 0;
  whiteboard_log_debug_fe();
  return self;
}


/**
 * Destroy a SIBAccess struct
 *
 * @param sa The SIBAccess struct to destroy
 * @return FALSE for error, TRUE for success.
 */
gboolean sib_access_destroy(SIBAccess* sa)
{
  whiteboard_log_debug_fb();
  
  g_return_val_if_fail(sa != NULL, FALSE);
  /* 	g_return_val_if_fail(server->mutex != NULL, FALSE); */
  

    /* Free the URI string */
  if (sa->uri)
    g_free(sa->uri);
  sa->uri = NULL;

  g_mutex_lock(sa->mutex);
  sa->receive_cancelled = TRUE;
  
  if( sa->context )
    {
      whiteboard_log_debug("Closing nota connection\n");
      SibAccessNota_user_remove_connection(sa->context);
      
      nota_stub_handler_remove_connection( sib_controller_get_stubhandler( sa->cp), 
					   sa->fd);
      //nota_stub_context_pointer_free( sa->context);
      //sa->context = NULL;
    }
  
  //if(sa->fd > 0)
  // {
  //   Hclose(sa->core, sa->fd);
  //   sa->fd = -1;
  // }
  g_mutex_unlock(sa->mutex);
  g_mutex_free(sa->mutex);

  g_hash_table_destroy( sa->requests);
  g_hash_table_destroy( sa->subscriptions);

  g_mutex_lock(sa->msgnumber_lock);
  g_mutex_unlock(sa->msgnumber_lock);
  g_mutex_free(sa->msgnumber_lock);
  /*Finally destroy self */
  g_free(sa);
  whiteboard_log_debug_fe();
  
  return TRUE;
}



/**
 * Increase the server's reference count
 *
 * @param server The server, whose reference count to increase
 */
void sib_access_ref(SIBAccess* self)
{
  whiteboard_log_debug_fb();

  g_return_if_fail(self != NULL);
  
  if (g_atomic_int_get(&self->refcount) > 0)
    {
      /* Refcount is something sensible */
      g_atomic_int_inc(&self->refcount);
      
      whiteboard_log_debug("SIBAccess refcount: %d\n",
			   self->refcount);
    }
  else
    {
      /* Refcount is already zero */
      whiteboard_log_debug("SIBAccess  refcount already %d!\n",
			   self->refcount);
    }
  
  whiteboard_log_debug_fe();
}

/**
 * Decrease the server's reference count. If the counter reaches zero,
 * the server is destroyed.
 *
 * @param server The server, whose reference count to increase
 */
void sib_access_unref(SIBAccess* self)
{
  whiteboard_log_debug_fb();
  
  g_return_if_fail(self != NULL);
  
  if (g_atomic_int_dec_and_test(&self->refcount) == FALSE)
    {
      whiteboard_log_debug("SIBAccess refcount dec: %d\n", 
			   self->refcount);
    }
  else
    {
      whiteboard_log_debug("SIBAccess refcount zeroed. Destroy.\n");
      sib_access_destroy(self);
    }
  
  whiteboard_log_debug_fe();
}

const gchar* sib_access_get_uri(SIBAccess* self)
{
  g_return_val_if_fail(self != NULL, NULL);
  return (const gchar*) self->uri;
}

/**
 * Compare the given SIBServer pointer with the given UDN.
 *
 * @param server The SIBServer to compare with
 * @param udn The UDN to try and match
 * @return 0 if server and udn match, !0 if they don't match
 */
gint sib_access_compare_uri(gconstpointer sib, gconstpointer uri)
{
  SIBAccess* sa = (SIBAccess*) sib;
  
  g_return_val_if_fail(sa != NULL, 1);
  g_return_val_if_fail(uri != NULL, -1);
  
  return g_ascii_strcasecmp( (gchar *)sa->uri, uri);
}
#if 0
gint sib_access_receive(SIBAccess* sa)
{
  gint retvalue;
  gint err;
  struct timeval tv;
  fd_set read_s;
  fd_set error_s;
  int maxfd = 0;
  FD_ZERO(&read_s);
  FD_ZERO(&error_s);
  FD_SET(sa->fd, &read_s);
  FD_SET(sa->fd, &error_s);

  //  tv.tv_sec = SELECT_TIMEOUT_SEC;
  //tv.tv_usec = SELECT_TIMEOUT_USEC;
  tv.tv_sec = 0;
  tv.tv_usec = 0;

  if(sa->receive_cancelled)
    {
      whiteboard_log_debug("Receive cancelled\n");
      return -1;
    }
  //  g_mutex_lock(sa->mutex);
  //  whiteboard_log_debug("Mutex %p locked for SibAccess %p\n", sa->mutex, sa);
  //whiteboard_log_debug("Select on socket %d\n",sa->fd);
  maxfd = sib_maxfd_lock_and_get( sib_controller_get_maxfd(sa->cp) );
  err = Hselect(sa->core, maxfd + 1, &read_s, NULL, &error_s, &tv);
  sib_maxfd_unlock(sib_controller_get_maxfd(sa->cp) );
  if(err < 0)
    {
      whiteboard_log_debug("Select (%d), err: %d\n", sa->fd, err);
      retvalue = -1;
    }
  else
    {
      if(FD_ISSET(sa->fd, &read_s))
	{
	  whiteboard_log_debug("Something received, call nota_stub_handle_incoming\n");
	  err = nota_stub_handle_incoming(sa->context);
	  if(err < 0)
	    {
	      whiteboard_log_debug("Failed incoming data (%d)\n", err);
	      retvalue = -1;
	    }
	  else
	    {
	      retvalue = 1;
	    }
	}
      else if(FD_ISSET(sa->fd, &error_s))
	{
	  whiteboard_log_debug("Error in socket\n");
	  retvalue = -1;
	}
      else
	{
	  //whiteboard_log_debug("select timeout");
	  retvalue = 0;
	}
    }
  // g_mutex_unlock(sa->mutex);
  // whiteboard_log_debug("Mutex %p UNlocked for SibAccess %p\n", sa->mutex, sa);
  return retvalue;
}
#endif
gint sib_access_join( SIBAccess *sa,
		      ssElement_ct nodeid,
		      gint msgnumber,
		      ssElement_ct credentials,
		      NodeMsgContent_t **msgContent )
{
  gint retvalue = -1;
  gint id;
  //  gint err;
  //h_in_t *core = Hgetinstance();
  whiteboard_log_debug_fb();
  whiteboard_log_debug("Locking mutex %p for SibAccess %p\n", sa->mutex, sa);
  
  sib_access_msgnumber_lock(sa);
  id = ++sa->msgnumber;
  sib_access_msgnumber_unlock(sa);
  if( sib_access_add_request(sa, id, msgContent) < 0 )
    {
      whiteboard_log_debug("Could not add request (msgnumber: %d) to hashtable\n", id);
    }
  else
    {
      g_mutex_lock(sa->mutex);
      whiteboard_log_debug("Mutex %p locked for SibAccess %p\n", sa->mutex, sa);
      retvalue= SibAccessNota_Join_req( sa->context,
					(uint8_t *)nodeid,
					(uint16_t)strlen( (gchar *)nodeid),
					(uint8_t *) sa->uri,
					(uint16_t)strlen((gchar *)sa->uri),
					id,
					(uint8_t *)credentials,
					(uint16_t)strlen((gchar *) credentials));
      g_mutex_unlock(sa->mutex);
      whiteboard_log_debug("Mutex %p UNlocked for SibAccess %p\n", sa->mutex, sa);
      if(retvalue < 0)
	{
	  whiteboard_log_debug("Sending Join_Req failed: %d\n", retvalue);
	}
      else
	{
	  // block until we receive confirmation or timeout
	  retvalue = sib_access_wait_for_confirm(sa, id);
	  if(retvalue < 0)
	    {
	      whiteboard_log_debug("Error while waiting respose for join\n");
	    }
	  else if(retvalue == 0)
	    {
	      whiteboard_log_debug("Join response timeout\n");
	    }
	  else
	    {
	      whiteboard_log_debug("Got Join response\n");
	    }
	  sib_access_remove_request(sa, id);
	}
    }
  whiteboard_log_debug_fe();
  return retvalue;
}

gint sib_access_leave( SIBAccess *sa, ssElement_ct nodeid, gint msgnumber, NodeMsgContent_t **msgContent )
{
  gint retvalue = -1;
  whiteboard_log_debug_fb();
  gint id;
  g_return_val_if_fail( NULL != sa, -1);
  g_return_val_if_fail( NULL != nodeid, -1 );
  sib_access_msgnumber_lock(sa);
  id = ++sa->msgnumber;
  sib_access_msgnumber_unlock(sa);
  if( sib_access_add_request(sa, id, msgContent) < 0 )
    {
      whiteboard_log_debug("Could not add request (msgnumber: %d) to hashtable\n", id);
    }
  else
    {
      g_mutex_lock(sa->mutex);
      whiteboard_log_debug("Mutex %p locked for SibAccess %p\n", sa->mutex, sa);
      retvalue= SibAccessNota_Leave_req( sa->context,
					 (uint8_t *)nodeid,
					 (uint16_t)strlen((gchar *)nodeid),
					 (uint8_t *) sa->uri,
					 (uint16_t)strlen((gchar *)sa->uri),
					 id );
      g_mutex_unlock(sa->mutex);
      whiteboard_log_debug("Mutex %p UNlocked for SibAccess %p\n", sa->mutex, sa);
      if(retvalue < 0)
	{
	  whiteboard_log_debug("Sending Leave_Req failed: %d\n", retvalue);
	}
      else
	{
	  // block until we receive confirmation or timeout
	  retvalue = sib_access_wait_for_confirm(sa, id);
	  if(retvalue < 0)
	    {
	      whiteboard_log_debug("Error while waiting respose for leave\n");
	    }
	  else if(retvalue == 0)
	    {
	      whiteboard_log_debug("Leave response timeout\n");
	    }
	  else
	    {
	      whiteboard_log_debug("Got leave response\n");
	    }
	  sib_access_remove_request(sa, id);
	}
    }
  whiteboard_log_debug_fe();
  return retvalue;
}

gint sib_access_insert(SIBAccess *sa,
		       guchar *nodeid,
		       gint msgnumber,
		       EncodingType encoding,
		       guchar *request,
		       NodeMsgContent_t **msgContent)
{
  gint retvalue = -1;
  encoding_t notae;
  whiteboard_log_debug_fb();
  gint id;
  g_return_val_if_fail( NULL != sa, -1);
  g_return_val_if_fail( NULL != nodeid, -1 );
  g_return_val_if_fail( NULL != request, -1 );

  switch(encoding)
    {
    case EncodingM3XML:
      notae = 	ENC_RDF_M3;
      break;
    case EncodingRDFXML:
      notae = ENC_RDF_XML;
      break;
    default:
      g_return_val_if_reached(-1);
    }
  sib_access_msgnumber_lock(sa);
  id = ++sa->msgnumber;
  sib_access_msgnumber_unlock(sa);  
  if( sib_access_add_request(sa, id, msgContent) < 0 )
    {
      whiteboard_log_debug("Could not add request (msgnumber: %d) to hashtable\n", id);
    }
  else
    {
      g_mutex_lock(sa->mutex);
      whiteboard_log_debug("Mutex %p locked for SibAccess %p\n", sa->mutex, sa);
      retvalue= SibAccessNota_Insert_req( sa->context,
					  (uint8_t *)nodeid,
					  (uint16_t)strlen((gchar *)nodeid),
					  (uint8_t *) sa->uri,
					  (uint16_t)strlen((gchar *)sa->uri),
					  id,
					  notae,
					  (uint8_t *)request,
					  (uint16_t)strlen((gchar *)request),
					  CONFIRM_YES);
      g_mutex_unlock(sa->mutex);
      whiteboard_log_debug("Mutex %p UNlocked for SibAccess %p\n", sa->mutex, sa);
      if(retvalue < 0)
	{
	  whiteboard_log_debug("Sending Insert_Req failed: %d\n", retvalue);
	}
      else
	{
	  // block until we receive confirmation or timeout
	  retvalue = sib_access_wait_for_confirm(sa, id);
	  if(retvalue < 0)
	    {
	      whiteboard_log_debug("Error while waiting respose for insert\n");
	    }
	  else if(retvalue == 0)
	    {
	      whiteboard_log_debug("Insert response timeout\n");
	    }
	  else
	    {
	      whiteboard_log_debug("Got insert response\n");
	    }
	  sib_access_remove_request(sa, id);
	}
    }
  
  whiteboard_log_debug_fe();
  return retvalue;
}

gint sib_access_update(SIBAccess *sa, guchar *nodeid, gint msgnumber, EncodingType encoding, guchar *irequest, guchar *rrequest, NodeMsgContent_t **msgContent)
{
  encoding_t notae;
  gint retvalue = -1;
  gint id;
  whiteboard_log_debug_fb();

  g_return_val_if_fail( NULL != sa, -1);
  g_return_val_if_fail( NULL != nodeid, -1 );
  g_return_val_if_fail( NULL != irequest, -1 );
  g_return_val_if_fail( NULL != rrequest, -1 );
  switch(encoding)
    {
    case EncodingM3XML:
      notae = 	ENC_RDF_M3;
      break;
    case EncodingRDFXML:
      notae = ENC_RDF_XML;
      break;
    default:
      g_return_val_if_reached(-1);
    }
    sib_access_msgnumber_lock(sa);
  id = ++sa->msgnumber;
  sib_access_msgnumber_unlock(sa);
  if( sib_access_add_request(sa, id, msgContent) < 0 )
    {
      whiteboard_log_debug("Could not add request (msgnumber: %d) to hashtable\n", id);
    }
  else
    {
      g_mutex_lock(sa->mutex);
      whiteboard_log_debug("Mutex %p locked for SibAccess %p\n", sa->mutex, sa);
      retvalue= SibAccessNota_Update_req( sa->context,
					  (uint8_t *)nodeid,
					  (uint16_t)strlen((gchar *)nodeid),
					  (uint8_t *) sa->uri,
					  (uint16_t)strlen((gchar *)sa->uri),
					  id,
					  notae,
					  (uint8_t *)irequest,
					  (uint16_t)strlen((gchar *)irequest),
					  (uint8_t *)rrequest,
					  (uint16_t)strlen((gchar *)rrequest),
					  CONFIRM_YES);
      g_mutex_unlock(sa->mutex);
      whiteboard_log_debug("Mutex %p UNlocked for SibAccess %p\n", sa->mutex, sa);
      if(retvalue < 0)
	{
	  whiteboard_log_debug("Sending Update_Req failed: %d\n", retvalue);
	}
      else
	{
	  // block until we receive confirmation or timeout
	  retvalue = sib_access_wait_for_confirm(sa, id);
	  if(retvalue < 0)
	    {
	      whiteboard_log_debug("Error while waiting respose for update\n");
	    }
	  else if(retvalue == 0)
	    {
	      whiteboard_log_debug("Update response timeout\n");
	    }
	  else
	    {
	      whiteboard_log_debug("Got update response, status: %d\n", (*msgContent ? parseSSAPmsg_get_msg_status(*msgContent): -1) );
	      
	    }
	  sib_access_remove_request(sa, id);
	}
    }
  
  whiteboard_log_debug_fe();
  return retvalue;
}

gint sib_access_remove(SIBAccess *sa, guchar *nodeid, gint msgnumber, EncodingType encoding, guchar *request, NodeMsgContent_t **msgContent)
{
  encoding_t notae;
  gint id;
  gint retvalue = -1;
  whiteboard_log_debug_fb();

  g_return_val_if_fail( NULL != sa, -1);
  g_return_val_if_fail( NULL != nodeid, -1 );
  g_return_val_if_fail( NULL != request, -1 );
  switch(encoding)
    {
    case EncodingM3XML:
      notae = 	ENC_RDF_M3;
      break;
    case EncodingRDFXML:
      notae = ENC_RDF_XML;
      break;
    default:
      whiteboard_log_debug("Invalid encoding: %d", encoding);
      g_return_val_if_reached(-1);
    }
  sib_access_msgnumber_lock(sa);
  id = ++sa->msgnumber;
  sib_access_msgnumber_unlock(sa);
  if( sib_access_add_request(sa, id, msgContent) < 0 )
    {
      whiteboard_log_debug("Could not add request (msgnumber: %d) to hashtable\n", id);
    }
  else
    {
      g_mutex_lock(sa->mutex);
      whiteboard_log_debug("Mutex %p locked for SibAccess %p\n", sa->mutex, sa);
      retvalue= SibAccessNota_Remove_req( sa->context,
					  (uint8_t *)nodeid,
					  (uint16_t)strlen((gchar *)nodeid),
					  (uint8_t *) sa->uri,
					  (uint16_t)strlen((gchar *)sa->uri),
					  id,
					  notae,
					  (uint8_t *)request,
					  (uint16_t)strlen((gchar *)request),
					  CONFIRM_YES);
      g_mutex_unlock(sa->mutex);
      whiteboard_log_debug("Mutex %p UNlocked for SibAccess %p\n", sa->mutex, sa);
      if(retvalue < 0)
	{
	  whiteboard_log_debug("Sending Remove_Req failed: %d\n", retvalue);
	}
      else
	{
	  // block until we receive confirmation or timeout
	  retvalue = sib_access_wait_for_confirm(sa, id);
	  if(retvalue < 0)
	    {
	      whiteboard_log_debug("Error while waiting respose for remove\n");
	    }
	  else if(retvalue == 0)
	    {
	      whiteboard_log_debug("Removet response timeout\n");
	    }
	  else
	    {
	      whiteboard_log_debug("Got remove response\n");
	    }
	  sib_access_remove_request(sa, id);
	}
    }
  
  whiteboard_log_debug_fe();
  return retvalue;
}

gint sib_access_query(SIBAccess *sa,
		      guchar *nodeid,
		      gint msgnumber,
		      gint type,
		      guchar *request,
		      NodeMsgContent_t **msgContent) 

{
  gint id;
  gint retvalue = -1;
  query_format_t qformat = QFORMAT_RDF_M3;
  whiteboard_log_debug_fb();
  sib_access_msgnumber_lock(sa);
  id = ++sa->msgnumber;
  sib_access_msgnumber_unlock(sa);
  if( sib_access_add_request(sa, id, msgContent) < 0 )
    {
      whiteboard_log_debug("Could not add request (msgnumber: %d) to hashtable\n", id);
    }
  else
    {
      switch(type)
	{
	case QueryTypeTemplate:
	  qformat = QFORMAT_RDF_M3;
	  break;
	case QueryTypeWQLValues:
	  qformat = QFORMAT_WQL_VALUES;
	  break;
	case QueryTypeWQLNodeTypes:
	  qformat = QFORMAT_WQL_NODETYPES;
	  break;
	case QueryTypeWQLRelated:
	  qformat = QFORMAT_WQL_RELATED;
	  break;
	case QueryTypeWQLIsType:
	  qformat = QFORMAT_WQL_ISTYPE;
	  break;
	case QueryTypeWQLIsSubType:
	  qformat = QFORMAT_WQL_ISSUBTYPE;
	  break;
	default:
	  whiteboard_log_debug("Invalid QueryFormat\n");
	  break;
	}
      g_mutex_lock(sa->mutex);
      whiteboard_log_debug("Mutex %p locked for SibAccess %p\n", sa->mutex, sa);
      
      retvalue= SibAccessNota_Query_req( sa->context,
					 (uint8_t *)nodeid,
					 (uint16_t)strlen((gchar *)nodeid),
					 (uint8_t *) sa->uri,
					 (uint16_t)strlen((gchar *)sa->uri),
					 id,
					 qformat,
					 (uint8_t *)request,
					 (uint16_t)strlen((gchar *)request),
					 CONFIRM_YES);
      g_mutex_unlock(sa->mutex);
      whiteboard_log_debug("Mutex %p UNlocked for SibAccess %p\n", sa->mutex, sa);
      if(retvalue < 0)
	{
	  whiteboard_log_debug("Sending Query_Req failed: %d\n", retvalue);
	}
      else
	{
	  // block until we receive confirmation or timeout
	  retvalue = sib_access_wait_for_confirm(sa, id);
	  if(retvalue < 0)
	    {
	      whiteboard_log_debug("Error while waiting response for Query\n");
	    }
	  else if(retvalue == 0)
	    {
	      whiteboard_log_debug("Query response timeout\n");
	    }
	  else
	    {
	      whiteboard_log_debug("Got query response\n");
	    }
	  sib_access_remove_request(sa, id);
	}
    }
  
  whiteboard_log_debug_fe();
  return retvalue;

}

gint sib_access_subscribe(SIBAccess *sa, guchar *nodeid, gint msgnumber, gint type, guchar *request, NodeMsgContent_t **msgContent)
{
  gint id;
  gint retvalue = -1;
  query_format_t qformat = QFORMAT_RDF_M3;
  whiteboard_log_debug_fb();
  sib_access_msgnumber_lock(sa);
  id = ++sa->msgnumber;
  sib_access_msgnumber_unlock(sa);
  if( sib_access_add_request(sa, id, msgContent) < 0 )
    {
      whiteboard_log_debug("Could not add request (msgnumber: %d) to hashtable\n", id);
    }
  else
    {
      switch(type)
	{
	case QueryTypeTemplate:
	  qformat = QFORMAT_RDF_M3;
	  break;
	case QueryTypeWQLValues:
	  qformat = QFORMAT_WQL_VALUES;
	  break;
	case QueryTypeWQLNodeTypes:
	  qformat = QFORMAT_WQL_NODETYPES;
	  break;
	case QueryTypeWQLRelated:
	  qformat = QFORMAT_WQL_RELATED;
	  break;
	case QueryTypeWQLIsType:
	  qformat = QFORMAT_WQL_ISTYPE;
	  break;
	case QueryTypeWQLIsSubType:
	  qformat = QFORMAT_WQL_ISSUBTYPE;
	  break;
	default:
	  whiteboard_log_debug("Invalid QueryFormat\n");
	  break;
	}
      g_mutex_lock(sa->mutex);
      whiteboard_log_debug("Mutex %p locked for SibAccess %p\n", sa->mutex, sa);
      
      retvalue= SibAccessNota_Subscribe_req( sa->context,
					     (uint8_t *)nodeid,
					     (uint16_t)strlen((gchar *)nodeid),
					     (uint8_t *) sa->uri,
					     (uint16_t)strlen((gchar *)sa->uri),
					     id,
					     qformat,
					     (uint8_t *)request,
					     (uint16_t)strlen((gchar *)request),
					     CONFIRM_YES);
      g_mutex_unlock(sa->mutex);
      whiteboard_log_debug("Mutex %p UNlocked for SibAccess %p\n", sa->mutex, sa);
      if(retvalue < 0)
	{
	  whiteboard_log_debug("Sending Subscribe_Req failed: %d\n", retvalue);
	}
      else
	{
	  // block until we receive confirmation or timeout
	  retvalue = sib_access_wait_for_confirm(sa, id);
	  if(retvalue < 0)
	    {
	      whiteboard_log_debug("Error while waiting respose for Subscribe\n");
	    }
	  else if(retvalue == 0)
	    {
	      whiteboard_log_debug("Subscribe response timeout\n");
	    }
	  else
	    {
	      whiteboard_log_debug("Got Subscribe response\n");
	    }
	  sib_access_remove_request(sa,id);
	}
    }
  
  whiteboard_log_debug_fe();
  return retvalue;

}

gint sib_access_wait_for_subscription_ind(SIBAccess *sa, guchar *id,  NodeMsgContent_t **msg)
{
  whiteboard_log_debug_fb();
  g_return_val_if_fail( sa != NULL, -1);
  
  if( sib_access_add_subscription(sa, id, msg) < 0)
    return -1;
  
  // just block until we get indication
  sib_access_wait_for_indication(sa, id);
  
  sib_access_remove_subscription(sa, id);
  
  whiteboard_log_debug_fe();
  return 0;
}

gint sib_access_unsubscribe(SIBAccess *sa, guchar *nodeid, gint msgnumber, guchar *request)
{
  gint id;
  gint retvalue = -1;
  RequestData *rd = NULL;
  whiteboard_log_debug_fb();

  sib_access_msgnumber_lock(sa);
  id = ++sa->msgnumber;
  sib_access_msgnumber_unlock(sa);

  if( sib_access_add_request(sa, id, NULL) < 0 )
    {
      whiteboard_log_debug("Could not add request (msgnumber: %d) to hashtable\n", id);
    }
  else
    {
      rd = sib_access_get_and_lock_request_data( sa, id);
      rd->subscription_id = (guchar *)g_strdup((gchar *)request);
      sib_access_unlock_request(sa,id);
      
      g_mutex_lock(sa->mutex);
      
      whiteboard_log_debug("Mutex %p locked for SibAccess %p\n", sa->mutex, sa);
      retvalue= SibAccessNota_Unsubscribe_req( sa->context,
					       (uint8_t *)nodeid,
					       (uint16_t)strlen((gchar *)nodeid),
					       (uint8_t *) sa->uri,
					       (uint16_t)strlen((gchar *)sa->uri),
					       id,
					       (uint8_t *)request,
					       (uint16_t)strlen((gchar *)request));
      
      g_mutex_unlock(sa->mutex);
      whiteboard_log_debug("Mutex %p UNlocked for SibAccess %p\n", sa->mutex, sa);
      if(retvalue < 0)
	{
	  whiteboard_log_debug("Sending Unsubscribe_Req failed: %d\n", retvalue);
	}
      else
	{
	  // block until we receive confirmation or timeout
	  retvalue = sib_access_wait_for_confirm(sa, id);
	  if(retvalue < 0)
	    {
	      whiteboard_log_debug("Error while waiting respose for unsubscribe\n");
	    }
	  else if(retvalue == 0)
	    {
	      whiteboard_log_debug("Unsubscribe response timeout\n");
	    }
	  else
	    {
	      whiteboard_log_debug("Got Unsubscribe response\n");
	    }
	  sib_access_remove_request(sa,id);
	}
    }
  whiteboard_log_debug_fe();
  return retvalue;
}

static gint sib_access_create_socket(SIBAccess *self)
{
  nota_addr_t addr = { self->sid, self->port};
  whiteboard_log_debug_fb();
  /* connect to service */
  g_return_val_if_fail(self->core != NULL, -1);
  
  self->fd = Hsocket(self->core, AF_NOTA, SOCK_STREAM, 0);

  g_return_val_if_fail(self->fd>0, -1);
  //  sib_maxfd_set(sib_controller_get_maxfd(self->cp), self->fd);
  
  
  if( Hconnect(self->core, self->fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
      whiteboard_log_debug("Could not connect to socket %d\n", self->fd);
      Hclose(self->core, self->fd);
      self->fd = -1;
      return -1;
    }
  whiteboard_log_debug("Socket (%d) connected to sid: %d\n", self->fd, self->sid);
  self->context = nota_stub_context_pointer_initialize(self->core, self->fd);
  if(!self->context)
    {
      whiteboard_log_debug("Could not create context pointer\n");
      Hclose(self->core, self->fd);
      self->fd = -1;
      self->context = NULL;
      return -1;
    }
  whiteboard_log_debug("Context inialized\n");
  if( SibAccessNota_user_new_connection(self->context) < 0)
    {
      whiteboard_log_debug("could not add to stub\n");
      nota_stub_context_pointer_free(self->context);
      Hclose(self->core, self->fd);
      self->fd = -1;
      self->context = NULL;
      return -1;
    }
  
  self->context->user_pointer = self;
  whiteboard_log_debug("Stub connected\n");
  whiteboard_log_debug_fe();
  return 0;
}

static void sib_access_requests_lock(SIBAccess *self)
{
  g_return_if_fail(self!=NULL);
  g_return_if_fail(self->request_mutex!=NULL);
  g_mutex_lock(self->request_mutex);
}
static void sib_access_requests_unlock(SIBAccess *self)
{
  g_return_if_fail(self!=NULL);
  g_return_if_fail(self->request_mutex!=NULL);
  g_mutex_unlock(self->request_mutex);
}


static void sib_access_msgnumber_lock(SIBAccess *self)
{
  g_return_if_fail(self!=NULL);
  g_return_if_fail(self->msgnumber_lock!=NULL);
  g_mutex_lock(self->msgnumber_lock);
}
static void sib_access_msgnumber_unlock(SIBAccess *self)
{
  g_return_if_fail(self!=NULL);
  g_return_if_fail(self->msgnumber_lock!=NULL);
  g_mutex_unlock(self->msgnumber_lock);
}

static gint sib_access_add_request(SIBAccess *self, gint msgNumber, NodeMsgContent_t **msg)
{
  RequestData *rd = NULL;
  whiteboard_log_debug_fb();
  g_return_val_if_fail(self != NULL, -1);
  sib_access_requests_lock(self);
  
  if( g_hash_table_lookup(self->requests, GINT_TO_POINTER(msgNumber)) == NULL)
    {
      rd = g_new0(RequestData,1);
      rd->msg = msg;
      rd->cond = g_cond_new();
      rd->mutex = g_mutex_new();
      rd->completed = FALSE;
      g_hash_table_insert(self->requests, GINT_TO_POINTER(msgNumber), (gpointer) rd);

      whiteboard_log_debug("Added RequestData w/ msgnumber %d\n", msgNumber);
    }
  
  sib_access_requests_unlock(self);
  whiteboard_log_debug_fe();
  return (rd == NULL ? -1:0);
}

static RequestData *sib_access_get_and_lock_request_data(SIBAccess *self, gint msgNumber)
{
  RequestData *rd = NULL;
  whiteboard_log_debug_fb();
  sib_access_requests_lock(self);
  rd = (RequestData *)g_hash_table_lookup(self->requests, GINT_TO_POINTER(msgNumber));
  sib_access_requests_unlock(self);
  
  if(rd)
    {
      g_mutex_lock(rd->mutex);
    }
  else
    {
      whiteboard_log_debug("RequestData not found w/ msgnumber %d\n", msgNumber);
    }

  whiteboard_log_debug_fe();
  return rd;
}

static NodeMsgContent_t **sib_access_get_and_lock_request(SIBAccess *self, gint msgNumber)
{
  RequestData *rd = NULL;
  NodeMsgContent_t **msg = NULL;
  whiteboard_log_debug_fb();
  sib_access_requests_lock(self);
  rd = (RequestData *)g_hash_table_lookup(self->requests, GINT_TO_POINTER(msgNumber));
  sib_access_requests_unlock(self);
  
  if(rd)
    {
      g_mutex_lock(rd->mutex);
      msg = rd->msg;
    }
  else
    {
      whiteboard_log_debug("RequestData not found w/ msgnumber %d\n", msgNumber);
    }

  whiteboard_log_debug_fe();
  return msg;
}

static gint sib_access_wait_for_confirm(SIBAccess *self, gint msgNumber)
{
  gint ret = -1;
  RequestData *rd = NULL;
  whiteboard_log_debug_fb();
  sib_access_requests_lock(self);
  rd = (RequestData *)g_hash_table_lookup(self->requests, GINT_TO_POINTER(msgNumber));
  sib_access_requests_unlock(self);

  if(rd)
    {
      GTimeVal timeout;
      g_get_current_time(&timeout);
      g_time_val_add( &timeout, REQUEST_TIMEOUT_USEC );

      g_mutex_lock( rd->mutex);
      
      while(!rd->completed && ( ret == -1))
	ret = ( g_cond_timed_wait(rd->cond, rd->mutex, &timeout) ? 1:0 ); 
      
      if(rd->completed)
	ret = 1;

      g_mutex_unlock(rd->mutex);
    }
  else
    {
      whiteboard_log_debug("No RequestData with msgnumber %d\n", msgNumber);
    }
  whiteboard_log_debug_fe();
  return ret;
}

static void sib_access_unlock_request(SIBAccess *self, gint msgNumber)
{
  RequestData *rd = NULL;
  whiteboard_log_debug_fb();
  sib_access_requests_lock(self);
  rd = (RequestData *)g_hash_table_lookup(self->requests, GINT_TO_POINTER(msgNumber));
  if(!rd)
    {
      whiteboard_log_debug("RequestData not found w/ msgnumber %d\n", msgNumber);
    }
  else
    {
      rd->completed = TRUE;
      g_mutex_unlock(rd->mutex);
    }
  sib_access_requests_unlock(self);
  whiteboard_log_debug_fe();
}

static void sib_access_complete_and_unlock_request(SIBAccess *self, gint msgNumber)
{
  RequestData *rd = NULL;
  whiteboard_log_debug_fb();
  sib_access_requests_lock(self);
  rd = (RequestData *)g_hash_table_lookup(self->requests, GINT_TO_POINTER(msgNumber));
  if(!rd)
    {
      whiteboard_log_debug("RequestData not found w/ msgnumber %d\n", msgNumber);
    }
  else
    {
      rd->completed = TRUE;
      g_cond_signal(rd->cond);
      g_mutex_unlock(rd->mutex);
    }
  sib_access_requests_unlock(self);
  whiteboard_log_debug_fe();
}

static void sib_access_remove_request(SIBAccess *self, gint msgNumber)
{
  RequestData *rd = NULL;
  whiteboard_log_debug_fb();
  sib_access_requests_lock(self);
  rd = (RequestData *)g_hash_table_lookup(self->requests,  GINT_TO_POINTER(msgNumber)) ;
  if(!rd)
    {
      whiteboard_log_debug("RequestData not found\n");
    }
  else
    {
      g_hash_table_remove(self->requests, GINT_TO_POINTER(msgNumber));
      g_cond_free(rd->cond);

      if(rd->subscription_id)
	g_free(rd->subscription_id);
      
      g_free(rd);
    }
  sib_access_requests_unlock(self);
  whiteboard_log_debug_fe();
}

static void sib_access_subscriptions_lock(SIBAccess *self)
{
  g_return_if_fail(self!=NULL);
  g_return_if_fail(self->sub_mutex!=NULL);
  g_mutex_lock(self->sub_mutex);
}
static void sib_access_subscriptions_unlock(SIBAccess *self)
{
  g_return_if_fail(self!=NULL);
  g_return_if_fail(self->sub_mutex!=NULL);
  g_mutex_unlock(self->sub_mutex);
}

static gint sib_access_add_subscription(SIBAccess *self, guchar * id, NodeMsgContent_t **msg)
{
  RequestData *rd = NULL;
  whiteboard_log_debug_fb();
  g_return_val_if_fail(self != NULL, -1);
  sib_access_subscriptions_lock(self);
  
  if( g_hash_table_lookup(self->subscriptions, id ) == NULL)
    {
      rd = g_new0(RequestData,1);
      rd->msg = msg;
      rd->cond = g_cond_new();
      rd->mutex = g_mutex_new();
      g_hash_table_insert(self->subscriptions, g_strdup( (gchar *)id), (gpointer) rd);

      whiteboard_log_debug("Added RequestData w/ msgnumber %s\n", id);
    }
  else
    {
      whiteboard_log_debug("RequestData w/ msgnumber %s already exists\n", id);
    }
  
  sib_access_subscriptions_unlock(self);
  whiteboard_log_debug_fe();
  return (rd == NULL ? -1:0);
}

static NodeMsgContent_t **sib_access_get_and_lock_subscription(SIBAccess *self, guchar *id)
{
  RequestData *rd = NULL;
  NodeMsgContent_t **msg = NULL;
  whiteboard_log_debug_fb();
  sib_access_subscriptions_lock(self);
  rd = (RequestData *)g_hash_table_lookup(self->subscriptions, id);
  sib_access_subscriptions_unlock(self);
  
  if(rd)
    {
      g_mutex_lock(rd->mutex);
      msg = rd->msg;
    }
  else
    {
      whiteboard_log_debug("RequestData not found w/ id %s\n", id);
    }

  whiteboard_log_debug_fe();
  return msg;
}

static gint sib_access_wait_for_indication(SIBAccess *self, guchar *id)
{
  gint ret = -1;
  RequestData *rd = NULL;
  whiteboard_log_debug_fb();
  sib_access_subscriptions_lock(self);
  rd = (RequestData *)g_hash_table_lookup(self->subscriptions, id);
  sib_access_subscriptions_unlock(self);

  if(rd)
    {
      g_mutex_lock( rd->mutex);
      g_cond_wait(rd->cond, rd->mutex);
      //while(!rd->completed && ( ret == -1))
      //	ret = ( g_cond_wait(rd->cond, rd->mutex) ? 1:0 ); 
      
      g_mutex_unlock(rd->mutex);
    }
  else
    {
      whiteboard_log_debug("No RequestData with id %s\n", id);
    }
  whiteboard_log_debug_fe();
  return ret;
}

static void sib_access_complete_and_unlock_subscription(SIBAccess *self, guchar *id)
{
  RequestData *rd = NULL;
  whiteboard_log_debug_fb();
  sib_access_subscriptions_lock(self);
  rd = (RequestData *)g_hash_table_lookup(self->subscriptions, id);
  if(!rd)
    {
      whiteboard_log_debug("RequestData not found w/ id %s\n", id );
    }
  else
    {
      rd->completed = TRUE;
      
      g_cond_signal(rd->cond);
      g_mutex_unlock(rd->mutex);
    }
  sib_access_subscriptions_unlock(self);
  whiteboard_log_debug_fe();
}

static void sib_access_remove_subscription(SIBAccess *self, guchar *id)
{
  RequestData *rd = NULL;
  whiteboard_log_debug_fb();
  sib_access_subscriptions_lock(self);
  rd = (RequestData *)g_hash_table_lookup(self->subscriptions, id ) ;
  if(!rd)
    {
      whiteboard_log_debug("RequestData not found\n");
    }
  else
    {
      g_hash_table_remove(self->subscriptions, id);
      g_cond_free(rd->cond);
      g_free(rd);
    }
  sib_access_subscriptions_unlock(self);
  whiteboard_log_debug_fe();
}

// Skeleton prototypes. User must implement these functions.
void SibAccessNota_Join_cnf_process( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, sibstatus_t status, uint8_t* credentials, uint16_t credentials_len)
{
  SIBAccess *self = (SIBAccess*)context->user_pointer;
  NodeMsgContent_t *msg = NULL;
  NodeMsgContent_t **rsp = NULL;
  whiteboard_log_debug_fb();
  g_return_if_fail(self != NULL);
  
  //msg = sib_access_get_and_lock_request(self, msgnum);
  msg = parseSSAPmsg_new();
  if(msg)
    {
      msg->nodeid_MSTR = g_strndup( (gchar *)nodeid, nodeid_len);
      msg->spaceid_MSTR = g_strndup( (gchar *)spaceid, spaceid_len);
      msg->msgNumber = msgnum;
      if(status == SSStatus_Success)
	{
	  msg->status = MSG_E_OK;
	}
      else
	{
	  msg->status = MSG_E_NOK;
	}
      msg->name = MSG_N_JOIN;
      msg->type = MSG_T_CNF;
      rsp = sib_access_get_and_lock_request(self, msgnum);
      if(rsp != NULL)
	{
	  *rsp=msg;
	  sib_access_complete_and_unlock_request(self,msgnum);
	}
      else
	{
	  whiteboard_log_debug("Could not find message to fill\n");
	}
      
    }
  else
    {
      whiteboard_log_debug("Could not allocate memory for reply message\n");
    }
  whiteboard_log_debug_fe();
}


void SibAccessNota_Insert_cnf_process( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, sibstatus_t status, uint8_t* nodelist, uint16_t nodelist_len )
{
  SIBAccess *self = (SIBAccess*)context->user_pointer;
  NodeMsgContent_t *msg = NULL;
  NodeMsgContent_t **rsp = NULL;
  whiteboard_log_debug_fb();
  g_return_if_fail(self != NULL);
  //msg = sib_access_get_and_lock_request(self, msgnum);
  msg = parseSSAPmsg_new();
  if(msg)
    {
      msg->nodeid_MSTR = g_strndup( (gchar *)nodeid, nodeid_len);
      msg->spaceid_MSTR = g_strndup( (gchar *)spaceid, spaceid_len);
      msg->m3XML_MSTR = g_strndup( (gchar *) nodelist, nodelist_len);
      msg->msgNumber = msgnum;
      if(status == SSStatus_Success)
	{
	  msg->status = MSG_E_OK;
	}
      else
	{
	  msg->status = MSG_E_NOK;
	}
      msg->name = MSG_N_INSERT;
      msg->type = MSG_T_CNF;
      rsp = sib_access_get_and_lock_request(self, msgnum);
      if(rsp != NULL)
	{
	  *rsp=msg;
	  sib_access_complete_and_unlock_request(self,msgnum);
	}
      else
	{
	  whiteboard_log_debug("Could not find message to fill\n");
	}
    }
  else
    {
      whiteboard_log_debug("Could not allocate memory for reply message\n");
    }
  whiteboard_log_debug_fe();
}

void SibAccessNota_Update_cnf_process( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, sibstatus_t status, uint8_t* bNodes, uint16_t bNodes_len )
{
  SIBAccess *self = (SIBAccess*)context->user_pointer;
  NodeMsgContent_t *msg = NULL;
  NodeMsgContent_t **rsp = NULL;
  whiteboard_log_debug_fb();
  g_return_if_fail(self != NULL);
  //msg = sib_access_get_and_lock_request(self, msgnum);
  msg = parseSSAPmsg_new();
  if(msg)
    {
      msg->nodeid_MSTR = g_strndup( (gchar *)nodeid, nodeid_len);
      msg->spaceid_MSTR = g_strndup( (gchar *)spaceid, spaceid_len);
      msg->m3XML_MSTR = g_strndup( (gchar *) bNodes, bNodes_len);
      msg->msgNumber = msgnum;
      if(status == SSStatus_Success)
	{
	  whiteboard_log_debug("status ok\n");
	  msg->status = MSG_E_OK;
	}
      else
	{
	  whiteboard_log_debug("status error\n");
	  msg->status = MSG_E_NOK;
	}
      msg->name = MSG_N_UPDATE;
      msg->type = MSG_T_CNF;
      rsp = sib_access_get_and_lock_request(self, msgnum);
      if(rsp != NULL)
	{
	  *rsp=msg;
	  sib_access_complete_and_unlock_request(self,msgnum);
	}
      else
	{
	  whiteboard_log_debug("Could not find message to fill\n");
	}
    }
  else
    {
      whiteboard_log_debug("Could not allocate memory for reply message\n");
    }
  whiteboard_log_debug_fe();
}

void SibAccessNota_Remove_cnf_process( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, sibstatus_t status )
{
    SIBAccess *self = (SIBAccess*)context->user_pointer;
  NodeMsgContent_t *msg = NULL;
  NodeMsgContent_t **rsp = NULL;
  whiteboard_log_debug_fb();
  g_return_if_fail(self != NULL);
  msg = parseSSAPmsg_new();
  if(msg)
    {
      msg->nodeid_MSTR = g_strndup( (gchar *)nodeid, nodeid_len);
      msg->spaceid_MSTR = g_strndup( (gchar *)spaceid, spaceid_len);
      msg->msgNumber = msgnum;
      if( status == SSStatus_Success)
	{
	  msg->status = MSG_E_OK;
	}
      else
	{
	  msg->status = MSG_E_NOK;
	}
      msg->name = MSG_N_REMOVE;
      msg->type = MSG_T_CNF;

      rsp = sib_access_get_and_lock_request(self, msgnum);
      if(rsp != NULL)
	{
	  *rsp= msg;
	  sib_access_complete_and_unlock_request(self,msgnum);
	}
      else
	{
	  whiteboard_log_debug("Could not find message to fill\n");
	}
    }
  else
    {
      whiteboard_log_debug("Could not allocate memory for reply message\n");
    }
  
  whiteboard_log_debug_fe();
}

void SibAccessNota_Query_cnf_process( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, sibstatus_t status, uint8_t* results, uint16_t results_len )
{
  SIBAccess *self = (SIBAccess*)context->user_pointer;
  NodeMsgContent_t *msg = NULL;
  NodeMsgContent_t **rsp = NULL;
  whiteboard_log_debug_fb();
  g_return_if_fail(self != NULL);
  msg = parseSSAPmsg_new();
  if(msg)
    {
      msg->nodeid_MSTR = g_strndup( (gchar *)nodeid, nodeid_len);
      msg->spaceid_MSTR = g_strndup( (gchar *)spaceid, spaceid_len);
      msg->m3XML_MSTR = ( (results != NULL) ? g_strndup( (gchar *) results, results_len) : NULL);

      msg->msgNumber = msgnum;
      if(status == SSStatus_Success)
	{
	  msg->status = MSG_E_OK;
	}
      else
	{
	  msg->status = MSG_E_NOK;
	}
      msg->name = MSG_N_QUERY;
      msg->type = MSG_T_CNF;

      rsp = sib_access_get_and_lock_request(self, msgnum);
      if(rsp != NULL)
	{
	  *rsp= msg;
	  sib_access_complete_and_unlock_request(self,msgnum);
	}
      else
	{
	  whiteboard_log_debug("Could not find message to fill\n");
	}
    }
  else
    {
      whiteboard_log_debug("Could not allocate memory for reply message\n");
    }
  
  whiteboard_log_debug_fe();

}

void SibAccessNota_Subscribe_cnf_process( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, sibstatus_t status, uint8_t* subscription_id, uint16_t subscription_id_len, uint8_t* results, uint16_t results_len )
{
  SIBAccess *self = (SIBAccess*)context->user_pointer;
  NodeMsgContent_t *msg = NULL;
  NodeMsgContent_t **rsp = NULL;
  whiteboard_log_debug_fb();
  g_return_if_fail(self != NULL);
  msg = parseSSAPmsg_new();
  if(msg)
    {
      msg->nodeid_MSTR = g_strndup( (gchar *)nodeid, nodeid_len);
      msg->spaceid_MSTR = g_strndup( (gchar *)spaceid, spaceid_len);
      msg->m3XML_MSTR = ( (results != NULL) ? g_strndup( (gchar *) results, results_len) : NULL);
      msg->queryids_MSTR = ( (subscription_id != NULL) ? g_strndup( (gchar *) subscription_id, subscription_id_len) : NULL);
      msg->msgNumber = msgnum;
      if(status == SSStatus_Success)
	{
	  msg->status = MSG_E_OK;
	}
      else
	{
	  msg->status = MSG_E_NOK;
	}
      msg->name = MSG_N_SUBSCRIBE;
      msg->type = MSG_T_CNF;

      rsp = sib_access_get_and_lock_request(self, msgnum);
      if(rsp != NULL)
	{
	  *rsp= msg;
	  sib_access_complete_and_unlock_request(self,msgnum);
	}
      else
	{
	  whiteboard_log_debug("Could not find message to fill\n");
	}
    }
  else
    {
      whiteboard_log_debug("Could not allocate memory for reply message\n");
    }
  
  whiteboard_log_debug_fe();
}

void SibAccessNota_Unsubscribe_cnf_process( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, sibstatus_t status )
{
  SIBAccess *self = (SIBAccess*)context->user_pointer;
  NodeMsgContent_t *msg = NULL;
  NodeMsgContent_t **rsp = NULL;
  RequestData *rd = NULL;
  whiteboard_log_debug_fb();
  g_return_if_fail(self != NULL);

  msg = parseSSAPmsg_new();
  if(msg)
    {
      msg->nodeid_MSTR = g_strndup( (gchar *)nodeid, nodeid_len);
      msg->spaceid_MSTR = g_strndup( (gchar *)spaceid, spaceid_len);
      //      msg->queryids_MSTR = ( (subscription_id != NULL) ? g_strndup( (gchar *) subscription_id, subscription_id_len) : NULL);
      msg->msgNumber = msgnum;
      msg->status = MSG_E_OK;
      msg->name = MSG_N_UNSUBSCRIBE;
      msg->type = MSG_T_CNF;

      whiteboard_log_debug("Unsubscribe_cnf w/ msgnum %d\n", msgnum);
      rd = sib_access_get_and_lock_request_data(self, msgnum);
      if(rd != NULL)
	{
	  whiteboard_log_debug("GOT Unsubscribe_cnf for subscription_id %s\n", rd->subscription_id);
	  rsp = sib_access_get_and_lock_subscription(self, rd->subscription_id);
	  if(rsp != NULL)
	    {
	      msg->queryids_MSTR = g_strdup((gchar *) rd->subscription_id);
	      *rsp = msg;
	      sib_access_complete_and_unlock_subscription(self, rd->subscription_id);
	    }
	  else
	    {
	      // TODO: queue for received messages, if wait_for_subscription_ind has not been called yet.
	      whiteboard_log_debug("Could not find message to fill\n");
	    }
	  sib_access_complete_and_unlock_request(self, msgnum);
	}
      else
	{
	  whiteboard_log_debug("Could not find RequestData");
	}
    }
  else
    {
      whiteboard_log_debug("Could not allocate memory for reply message\n");
    }

  whiteboard_log_debug_fe();
}

void SibAccessNota_Leave_cnf_process( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, sibstatus_t status )
{

  SIBAccess *self = (SIBAccess*)context->user_pointer;
  NodeMsgContent_t *msg = NULL;
  NodeMsgContent_t **rsp = NULL;
  whiteboard_log_debug_fb();
  g_return_if_fail(self != NULL);
  msg = parseSSAPmsg_new();
  if(msg)
    {
      msg->nodeid_MSTR = g_strndup( (gchar *)nodeid, nodeid_len);
      msg->spaceid_MSTR = g_strndup( (gchar *)spaceid, spaceid_len);
      msg->msgNumber = msgnum;
      if(status == SSStatus_Success)
	{
	  msg->status = MSG_E_OK;
	}
      else
	{
	  msg->status = MSG_E_NOK;
	}
      msg->name = MSG_N_LEAVE;
      msg->type = MSG_T_CNF;

      rsp = sib_access_get_and_lock_request(self, msgnum);
      if(rsp != NULL)
	{
	  *rsp= msg;
	  sib_access_complete_and_unlock_request(self,msgnum);
	}
      else
	{
	  whiteboard_log_debug("Could not find message to fill\n");
	}
    }
  else
    {
      whiteboard_log_debug("Could not allocate memory for reply message\n");
    }
  
  whiteboard_log_debug_fe();

}

void SibAccessNota_Subscription_ind_process( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, uint32_t seqnum, uint8_t* subscription_id, uint16_t subscription_id_len, uint8_t* results_added, uint16_t results_added_len, uint8_t* results_removed, uint16_t results_removed_len )
{
  SIBAccess *self = (SIBAccess*)context->user_pointer;
  NodeMsgContent_t *msg = NULL;
  NodeMsgContent_t **rsp = NULL;
  whiteboard_log_debug_fb();
  g_return_if_fail(self != NULL);
  msg = parseSSAPmsg_new();
  if(msg)
    {
      msg->nodeid_MSTR = g_strndup( (gchar *)nodeid, nodeid_len);
      msg->spaceid_MSTR = g_strndup( (gchar *)spaceid, spaceid_len);
      msg->m3XML_MSTR = ( (results_added != NULL) ? g_strndup( (gchar *) results_added, results_added_len) : NULL);
      msg->removedResults_MSTR = ( (results_removed != NULL) ? g_strndup( (gchar *) results_removed, results_removed_len) : NULL);
      msg->queryids_MSTR = ( (subscription_id != NULL) ? g_strndup( (gchar *) subscription_id, subscription_id_len) : NULL);
      msg->msgNumber = msgnum;
      msg->indUpdateSequence = seqnum;
      msg->status = MSG_E_OK;
      msg->name = MSG_N_SUBSCRIBE;
      msg->type = MSG_T_IND;
      whiteboard_log_debug("Got SubscriptionIND:\n");
      whiteboard_log_debug("NodeID: %s\n",msg->nodeid_MSTR);
      whiteboard_log_debug("SpaceID: %s\n",msg->spaceid_MSTR);
      whiteboard_log_debug("msgNum: %d\n",msg->msgNumber);
      whiteboard_log_debug("seqNum: %d\n",msg->indUpdateSequence);
      whiteboard_log_debug("Results_added: %s\n",  msg->m3XML_MSTR);
      whiteboard_log_debug("Results_obs: %s\n",  msg->removedResults_MSTR);
      whiteboard_log_debug("SubscriptionID: %s\n",  msg->queryids_MSTR);
      
      rsp = sib_access_get_and_lock_subscription(self, (guchar *)msg->queryids_MSTR);
      if(rsp != NULL)
	{
	  *rsp = msg;
	  sib_access_complete_and_unlock_subscription(self, (guchar *)msg->queryids_MSTR);
	}
      else
	{
	 // TODO: queue for received messages, if wait_for_subscription_ind has not been called yet.
	  whiteboard_log_debug("Could not find message to fill\n");
	}
    }
  else
    {
      whiteboard_log_debug("Could not allocate memory for reply message\n");
    }

  whiteboard_log_debug_fe();
}

void SibAccessNota_Unsubscribe_ind_process( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, sibstatus_t status, uint8_t* subscription_id, uint16_t subscription_id_len )
{
  SIBAccess *self = (SIBAccess*)context->user_pointer;
  NodeMsgContent_t *msg = NULL;
  NodeMsgContent_t **rsp = NULL;
  whiteboard_log_debug_fb();
  g_return_if_fail(self != NULL);
  msg = parseSSAPmsg_new();
  if(msg)
    {
      msg->nodeid_MSTR = g_strndup( (gchar *)nodeid, nodeid_len);
      msg->spaceid_MSTR = g_strndup( (gchar *)spaceid, spaceid_len);
      msg->queryids_MSTR = ( (subscription_id != NULL) ? g_strndup( (gchar *) subscription_id, subscription_id_len) : NULL);
      msg->msgNumber = msgnum;
      if(status == SSStatus_Success)
	{
	  msg->status = MSG_E_OK;
	}
      else
	{
	  msg->status = MSG_E_NOK;
	}
      
      msg->name = MSG_N_UNSUBSCRIBE;
      msg->type = MSG_T_IND;
      
      rsp = sib_access_get_and_lock_subscription(self, (guchar *)msg->queryids_MSTR);
      if(rsp != NULL)
	{
	  *rsp = msg;
	  sib_access_complete_and_unlock_subscription(self, (guchar *)msg->queryids_MSTR);
	}
      else
	{
	 // TODO: queue for received messages, if wait_for_subscription_ind has not been called yet.
	  whiteboard_log_debug("Could not find message to fill\n");
	}
    }
  else
    {
      whiteboard_log_debug("Could not allocate memory for reply message\n");
    }

  whiteboard_log_debug_fe();
}  


void SibAccessNota_Leave_ind_process( struct context_pointer* context, uint8_t* nodeid, uint16_t nodeid_len, uint8_t* spaceid, uint16_t spaceid_len, uint32_t msgnum, sibstatus_t status )
{
  SIBAccess *self = (SIBAccess*)context->user_pointer;
  NodeMsgContent_t *msg = NULL;
  NodeMsgContent_t **rsp = NULL;
  whiteboard_log_debug_fb();
  g_return_if_fail(self != NULL);
  msg = parseSSAPmsg_new();
  if(msg)
    {
      msg->nodeid_MSTR = g_strndup( (gchar *)nodeid, nodeid_len);
      msg->spaceid_MSTR = g_strndup( (gchar *)spaceid, spaceid_len);
      msg->msgNumber = msgnum;
      if(status == SSStatus_Success)
	{
	  msg->status = MSG_E_OK;
	}
      else
	{
	  msg->status = MSG_E_NOK;
	}
      
      msg->name = MSG_N_LEAVE;
      msg->type = MSG_T_IND;
      
      rsp = sib_access_get_and_lock_subscription(self, (guchar *)msg->queryids_MSTR);
      if(rsp != NULL)
	{
	  *rsp = msg;
	  sib_access_complete_and_unlock_subscription(self, (guchar *)msg->queryids_MSTR);
	}
      else
	{
	 // TODO: queue for received messages, if wait_for_subscription_ind has not been called yet.
	  whiteboard_log_debug("Could not find message to fill\n");
	}
    }
  else
    {
      whiteboard_log_debug("Could not allocate memory for reply message\n");
    }

  whiteboard_log_debug_fe();
}  

void SibAccessNota_user_handler_disconnected(struct context_pointer* context)
{
  whiteboard_log_debug_fb();
  whiteboard_log_debug("disconnected\n");
  whiteboard_log_debug_fe();
}

void SibAccessNota_user_handler_error(struct context_pointer* context, int error_type)
{
  whiteboard_log_debug_fb();
  whiteboard_log_debug("error_type: %d\n", error_type);
  whiteboard_log_debug_fe();
}

			    
