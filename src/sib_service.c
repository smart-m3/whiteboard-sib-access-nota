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
 * sib_service.c
 *
 * Copyright 2007 Nokia Corporation
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <whiteboard_sib_access.h>
#include <whiteboard_log.h>


#include "sib_service.h"

#include "sib_nota_access.h"

#include "serverthread.h"

/*****************************************************************************
 * Structure definitions
 *****************************************************************************/
#define SIB_CONTROLLER_SEARCH_TIMEOUT 15000
struct _SIBService
{
  SIBController *controller;
  gboolean ctrl_running;
  
  GMainLoop* main_loop;
  //osso_context_t* osso_context;
  
  GList* siblist;
  GMutex* mutex;
  
  WhiteBoardSIBAccess  *control_channel;
};

static SIBService* service_singleton = NULL;

static SIBController *sib_service_create_controller();
static gint sib_service_controller_start();
static gboolean sib_service_controller_search_timeout( gpointer user_data);

/*****************************************************************************
 * Control channel signal callbacks
 *****************************************************************************/
static void sib_service_refresh_cb(WhiteBoardNode* context,
				   gpointer user_data);

static void sib_service_shutdown_cb(WhiteBoardNode* context,
				    gpointer user_data);

static void sib_service_device_listener( SIBDeviceStatus status, SIBServerData *ssdata, gpointer user_data);

/*****************************************************************************
 * Creation/destruction
 *****************************************************************************/


/**
 * Create the SIBService singleton if it doesn't exist. Otherwise returns
 * the pointer to the service singleton.
 *
 * @return SIBService*
 */
static SIBService* sib_service_new()
{
  whiteboard_log_debug_fb();

  if (service_singleton == NULL)
    {
      service_singleton = g_new0(SIBService, 1);
      g_return_val_if_fail(service_singleton != NULL, NULL);

      service_singleton->controller = sib_service_create_controller(service_singleton);
      service_singleton->ctrl_running = FALSE;

      service_singleton->main_loop = g_main_loop_new(NULL, FALSE);
      g_main_loop_ref(service_singleton->main_loop);

      service_singleton->mutex = g_mutex_new();
    }

  whiteboard_log_debug_fe();

  return service_singleton;
}

/**
 * Get the SIBService singleton.
 *
 * @return SIBService*
 */
SIBService* sib_service_get()
{
  whiteboard_log_debug_fb();
  if (service_singleton == NULL)
    {
      service_singleton = sib_service_new();
      serverthread_init(service_singleton);
    }
  whiteboard_log_debug_fe();
  return service_singleton;
}

/**
 * Destroy a UPnP service
 *
 * @param service The service to destroy
 * @return -1 on errors, 0 on success
 */
gint sib_service_destroy(SIBService* service)
{
  GList* item = NULL;

  whiteboard_log_debug_fb();

  g_return_val_if_fail(service != NULL, -1);

  /* Ensure that the service is stopped */
  sib_service_stop(service);

  //  sib_service_lock(service);

  /* Ensure that there are no servers left in the cache */
  while (service->siblist != NULL)
    {
      item = service->siblist;
      service->siblist = g_list_remove_link(service->siblist,
					    item);

      sib_server_unref((SIBServer*)item->data);

      g_list_free(item);
    }

  //osso_deinitialize(service->osso_context);
  //service->osso_context = NULL;

  g_object_unref(G_OBJECT(service->control_channel));
  //  sib_service_unlock(service);
  g_main_loop_unref(service->main_loop);
  service->main_loop = NULL;

  //  sib_service_unlock(service);

  whiteboard_log_debug_fe();

  return 0;
}

/**
 * Lock a SIBService instance during a critical operation
 *
 * @param self The SIBService instance to lock
 */
void sib_service_lock(SIBService* self)
{
  whiteboard_log_debug_fb();
  g_return_if_fail(self != NULL);
  g_mutex_lock(self->mutex);
  whiteboard_log_debug_fe();
}

/**
 * Unlock a SIBService instance after a critical operation
 *
 * @param self The SIBService instance to unlock
 */
void sib_service_unlock(SIBService* self)
{
  whiteboard_log_debug_fb();
  g_return_if_fail(self != NULL);
  g_mutex_unlock(self->mutex);
  whiteboard_log_debug_fe();
}


/*****************************************************************************
 *
 *****************************************************************************/

/**
 * Start a UPnP service. This function will block until the service is stopped.
 *
 * @param service The service to start
 * @return -1 on errors, 0 on success
 */
gint sib_service_start(SIBService* service)
{
  gint retval = 0;
  whiteboard_log_debug_fb();

  g_return_val_if_fail(service != NULL, -1);
  g_return_val_if_fail(service->main_loop != NULL, -1);

  if (g_main_loop_is_running(service->main_loop) == TRUE)
    {
      whiteboard_log_debug("sib_service_start(): main loop running\n");
      whiteboard_log_debug_fe();
      return -1;
    }
  else
    {
    
  
      /* TODO: control channel uuid define */
      service->control_channel = 
	WHITEBOARD_SIB_ACCESS(whiteboard_sib_access_new(NULL,
							(guchar *)"unique_whiteboard_sib_nota_cc_id",
						       NULL,
						       (guchar *)"SIB Access",
						       (guchar *)"Not yet done"));
      
      g_signal_connect(G_OBJECT(service->control_channel),
		       WHITEBOARD_SIB_ACCESS_SIGNAL_REFRESH,
		       (GCallback) sib_service_refresh_cb,
		       NULL);
      
      g_signal_connect(G_OBJECT(service->control_channel),
		       WHITEBOARD_SIB_ACCESS_SIGNAL_SHUTDOWN,
		       (GCallback) sib_service_shutdown_cb,
		       NULL);
      
      retval = sib_service_controller_start( g_main_loop_get_context(service->main_loop) );
      if(retval < 0)
	{
	  whiteboard_log_error("SIB Controller start failed\n");
	  retval = -1;
	  //	  exit(1);
	}
      
      whiteboard_log_debug("RUnning main loop\n");
      g_main_loop_run(service->main_loop);
    }
  
  whiteboard_log_debug_fe();
  
  return retval;
}

static gint sib_service_controller_start( GMainContext *context)
{
  gint retval = -1;
  whiteboard_log_debug_fb();
  SIBService *service = sib_service_get(context);
  
  sib_service_lock(service);
  if (service->ctrl_running == FALSE)
    {
      if (sib_controller_start(service->controller) == 0)
	{
	  service->ctrl_running = TRUE;
	  

	  if(sib_controller_search(service->controller )  < 0)
	    {
	      whiteboard_log_debug("Could not search for services\n");
	    }
	  else
	    {
	      g_timeout_add( SIB_CONTROLLER_SEARCH_TIMEOUT,  
			     sib_service_controller_search_timeout,
			     service );
	      retval = 0;
	      
	    }
	}
      else
	{
	  whiteboard_log_debug("Error starting SIB controller\n");  
	  service->ctrl_running = FALSE;
	  retval = -1;
	}
    }
  else
    {
      whiteboard_log_debug("SIB Controller already started\n");  
    }
  sib_service_unlock(service);
  whiteboard_log_debug_fe();
  return retval;
}

/**
 * Stop a UPnP service.
 *
 * @param service The service to stop
 * @return -1 on errors, 0 on success
 */
gint sib_service_stop(SIBService* service)
{
  whiteboard_log_debug_fb();

  g_return_val_if_fail(service != NULL, -1);
  g_return_val_if_fail(service->main_loop != NULL, -1);

  if (g_main_loop_is_running(service->main_loop) == TRUE)
    {
      /* Ensure that there are no servers left in the cache */
      whiteboard_log_debug("Cleaning SIBServer list\n");
      //sib_service_lock(service);
      while (service->siblist != NULL )
	{
	  GList *item = service->siblist;
	  service->siblist = g_list_remove_link(service->siblist,
						item);
	  
	  sib_server_unref((SIBServer*)item->data);
	  
	  g_list_free(item);
	}
      // sib_service_unlock(service);
      

      sib_controller_stop(service->controller);
      service->ctrl_running = FALSE;
      
      

      g_main_loop_quit(service->main_loop);
    }

  whiteboard_log_debug_fe();

  return 0;
}

static gboolean sib_service_controller_search_timeout( gpointer user_data)
{
  gboolean retval = FALSE;
  SIBService *self = (SIBService *)user_data;

  if(self->ctrl_running && (sib_controller_search(self->controller )  == 0 ) )
    {
      retval = TRUE;
    }
  else
    {
      whiteboard_log_debug("Controller not running or sib_controller_search failed\n");
    }
  return retval;    
}

SIBController* sib_service_create_controller(SIBService *self)
{
  SIBController* controller = NULL;
  whiteboard_log_debug_fb();
  
  //  controller = g_new0(SIBController, 1);
  controller = sib_controller_get();
  
  g_return_val_if_fail(controller != NULL, NULL);

  sib_controller_set_listener(controller, sib_service_device_listener, self);
  whiteboard_log_debug_fe();
    
  return controller;
}
					    

/**
 * Add a UPnP server to the service by the server's UDN.
 *
 * @param service The service to add to
 * @param udn The UDN of the device to add
 * @return -1 on errors, 0 on success
 */
// TODO: change UDN to SIB_identifier
gint sib_service_add_server(SIBService* service , guchar *udn, guchar *name, gint sid, gint port )
{
  SIBServer *server = NULL;
  
  whiteboard_log_debug_fb();
  
  sib_service_lock(service);

  server = sib_service_find_server(service, udn);
  if (server == NULL)
    {
      server = sib_server_new(service, udn, name, sid, port);
      if (server != NULL)
	{
	  //upnpserver_subscribe(server);
	  
	  service->siblist =
	    g_list_append(service->siblist,
			  server);
	}
    }
  
  sib_service_unlock(service);
  
  whiteboard_log_debug_fe();
  return 0;
}

/**
 * Remove a server from the service by the server's UDN.
 *
 * @param service The service to remove from
 * @param udn The UDN of the server to remove
 * @return -1 on errors, 0 on success
 */
gint sib_service_remove_server(SIBService* service, guchar *udn )
{
  GList* item = NULL;
  gint retval = -1;

  whiteboard_log_debug_fb();

  g_return_val_if_fail(service != NULL, -1);

  sib_service_lock(service);
  item = g_list_find_custom(service->siblist, udn,
			    sib_server_compare_udn);

  if (item != NULL)
    {
      sib_server_unref((SIBServer*) item->data);
      
      service->siblist =
	g_list_remove_link(service->siblist, item);
      
      retval = 0;
    }
  else
    {
      whiteboard_log_debug("Device %s is not in our server list. Ignored.\n",
			   udn);
    }
  
  sib_service_unlock(service);
  
  whiteboard_log_debug_fe();
  
  return retval;
}

/**
 * Find a server from the service by the server's UDN.
 *
 * @param service The service to search from
 * @param udn The UDN of the server to look for
 * @return A SIBServer* or NULL
 */
SIBServer* sib_service_find_server(SIBService* service, const guchar *udn)
{
  SIBServer* server = NULL;
  GList* item = NULL;
  whiteboard_log_debug_fb();
  g_return_val_if_fail(service != NULL, NULL);
  
  item = g_list_find_custom(service->siblist, udn, 
 			    sib_server_compare_udn); 
  
  if (item != NULL) 
    { 
      server = (SIBServer*) item->data; 
    } 
  else 
    { 
      server = NULL; 
    } 
  whiteboard_log_debug_fe();
  return server;
}

/**
 * Get the UPnP control point instance from a UPnPService instance
 *
 * @param self A UPnPService instance
 * @return The control point instance
 */
SIBController* sib_service_get_controller(SIBService* self)
{
  g_return_val_if_fail(self != NULL, NULL);
  return self->controller;
}


/*****************************************************************************
 * Control channel signal callbacks
 *****************************************************************************/

static void sib_service_refresh_cb(WhiteBoardNode* context,
				   gpointer user_data)
{
  SIBService* service = NULL;

  whiteboard_log_debug_fb();

  service = sib_service_get();
  //  g_return_if_fail(service->cp_running == TRUE);
  //cg_upnp_controlpoint_search(service->cp, "upnp:rootdevice");

  whiteboard_log_debug_fe();
}

static void sib_service_shutdown_cb(WhiteBoardNode* context,
				    gpointer user_data)
{
  SIBService* service = NULL;

  whiteboard_log_debug_fb();

  service = sib_service_get();
  sib_service_stop(service);

  whiteboard_log_debug_fe();
}

static void sib_service_device_listener(SIBDeviceStatus status, SIBServerData *ssdata, gpointer user_data)
{
  SIBService *service = NULL;
  whiteboard_log_debug_fb();

  service = (SIBService *)user_data;
  g_return_if_fail(service != NULL);
  
  switch(status)
    {
    case SIBDeviceStatusAdded:
      whiteboard_log_debug("SIBDeviceStatusAdded: %s<%s>\n", ssdata->name,ssdata->uri );
      g_return_if_fail(ssdata!= NULL);
      sib_service_add_server(service,ssdata->uri,ssdata->name, ssdata->sid, ssdata->port);
      break;
    case SIBDeviceStatusRemoved:
      whiteboard_log_debug("SIBDeviceStatusRemoved: %s<%s>\n",ssdata->name,ssdata->uri );
      sib_service_remove_server(service,ssdata->uri);
      break;
    case SIBDeviceStatusInvalid:
      whiteboard_log_debug("SIBDeviceStatusInvalid: %s<%s>\n",ssdata->name,ssdata->uri );
      sib_service_remove_server(service,ssdata->uri);
      break;
    default:
      whiteboard_log_error("Unknown SIBDeviceStatus for server: %s\n",ssdata->uri );
      break;
    }
   
  whiteboard_log_debug_fe();
}

