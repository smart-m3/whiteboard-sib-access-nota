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
 * @file sib_nota_access.h
 *
 * Copyright 2007 Nokia Corporation
 */

#ifndef SIB_ACCESS_H
#define SIB_ACCESS_H

typedef struct _SIBAccess SIBAccess;

#include <glib.h>

#include <sibmsg.h>
#include "sib_controller.h"

SIBAccess* sib_access_new(SIBController* cp, guchar *uri, gint sid, gint port);
gboolean sib_access_destroy(SIBAccess *sa);

/**
 * Increase the access's reference count
 *
 * @param access The access, whose reference count to increase
 */
void sib_access_ref(SIBAccess* access);

/**
 * Decrease the access's reference count. If the counter reaches zero,
 * the access is destroyed.
 *
 * @param access The access, whose reference count to increase
 */
void sib_access_unref(SIBAccess* access);
gint sib_access_receive(SIBAccess *sa);

gint sib_access_join(SIBAccess *sa,
		     ssElement_ct nodeName,
		     gint msgnumber,
		     ssElement_ct credentials,
		     NodeMsgContent_t **msgContent );

gint sib_access_leave(SIBAccess *sa, ssElement_ct nodeid,
		      gint msgnumber,NodeMsgContent_t **msgContent);

gint sib_access_insert(SIBAccess *sa,
		       guchar *nodeid,
		       gint msgnumber,
		       EncodingType encoding,
		       guchar *request,
		       NodeMsgContent_t **msgContent);

gint sib_access_update(SIBAccess *sa,
		       guchar *nodeid,
		       gint msgnumber,
		       EncodingType encoding,
		       guchar *irequest,
		       guchar *rrequest,
		       NodeMsgContent_t **msgContent);

gint sib_access_remove(SIBAccess *sa, 
		       guchar *nodeid,
		       gint msgnumber,
		       EncodingType encoding,
		       guchar *request, 
		       NodeMsgContent_t **msgContent);

gint sib_access_query(SIBAccess *sa,
		      guchar *nodeid,
		      gint msgnumber, 
		      gint type,
		      guchar *request, 
		      NodeMsgContent_t **msgContent);

gint sib_access_subscribe(SIBAccess *sa, 
			  guchar *nodeid,
			  gint msgnumber, 
			  gint type, 
			  guchar *request,
			  NodeMsgContent_t **msgContent);

gint sib_access_unsubscribe(SIBAccess *sa,
			    guchar *nodeid,
			    gint msgnumber,
			    guchar *request);

gint sib_access_wait_for_subscription_ind(SIBAccess *sa, guchar *nodeid, NodeMsgContent_t **msgContent);
gint sib_access_handle_receive(int sockfd, NodeMsgContent_t *msgContent);

#endif
 
