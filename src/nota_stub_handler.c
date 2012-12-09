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
#include <glib.h>
#include <whiteboard_log.h>
#include "nota_stub_handler.h"

typedef enum  {
  AddConnection = 0,
  RemoveConnection,
  Quit
} RequestType;

typedef struct _StubData
{
  int fd;
  struct context_pointer *cp;
} StubData;

typedef struct _Request
{
  RequestType type;
  StubData *data;
} Request;

struct _NotaStubHandler
{
  GThread *receiver;
  GAsyncQueue *queue;
  GAsyncQueue *rspqueue;
  GHashTable *connections; // fd->stub_data
  fd_set read_s;
  fd_set error_s;
  GSList *errors;

};

static StubData *stub_data_new(int fd, struct context_pointer *cp);
static Request *request_new(RequestType type, StubData *data );
static void check_fdsets(gpointer key, gpointer value, gpointer userdata);
static gpointer nota_stub_handler_receiver(gpointer data);
static void close_connection( gpointer _key, gpointer _value, gpointer userdata);

NotaStubHandler *nota_stub_handler_new()
{
  NotaStubHandler *self = g_try_new0(NotaStubHandler,1);
  if(!self)
    {
      whiteboard_log_debug("nota_stub_handler_new(): memory allocation error\n");
    }
  else
    {
      self->connections = g_hash_table_new(g_direct_hash, g_direct_equal);
      self->queue = g_async_queue_new();
      self->rspqueue = g_async_queue_new();
      self->receiver = NULL;
    }
  return self;
}

void nota_stub_handler_destroy(NotaStubHandler *self)
{
  
  g_hash_table_foreach (self->connections, close_connection, self);

  g_hash_table_destroy(self->connections);
  g_async_queue_unref(self->queue);      
  g_async_queue_unref(self->rspqueue);	  
  g_free(self);
}

void close_connection( gpointer _key, gpointer _value, gpointer userdata)
{
  whiteboard_log_debug_fb();
  int fd = GPOINTER_TO_INT(_key);
  NotaStubHandler *stub = (NotaStubHandler *)userdata;
  Hclose(Hgetinstance(), fd);
  whiteboard_log_debug_fe();
}

gint nota_stub_handler_add_connection(NotaStubHandler *self, int fd, struct context_pointer *cp)
{
  Request *req = NULL;
  StubData *sd = NULL;
  GError *gerr = NULL;
  if(!self->receiver)
    {
      self->receiver = g_thread_create( nota_stub_handler_receiver, 
					self,
					FALSE,
					&gerr);
      if(gerr)
	{
	  whiteboard_log_debug("nota_stub_handler_add_connection(): Could not create receiver thread, err: %s\n", gerr->message);
	  g_error_free(gerr);
	  return -1;
	}
    }
  
  if(g_hash_table_lookup( self->connections, GINT_TO_POINTER(fd) ) == NULL)
    {
      sd = stub_data_new(fd, cp);
      if(sd)
	{
	  req = request_new(AddConnection, sd);
	  if(!req)
	    {
	      g_free(sd);
	      return -1;
	    }
	  else 
	    {
	      g_async_queue_push(self->queue, (gpointer *)req);
	      whiteboard_log_debug("nota_stub_handler_add_connection: Add connection request sent\n");
	      if( g_async_queue_pop(self->rspqueue) != NULL)
		{
		  whiteboard_log_debug("nota_stub_handler_add_connection: got response to add connection request\n");
		}
	    }
	}
      else
	{
	  return -1;
	}
    }
  else
    {
      return -1;
    }

  return 0;
}

gint nota_stub_handler_remove_connection(NotaStubHandler *self, int fd)
{
  Request *req = NULL;
  StubData *sd = NULL;
  struct context_pointer *cp = NULL;
  if(self->receiver)
    {
      if( (cp = g_hash_table_lookup( self->connections, GINT_TO_POINTER(fd) )) != NULL)
#if 0
	{
	  sd = stub_data_new(fd, cp);
	  if(sd)	    
	    {
	      req = request_new(RemoveConnection, sd);
	      if(!req)
		{
		  g_free(sd);
		  return -1;
		}
	      else 
		{
		  g_async_queue_push(self->queue, (gpointer *)req);
		  whiteboard_log_debug("nota_stub_handler_remove_connection: Add connection request sent\n");
		  if( g_async_queue_pop(self->rspqueue) != NULL)
		    {
		      whiteboard_log_debug("nota_stub_handler_add_connection: got response to remove connection request\n");
		    }
		}
	    }
	  else
	    {
	      return -1;
	    }
	}	  
#else
      {
	whiteboard_log_debug("nota_stub_handler_remove_connection: Closing socket %d\n",fd);
	Hclose(Hgetinstance(), fd);
      }
#endif
      else
	{
	  whiteboard_log_debug("nota_stub_handler_remove_connection(): connetion w/ fd: %d not found\n", fd);
	  return -1;
	}
    }
  else
    {
      whiteboard_log_debug("nota_stub_handler_remove_connection(): receiver not on\n");
      return -1;
    }
  return 0;
}


gpointer nota_stub_handler_receiver(gpointer data)
{
  NotaStubHandler *self = (NotaStubHandler *)data;
  Request* req = NULL;
  int maxfd=-1;
  h_in_t *core = Hgetinstance();
  int err;
  struct timeval tv;
  GSList *open_sockets = NULL;
  
  self->errors = NULL;
  
  tv.tv_sec = 0;
  tv.tv_usec = 100;
  g_async_queue_ref(self->queue);
  while(1)
    {
      FD_ZERO(&self->read_s);
      FD_ZERO(&self->error_s);      
      if( (req = g_async_queue_try_pop(self->queue) ) != NULL)
	{
	  if(req->type == AddConnection)
	    {
	      g_hash_table_insert(self->connections, GINT_TO_POINTER(req->data->fd), (gpointer)(req->data->cp));
	      maxfd = req->data->fd;
	     open_sockets = g_slist_prepend(open_sockets, GINT_TO_POINTER(req->data->fd));
	     g_async_queue_push(self->rspqueue, GINT_TO_POINTER(1));
	      whiteboard_log_debug("Added fd: %d to select set\n",req->data->fd );
	    }
#if 0
	  else if(req->type == RemoveConnection)
	    {
	      struct context_pointer *cp = g_hash_table_lookup(self->connections, GINT_TO_POINTER(req->data->fd));
	      g_hash_table_remove(self->connections, GINT_TO_POINTER(req->data->fd));
	      whiteboard_log_debug("Removed fd: %d from select set\n",req->data->fd );
	      open_sockets = g_slist_remove(open_sockets, GINT_TO_POINTER(req->data->fd));
	      if(cp)
		{
		  whiteboard_log_debug("Removing stub_context for fd: %d \n",req->data->fd );
		  nota_stub_context_pointer_free(cp );
		}
	      Hclose(core, req->data->fd);
	      g_async_queue_push(self->rspqueue, GINT_TO_POINTER(1));
	    }
#endif
	  else if(req->type == Quit)
	    {
	      whiteboard_log_debug("nota_stub_handler_receiver(): Got QUIT request\n");
	      g_async_queue_push(self->rspqueue, GINT_TO_POINTER(1));
	      g_free(req->data);
	      g_free(req);
	      break;
	    }

	  else
	    {
	      whiteboard_log_debug("nota_stub_handler_receiver(): Invalid request\n");
	      break;
	    }
	  g_free(req->data);
	  g_free(req);
	}

      if( g_hash_table_size(self->connections) > 0)
	{
	  GSList *link = open_sockets;
	  while(link)
	    {
	      FD_SET( GPOINTER_TO_INT(link->data), &self->read_s);
	      FD_SET( GPOINTER_TO_INT(link->data),&self->error_s);
	      link = g_slist_next(link);
	    }
	  err = Hselect(core, maxfd + 1, &self->read_s, NULL, &self->error_s, &tv);
	  if(err < 0)
	    {
	      whiteboard_log_debug("Select, error: %d\n", err);
	      whiteboard_log_debug("%s\n",strerror(errno));
	      err = -1;
	      break;
	    }
	  else
	    {
	      g_hash_table_foreach(self->connections, &check_fdsets, self);
	      
	      while(self->errors)
		{
		  GSList *link = self->errors;
		  int errfd = GPOINTER_TO_INT(link->data);
		  struct context_pointer *cp = g_hash_table_lookup(self->connections, GINT_TO_POINTER(errfd) );
		  self->errors = g_slist_remove_link(self->errors, link);
		  if(cp)
		    {
		      nota_stub_context_pointer_free(cp);
		    }
		  g_hash_table_remove(self->connections, GINT_TO_POINTER(errfd));

		  Hclose(core, errfd);
		  whiteboard_log_debug("Error on  fd: %d\n",errfd );
		  open_sockets = g_slist_remove(open_sockets, GINT_TO_POINTER( errfd));
		}	      
	    }
	  //g_mutex_unlock(rm->mutex);
	  //usleep(10);
	  //g_thread_yield ();
	}
      else
	{
	  whiteboard_log_debug("No connections, exiting\n");
	}
    }
  
  //g_hash_table_destroy(self->connections);
  g_async_queue_unref(self->queue);
  //g_free(self);
  whiteboard_log_debug("Exiting nota_stub_handler_receiver\n");
  return NULL;
}

static void check_fdsets(gpointer key, gpointer value, gpointer userdata)
{
  NotaStubHandler *self=( NotaStubHandler *)userdata;
  int fd = GPOINTER_TO_INT(key);
  struct context_pointer *cp = (struct context_pointer *)value;

  if(FD_ISSET(fd, &self->error_s))
    {
      whiteboard_log_debug("Error in socket: %d\n",fd);
      self->errors = g_slist_prepend(self->errors, GINT_TO_POINTER(fd));
    }
  else if(FD_ISSET(fd, &self->read_s))
    {
      if( nota_stub_handle_incoming(cp) < 0)
	{
	  whiteboard_log_debug("Failed incoming data: %s\n", strerror(errno));
	  self->errors = g_slist_prepend(self->errors, GINT_TO_POINTER(fd));
	}
    }
}

static Request *request_new(RequestType type, StubData *data )
{
  Request *req = g_try_new0(Request,1);
  if(!req)
    {
      whiteboard_log_debug("request_new():memory allocation error\n");
    }
  else
    {
      req->type = type;
      req->data = data;
    }
  return req;
}

static StubData *stub_data_new(int fd, struct context_pointer *cp)
{ 
  StubData *sd = g_try_new0(StubData,1);
  if(!sd)
    {
      whiteboard_log_debug("stub_data_new():memory allocation error\n");
    }
  else
    {
      sd->fd = fd;
      sd->cp = cp;
    }
  return sd;
}
