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
 * @file sib_controller.h
 *
 * Copyright 2007 Nokia Corporation
 */

#ifndef SIB_CONTROLLER_H
#define SIB_CONTROLLER_H

typedef struct _SIBController SIBController;

#include <glib.h>
#if AVAHI==1
#include <avahi-gobject/ga-service-resolver.h>
#endif
#include "sib_service.h"
#include "nota_stub_handler.h"
//#include "sib_maxfd.h"

typedef struct _SIBServerData
{
  guchar *uri;
  gint sid;
  guchar *name;
  gint port;
  
} SIBServerData;

typedef enum {
  SIBDeviceStatusAdded,
  SIBDeviceStatusRemoved,
  SIBDeviceStatusInvalid,
  
} SIBDeviceStatus;

typedef void (* SIBControllerListener )(SIBDeviceStatus status, SIBServerData *ssdata, gpointer user_data);

SIBController* sib_controller_get();

gint sib_controller_start( SIBController *self);

gint sib_controller_stop( SIBController *self);

gint sib_controller_search( SIBController *self);

gint sib_controller_set_listener(SIBController *self, SIBControllerListener listener, gpointer user_data);


//SIBMaxFd *sib_controller_get_maxfd(SIBController *self);
NotaStubHandler *sib_controller_get_stubhandler(SIBController *self);
#endif
 
