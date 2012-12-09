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
 * @file serverthread.h
 *
 * Copyright 2007 Nokia Corporation
 */

#ifndef SERVERTHREAD_H
#define SERVERTHREAD_H

#include <whiteboard_sib_access.h> 

#include "sib_server.h"
#include "sib_service.h"

/**
 * Initialize the server thread
 *
 * @param service The SIBService, that this thread pool is assigned to
 * @return -1 for errors, 0 on success
 */
gint serverthread_init(SIBService* service);
gint serverthread_receive( SIBServer *sa );
gint serverthread_join(SIBServer* server,
		       WhiteBoardSIBAccessHandle* handle,
		       gint access_id,
		       guchar *nodeid,
		       guchar *udn,
		       gint msgnumber);

gint serverthread_leave(SIBServer* server,
			WhiteBoardSIBAccessHandle* handle,
			guchar *nodeid,
			guchar *udn,
			gint msgnumber);

gint serverthread_insert(SIBServer* server,
			 WhiteBoardSIBAccessHandle* handle,
			 guchar *nodeid,
			 guchar *sibid,
			 gint msgnumber,
			 EncodingType encoding,
			 guchar *request);

gint serverthread_update(SIBServer* server,
			 WhiteBoardSIBAccessHandle* handle,
			 guchar *nodeid,
			 guchar *sibid,
			 gint msgnumber,
			 EncodingType encoding,
			 guchar *irequest,
			 guchar *rrequest);

gint serverthread_query(SIBServer* server,
			 WhiteBoardSIBAccessHandle* handle,
			gint access_id,
			 guchar *nodeid,
			 guchar *sibid,
			gint type,
			gint msgnumber,
			 guchar *request);

gint serverthread_remove(SIBServer* server,
			 WhiteBoardSIBAccessHandle* handle,
			 guchar *nodeid,
			 guchar *sibid,
			 gint msgnumber,
			 EncodingType encoding,
			 guchar *request);

gint serverthread_subscribe(SIBServer* server,
			    WhiteBoardSIBAccessHandle* handle,
			    gint access_id,
			    guchar *nodeid,
			    guchar *sibid,
			    gint msgnumber,
			    gint type,
			    guchar *request);

gint serverthread_unsubscribe(SIBServer* server,
			      WhiteBoardSIBAccessHandle* handle,
			      gint access_id,
			      guchar *nodeid,
			      guchar *sibid,
			      gint msgnumber,
			      guchar *request);

gint serverthread_create_listener();


#endif
 
