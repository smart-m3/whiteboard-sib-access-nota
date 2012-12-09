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
 *
 * sib_controller.c
 *
 * Copyright 2007 Nokia Corporation
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>

#include <h_in/h_bsdapi.h>

#include <whiteboard_log.h>
#include "nota_stub_handler.h"
#if USE_RM==1
//#include <stubgen/resourcemng.h>
#define SID_RM 1
#include "NoTA_System_ResourceMng_R01_01_user.h"
#endif


#include "sib_service.h"
#include "sib_controller.h"
//#include "sib_maxfd.h"


#if USE_RM==1
#define M3_ONTOLOGY_NAME "M3_SIB"
#else
#define TEST_UDN "X"
#define TEST_NAME "TestServer"
#define TEST_SID 33
#endif
#define TEST_PORT 0


typedef struct _NotaRM
{
  int fd;
  struct context_pointer *context;
  h_in_t *core;
  //  GMutex *mutex;



  GMutex *cond_mutex;
  GCond *cond;
} NotaRM;


/*****************************************************************************
 * Structure definitions
 *****************************************************************************/
struct _SIBController
{

  SIBControllerListener listener;
  gpointer listener_data;
  GHashTable *services;

  NotaRM *rm;
  
  GMutex* mutex; /*locking*/

  GThread *receiver; // receiver thread for RM
  
  gboolean searcher_running;

  NotaStubHandler *stub_handler;
  
  GList *resolvelist;
  GList *entries;

  //  SIBMaxFd *maxfd;
};


//static int servercount = 0;
/** The timeout ID */
#if USE_RM==0
static guint timeout_id = 0;
static gboolean sib_controller_search_timeout( gpointer user_data);
#endif

static SIBController* sib_controller_new( );
static void sib_controller_lock(SIBController *self);
static void sib_controller_unlock(SIBController *self);


#if USE_RM==1
static gint sib_controller_create_rm_socket(SIBController *self);
gpointer sib_controller_rm_receiver(gpointer userdata);
gpointer sib_controller_search_thread(gpointer user_data);
#endif
static SIBServerData * sib_controller_get_sibentry(SIBController *self, guchar *desc );
gint sib_controller_start( SIBController *self);

gint sib_controller_stop( SIBController *self);

static SIBController *controller_singleton = NULL;

// utilities for handling hash table
#if 0
static gboolean sib_controller_add_service( SIBController *self,
					    SIBServerData *data);
static gboolean sib_controller_remove_service( SIBController *self,
					       gchar *uri);

static SIBServerData *sib_controller_get_service( SIBController *self,
						  gchar *uri);
#endif
static void sib_controller_free_sibserverdata(gpointer data);

/**
 * Create the SIBService singleton if it doesn't exist. Otherwise returns
 * the pointer to the service singleton.
 *
 * @return SIBService*
 */
static SIBController* sib_controller_new( )
{
  whiteboard_log_debug_fb();
  SIBController *self = NULL;
  self = g_new0(SIBController, 1);
  g_return_val_if_fail(self != NULL, NULL);

  self->mutex = g_mutex_new();
  self->listener = NULL;
  self->listener_data = NULL;

  self->services = g_hash_table_new_full(g_str_hash,
					 g_str_equal,
					 NULL, 
					 sib_controller_free_sibserverdata);



  //self->maxfd = sib_maxfd_new();
  self->stub_handler = nota_stub_handler_new();
  whiteboard_log_debug_fe();
  
  return self;
}

gint sib_controller_set_listener(SIBController *self, SIBControllerListener listener, gpointer user_data)
{
whiteboard_log_debug_fb();
  g_return_val_if_fail( self != NULL, -1);

  self->listener = listener;
  self->listener_data = user_data;
  whiteboard_log_debug_fe();
  return 0;
}

//SIBMaxFd *sib_controller_get_maxfd(SIBController *self)
//{
// return self->maxfd;
//}
/**
 * Get the SIBService singleton.
 *
 * @return SIBService*
 */
SIBController* sib_controller_get()
{
  whiteboard_log_debug_fb();
  if (controller_singleton == NULL)
    {
      controller_singleton = sib_controller_new();
    }
  whiteboard_log_debug_fe();
  return controller_singleton;
}

/**
 * Destroy a UPnP service
 *
 * @param service The service to destroy
 * @return -1 on errors, 0 on success
 */
gint sib_controller_destroy(SIBController* self)
{

  whiteboard_log_debug_fb();

  g_return_val_if_fail(self != NULL, -1);

  /* Ensure that the ctrl is stopped */
  sib_controller_stop(self);
  
  sib_controller_lock(self);
  g_hash_table_destroy(self->services);

  sib_controller_unlock(self);
  g_mutex_free(self->mutex);
  whiteboard_log_debug_fe();

  return 0;
}

/**
 * Lock a SIBController instance during a critical operation
 *
 * @param self The SIBController instance to lock
 */
static void sib_controller_lock(SIBController* self)
{
  g_return_if_fail(self != NULL);
  g_mutex_lock(self->mutex);
}

/**
 * Unlock a SIBController instance after a critical operation
 *
 * @param self The SIBController instance to unlock
 */ 
static void sib_controller_unlock(SIBController* self)
{
  g_return_if_fail(self != NULL);
  g_mutex_unlock(self->mutex);
}


/*****************************************************************************
 *
 *****************************************************************************/

/**
 * Start a controller
 * * @param self The controller to start
 * @return -1 on errors, 0 on success
 */
gint sib_controller_start(SIBController* self)
{
  //  struct timeval tv;
  //const char *version;
  gint retvalue = -1;
  whiteboard_log_debug_fb();
  g_return_val_if_fail(self != NULL, -1);

#if USE_RM==1
  retvalue = sib_controller_create_rm_socket(self);
  if(retvalue<0)
    {
      whiteboard_log_debug("Failed to create ResourceManager connection\n");
    }
  else
    {
      if( nota_stub_handler_add_connection(self->stub_handler, self->rm->fd, self->rm->context)<0)
	{
	  whiteboard_log_warning("Could not add RM connection to stub handler\n");
	  //nota_rm_handler_disconnect_rm_socket(self);
	}
      else
	{
	  whiteboard_log_debug("ResourceManager connection created.\n");
	}
      whiteboard_log_debug("ResourceManager connection created.\n");
    }
#else
  retvalue =0;
  whiteboard_log_debug("NoTA RM no in use\n");
#endif  
  whiteboard_log_debug_fe();

  return retvalue;
}

/**
 * Stop a controller
 *
 * @param controller The controller to stop
 * @return -1 on errors, 0 on success
 */
gint sib_controller_stop(SIBController* controller)
{
  whiteboard_log_debug_fb();

  g_return_val_if_fail(controller != NULL, -1);

  nota_stub_handler_destroy(controller->stub_handler);

  whiteboard_log_debug_fe();

  return 0;
}

#if USE_RM==0
gint sib_controller_search(SIBController *self)
{
  
  whiteboard_log_debug_fb();
  g_return_val_if_fail(self != NULL, -1);

  if( timeout_id == 0)
    {
      timeout_id=g_timeout_add( 5000,   // 5 seconds
                                sib_controller_search_timeout,
                                self );
    }
  
  
  whiteboard_log_debug_fe();
  return 0;
}

static gboolean sib_controller_search_timeout( gpointer user_data)
{
  SIBController *self = (SIBController *)(user_data);
  SIBService *service = (SIBService *)( self->listener_data);
  SIBServerData *ssdata = NULL;
  whiteboard_log_debug_fb();

  g_return_val_if_fail(self != NULL, FALSE);
  g_return_val_if_fail(service != NULL, FALSE);
  g_return_val_if_fail(servercount < 2, FALSE);
  
  ssdata = g_new0(SIBServerData,1);    
  /*   if(servercount == 0) */
/*     { */
/*       ssdata->ip = g_strdup(SIB_IP); */
/*       ssdata->port = SIB_PORT; */
/*       name = g_strdup(TEST_NAME); */
/*       udn = g_strdup(TEST_UDN); */
/*       servercount++; */
/*     } */
/*   else  */
/*     { */
/*       ssdata->ip = g_strdup(SIB_LOCAL_IP); */
/*       ssdata->port = SIB_PORT; */
/*       name = g_strdup(LOCAL_NAME); */
/*       udn = g_strdup(LOCAL_UDN); */
/*       servercount++;  */
/*     } */
  ssdata->sid = TEST_SID;
  ssdata->port = TEST_PORT;
       ssdata->name = g_strdup(TEST_NAME); 
       ssdata->uri = g_strdup(TEST_UDN); 

       self->listener( SIBDeviceStatusAdded, ssdata, self->listener_data);
       
       sib_controller_free_sibserverdata(ssdata);
       whiteboard_log_debug_fe();
         
  // do no call again
  return FALSE;
/*   if(servercount > 1) */
/*     return FALSE; */
/*   else */
/*     return TRUE; */
}
#else
gint sib_controller_search( SIBController *self)
{
  GError *err = NULL;
  GThread *searcher = NULL;
  whiteboard_log_debug_fb();
  g_return_val_if_fail(self != NULL, -1);
  
  if(self->searcher_running)
    {
      whiteboard_log_debug("Searcher already running...doing nothing.");
      whiteboard_log_debug_fe();
      return 0;
    }
  
  searcher = g_thread_create( sib_controller_search_thread,
			      self,
			      FALSE,
			      &err);
  if(searcher == NULL)
    {
      if(err != NULL)
	{
	  whiteboard_log_debug("Could not create search thread: %s\n", err->message);
	  g_error_free(err);
	}
      else
	{
	  whiteboard_log_debug("Could not create search thread\n");
	}
      whiteboard_log_debug_fe();
      return -1;
    }
  whiteboard_log_debug_fe();
  return 0;
}

gpointer sib_controller_search_thread(gpointer user_data)
{
  //GError *err = NULL;
  whiteboard_log_debug_fb();
  SIBController *self = (SIBController *)user_data;
  gchar *oname;
  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(self->rm != NULL, NULL);
  g_return_val_if_fail(self->rm->context != NULL, NULL);
#if 0  
  if( self->receiver == NULL )
    {

      self->receiver = g_thread_create( sib_controller_rm_receiver,
					self,
					FALSE,
					&err);
  
  
      if(self->receiver == NULL)
	{
	  if(err != NULL)
	    {
	      whiteboard_log_debug("Could not create receiver thread: %s\n", err->message);
	      g_error_free(err);
	    }
	  else
	    {
	      whiteboard_log_debug("Could not create receiver thread\n");
	    }
	  self->searcher_running = FALSE;
	  return NULL;
	}
    }
#endif
  self->searcher_running = TRUE;
  g_mutex_lock(self->rm->cond_mutex);
  //  g_mutex_lock(self->rm->mutex);
  oname = g_strndup( M3_ONTOLOGY_NAME,  strlen(M3_ONTOLOGY_NAME));
  if( NoTA_System_ResourceMng_R01_01_ListOfServices_req( self->rm->context,
  							 (uint8_t *) oname,
  							 (uint16_t) strlen(M3_ONTOLOGY_NAME)) < 0 )
   {
     whiteboard_log_debug("RM ListOfServices_req failed\n");
     //   g_mutex_unlock(self->rm->mutex);
     g_mutex_unlock(self->rm->cond_mutex);
   }
  else
    {
      whiteboard_log_debug("RM ListOfServices_req sent\n");
    
      //g_mutex_unlock(self->rm->mutex);
      // TODO: Add timeout
      whiteboard_log_debug("waiting for reply...\n");
      g_cond_wait(self->rm->cond, self->rm->cond_mutex);
      g_mutex_unlock( self->rm->cond_mutex);
      whiteboard_log_debug("got reply... number of found SIBs :%d\n", g_list_length(self->resolvelist));

      while( g_list_length( self->resolvelist) > 0)
	{
	  g_mutex_lock(self->rm->cond_mutex);
	  //g_mutex_lock(self->rm->mutex);
	  SIBServerData *ssdata = (SIBServerData *)self->resolvelist->data;
	  if( NoTA_System_ResourceMng_R01_01_ResolveService_req( self->rm->context,
								 (uint8_t *)oname,
								 (uint16_t) strlen(M3_ONTOLOGY_NAME),
								 (uint8_t *) ssdata->uri,
								 (uint16_t) strlen( (gchar *)ssdata->uri)) < 0 )
	    {
	      whiteboard_log_debug("RM ResolveService_req failed\n");
	      //  g_mutex_unlock(self->rm->mutex);
	      g_mutex_unlock(self->rm->cond_mutex);
	      break;
	    }
	  else
	    {
	      whiteboard_log_debug("RM ResolveService_req sent\n");
	      
	      //g_mutex_unlock(self->rm->mutex);
	      
	      whiteboard_log_debug("waiting for reply...\n");
	      // TODO: add timeout
	      g_cond_wait(self->rm->cond, self->rm->cond_mutex);
	      g_mutex_unlock( self->rm->cond_mutex);
	      whiteboard_log_debug("got reply... sid: %d\n", ssdata->sid);
	      GList *link = self->resolvelist;
	      self->resolvelist = g_list_remove_link(self->resolvelist, link);
	      self->entries = g_list_append(self->entries, link);
	      if(ssdata->sid>0)
		{
		  self->listener(SIBDeviceStatusAdded, ssdata, self->listener_data);
		}
	      else
		{
		  whiteboard_log_debug("Not valid SID: %d\n", ssdata->sid);
		}
	    }
	}

/*       g_mutex_lock(self->rm->cond_mutex); */
/*       g_mutex_lock(self->rm->mutex); */

/*       if( NoTA_System_ResourceMng_R01_01_NewEvent_req( self->rm->context, */
/* 						       EVENT_SERVICE_REGISTERED, */
/* 						       (uint8_t*) M3_ONTOLOGY_NAME, */
/* 						       (uint16_t) strlen(M3_ONTOLOGY_NAME), */
/* 						       NULL, */
/* 						       0) < 0 ) */
/* 	{ */
/* 	  whiteboard_log_debug("RM ResolveService_req failed\n"); */
/* 	  g_mutex_unlock(self->rm->mutex); */
/* 	  g_mutex_lock(self->rm->cond_mutex); */
/* 	} */
/*       else */
/* 	{ */
/* 	  whiteboard_log_debug("RM NewEvent_req sent\n"); */
	      
/* 	  g_mutex_unlock(self->rm->mutex); */
	      
/* 	  whiteboard_log_debug("waiting for reply...\n"); */
/* 	  g_cond_wait(self->rm->cond, self->rm->cond_mutex); */
/* 	  g_mutex_unlock( self->rm->cond_mutex); */
/* 	  //whiteboard_log_debug("got reply... sid: %d\n", ssdata->sid); */
	  
/* 	} */
    }
  g_free(oname);
  self->searcher_running = FALSE;
  whiteboard_log_debug_fe();
  return NULL;
}

#if 0
gpointer sib_controller_rm_receiver(gpointer userdata)
{
  gint err=0;
  SIBController *self = (SIBController *)userdata;
  NotaRM *rm = self->rm;

  struct timeval tv;
  fd_set read_s;
  fd_set error_s;

  tv.tv_sec = 0;
  tv.tv_usec = 0;

  while(err >= 0)
    {
      FD_ZERO(&read_s);
      FD_ZERO(&error_s);
      FD_SET(rm->fd, &read_s);
      FD_SET(rm->fd, &error_s);
      int maxfd;
      //g_mutex_lock(rm->mutex);
      maxfd = sib_maxfd_lock_and_get( self->maxfd);
      err = Hselect(rm->core, maxfd + 1, &read_s, NULL, &error_s, &tv);
      sib_maxfd_unlock(self->maxfd);
      if(err < 0)
	{
	  whiteboard_log_debug("Select (%d), error: %d\n", rm->fd, err);
	  whiteboard_log_debug("%s\n",strerror(err));
	}
      else
	{
	  if(FD_ISSET(rm->fd, &read_s))
	    {
	      err = nota_stub_handle_incoming(rm->context);
	      if(err < 0)
		{
		  whiteboard_log_debug("Failed incoming data (%d)\n", err);
		}
	    }
	  else if(FD_ISSET(rm->fd, &error_s))
	    {
	      whiteboard_log_debug("Error in socket\n");
	      err = -1;
	    }
	  // else timeout, continue...
	}
      //g_mutex_unlock(rm->mutex);
      usleep(10);
    }
  return NULL;
}
#endif
#endif
#if 0
static gboolean sib_controller_add_service( SIBController *self,
					    SIBServerData *data)
{
  gboolean ret = FALSE;
  whiteboard_log_debug_fb();
  // check that not existing previously
  if( sib_controller_get_service(self, data->uri) == NULL)
    {
      g_hash_table_insert(self->services, data->uri, data);
      ret = TRUE;
    }
  whiteboard_log_debug_fe();
  return ret;
}

static gboolean sib_controller_remove_service( SIBController *self,
					       gchar *uri)
{
  gboolean ret = FALSE;
  whiteboard_log_debug_fb();
  SIBServerData *ssdata=NULL;
  if( (ssdata = sib_controller_get_service(self, uri)) != NULL)
    {
      ret = g_hash_table_remove(self->services, uri);
    }
  whiteboard_log_debug_fe();
  return ret;
}

static SIBServerData *sib_controller_get_service( SIBController *self,
						  gchar *uri)
{
  SIBServerData *ssdata=NULL;
  
  whiteboard_log_debug_fb();
  // check that not existing previously
  
  ssdata = (SIBServerData *)g_hash_table_lookup(self->services,uri);
  
  whiteboard_log_debug_fe();
  return ssdata;
}
#endif

static void sib_controller_free_sibserverdata(gpointer data)
{
  SIBServerData *ssdata=(SIBServerData *)data;
  whiteboard_log_debug_fb();
  g_return_if_fail(data!=NULL);
  if(ssdata->uri)
    g_free(ssdata->uri);
  

  if(ssdata->name)
    g_free(ssdata->name);
  
  g_free(ssdata);
  whiteboard_log_debug_fe();
}
#if USE_RM==1
static gint sib_controller_create_rm_socket(SIBController *self)
 {
  nota_addr_t addr = { SID_RM, 0};
  NotaRM *rm = NULL;
  whiteboard_log_debug_fb();
  /* connect to service */
  g_return_val_if_fail(self->rm == NULL, -1);

  rm = g_new0(NotaRM, 1);
  g_return_val_if_fail(rm != NULL, -1);
  
  rm->core = Hgetinstance();
  if(!rm->core)
    {
      g_free(rm);
      whiteboard_log_debug_fe();
      return -1;
    }
  
  
  rm->fd = Hsocket(rm->core, AF_NOTA, SOCK_STREAM, 0);
  if(rm->fd<0)
    {
      g_free(rm);
      whiteboard_log_debug("Could not create RM socket\n");
      whiteboard_log_debug_fe();
      return -1;
    }

  //  sib_maxfd_set(self->maxfd, rm->fd);
  
  if( Hconnect(rm->core, rm->fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
      whiteboard_log_debug("Could not connect\n");
      Hclose(rm->core, rm->fd);
      g_free(rm);
      whiteboard_log_debug_fe();
      return -1;
    }
  whiteboard_log_debug("Socket connected\n");
  rm->context = nota_stub_context_pointer_initialize(rm->core, rm->fd);
  if(!rm->context)
    {
      whiteboard_log_debug("Could not create context pointer\n");
      Hclose(rm->core, rm->fd);
      g_free(rm);
      whiteboard_log_debug_fe();
      return -1;
    }
  whiteboard_log_debug("Context inialized\n");
  if( NoTA_System_ResourceMng_R01_01_user_new_connection(rm->context) < 0)
    {
      whiteboard_log_debug("could not add to stub\n");
      nota_stub_context_pointer_free(rm->context);
      Hclose(rm->core, rm->fd);
      g_free(rm);
      whiteboard_log_debug_fe();
      return -1;
    }
  rm->context->user_pointer = self;
  // rm->mutex = g_mutex_new();

  rm->cond_mutex = g_mutex_new();
  rm->cond = g_cond_new();
  
  self->rm = rm;
  
  whiteboard_log_debug("Stub connected\n");
  whiteboard_log_debug_fe();
  return 0;
}
#endif
SIBServerData* sib_controller_get_sibentry( SIBController *self, guchar *desc )
{
  GList *link = self->entries;
  SIBServerData *ssdata= NULL;
  while( link != NULL)
    {
      ssdata = (SIBServerData *)link->data;
      if( g_ascii_strcasecmp( (gchar *)ssdata->uri, (gchar *)desc) == 0)
	{
	  break;
	}
      link = g_list_next(link);
    }
  return ssdata;
}

NotaStubHandler *sib_controller_get_stubhandler(SIBController *self)
{
  return self->stub_handler;
}

#if USE_RM==1
void NoTA_System_ResourceMng_R01_01_Register_cnf_process( struct context_pointer* context, status_t status, sid_t sid, uint8_t* cert_dereg, uint16_t cert_dereg_len, uint8_t* cert_act, uint16_t cert_act_len )
{
  whiteboard_log_debug_fb();
  whiteboard_log_debug_fe();
}

void NoTA_System_ResourceMng_R01_01_Deregister_cnf_process( struct context_pointer* context, status_t status )
{
  whiteboard_log_debug_fb();
  whiteboard_log_debug_fe();
}

void NoTA_System_ResourceMng_R01_01_Challenge_ind_process( struct context_pointer* context, uint8_t* challenge, uint16_t challenge_len )
{
  whiteboard_log_debug_fb();
  whiteboard_log_debug_fe();
}

void NoTA_System_ResourceMng_R01_01_ResolveService_cnf_process( struct context_pointer* context, status_t status, sid_t sid )
{
  SIBController *self = (SIBController *)context->user_pointer;
  whiteboard_log_debug_fb();
  g_mutex_lock(self->rm->cond_mutex);
  SIBServerData *ssdata = (SIBServerData *)self->resolvelist->data;

  if(status == S_OK)
    {
      ssdata->sid = sid;
    }
  g_cond_signal(self->rm->cond);
  g_mutex_unlock(self->rm->cond_mutex);
  whiteboard_log_debug_fe();

}

void NoTA_System_ResourceMng_R01_01_ListOfServices_cnf_process( struct context_pointer* context, servicelist_t* servicelist )
{
  int ii;
  SIBController *self = (SIBController *)context->user_pointer;
  whiteboard_log_debug_fb();
  whiteboard_log_debug("Services: %d\n", servicelist->service_desc_count);
  for(ii=0; ii < servicelist->service_desc_count; ii++)
    {
      guchar *desc = (guchar *)g_strndup( (gchar *)servicelist->service_desc[ii],
			       (gint)servicelist->service_desc_len[ii]);
      if( sib_controller_get_sibentry(self, desc ) == NULL )
	{
	  SIBServerData *entry = g_new0(SIBServerData,1);
	  entry->uri = (guchar *)desc;
	  entry->name = (guchar *)g_strdup("dummy");
	  self->resolvelist = g_list_append(self->resolvelist, entry);
	  whiteboard_log_debug("SIB[%d]: %s added\n", ii, desc);
	}
      else
	{
	  whiteboard_log_debug("SIB[%d]: %s\n already exists", ii, desc);
	  g_free(desc);
	}
    }
  g_mutex_lock(self->rm->cond_mutex);
  g_cond_signal(self->rm->cond);
  g_mutex_unlock(self->rm->cond_mutex);
  whiteboard_log_debug_fe();
}

void NoTA_System_ResourceMng_R01_01_NewEvent_cnf_process( struct context_pointer* context, int32_t event_id, status_t status )
{
  SIBController *self = (SIBController *)context->user_pointer;
  whiteboard_log_debug_fb();

  if(status == S_OK)
    {
      
    }
  
  g_mutex_lock(self->rm->cond_mutex);
  g_cond_signal(self->rm->cond);
  g_mutex_unlock(self->rm->cond_mutex);

  whiteboard_log_debug_fe();
}

void NoTA_System_ResourceMng_R01_01_NewEvent_ind_process( struct context_pointer* context, uint32_t event_id, event_t event_t )
{
  whiteboard_log_debug_fb();
  whiteboard_log_debug_fe();
}

void NoTA_System_ResourceMng_R01_01_DeleteEvent_cnf_process( struct context_pointer* context, status_t status )
{
  whiteboard_log_debug_fb();
  whiteboard_log_debug_fe();
}


/* these hooks must be implemented by user. */
/* This function is called when connection was closed */
void NoTA_System_ResourceMng_R01_01_user_handler_disconnected(struct context_pointer* context)
{
  whiteboard_log_debug_fb();
  whiteboard_log_debug_fe();
}


/* This function is called when there was out-of-memory or other not so critical error (connection is still usable) */
void NoTA_System_ResourceMng_R01_01_user_handler_error(struct context_pointer* context, int error_type)
{
  whiteboard_log_debug_fb();
  whiteboard_log_debug("Error_type: %d\n", error_type);
  whiteboard_log_debug_fe();
}
#endif
