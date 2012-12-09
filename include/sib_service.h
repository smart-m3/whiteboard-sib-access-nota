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
 * WhiteBoard SIB component
 *
 * sib_service.h
 *
 * Copyright 2007 Nokia Corporation
 */

#ifndef SIB_SERVICE_H
#define SIB_SERVICE_H

typedef struct _SIBService SIBService;

#include <glib.h>
//#include <libosso.h>
#include <whiteboard_node.h>

#include "sib_server.h"
#include "sib_controller.h"



/**
 * Create the UPnPService singleton if it doesn't exist. Otherwise returns
 * the pointer to the service singleton.
 *
 * @return UPnPService*
 */
//UPnPService* upnpservice_new();

//SIBService *sib_service_new();

/**
 * Get the UPnPService singleton.
 *
 * @return UPnPService*
 */
//UPnPService* upnpservice_get();
SIBService *sib_service_get();

/**
 * Destroy a UPnP service
 *
 * @param service The service to destroy
 * @return -1 on errors, 0 on success
 */
//gint upnpservice_destroy(UPnPService* service);
gint sib_service_destroy(SIBService *service);

/**
 * Lock a UPnPService instance during a critical operation
 *
 * @param self The UPnPService instance to lock
 */
//void upnpservice_lock(UPnPService* self);
void sib_service_lock(SIBService *self);

/**
 * Unlock a UPnPService instance after a critical operation
 *
 * @param self The UPnPService instance to unlock
 */
//void upnpservice_unlock(UPnPService* self);
void sib_service_unlock(SIBService *self);
/**
 * Get the UPnP control point instance from a UPnPService instance
 *
 * @param self A UPnPService instance
 * @return The control point instance
 */
SIBController* sib_service_get_controller(SIBService* self);

/**
 * Start a UPnP service. This function will block until the service is stopped.
 *
 * @param service The service to start
 * @return -1 on errors, 0 on success
 */
//gint upnpservice_start(UPnPService* service);
gint sib_service_start(SIBService *service);

/**
 * Stop a UPnP service.
 *
 * @param service The service to stop
 * @return -1 on errors, 0 on success
 */
//gint upnpservice_stop(UPnPService* service);
gint sib_service_stop(SIBService *service);

/**
 * Start a UPnP control point.
 *
 * @param service The service, whose controlpoint to start
 * @return -1 on errors, 0 on success
 */
//gint upnpservice_controlpoint_start(UPnPService* service);

/**
 * Stop a UPnP service's control point
 *
 * @param service The service, whose controlpoint to start
 * @return -1 on errors, 0 on success
 */
//gint upnpservice_controlpoint_stop(UPnPService* service);

/**
 * Add a UPnP server to the service by the server's UDN.
 *
 * @param service The service to add to
 * @param udn The UDN of the device to add
 * @return -1 on errors, 0 on success
 */
//gint upnpservice_add_server_by_udn(UPnPService* service, gchar* udn);
gint sib_service_add_server(SIBService *service, guchar *udn, guchar *name, gint sid, gint port );

/**
 * Remove a server from the service by the server's UDN.
 *
 * @param service The service to remove from
 * @param udn The UDN of the server to remove
 * @return -1 on errors, 0 on success
 */
//gint upnpservice_remove_server_by_udn(UPnPService* service, gchar* udn);
gint sib_service_remove_server(SIBService *service, guchar *udn  );

/**
 * Find a server from the service by the server's UDN.
 *
 * @param service The service to search from
 * @param udn The UDN of the server to look for
 * @return A UPnPServer* or NULL
 */
SIBServer* sib_service_find_server(SIBService* service, const guchar* udn);

/**
 * Get the size of the service's serverlist
 *
 * @param service The service whose size to get
 * @return The number of servers in the serverlist
 */
gint sib_service_serverlist_size(SIBService* service);

/**
 * Send the root containers of all available servers
 *
 * @param service The UPnPService instance
 * @param sourceid MediaHubSourceHandle instance
 */
//void upnpservice_send_all_servers(UPnPService* service,
//				  MediaHubSourceHandle* sourceid);



#endif
 
