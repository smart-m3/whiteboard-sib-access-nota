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
/**
 * Whiteboard SIB access
 *
 * @file main.c
 *
 * Copyright 2007 Nokia Corporation
 *
 * @mainpage Whiteboard SIB Access component documentation
 *
 * @section intro_sec Introduction
 *
 * The SIB access component provides access to remote SIBs via TCP/IP connection and discovery of new SIBs. It connects to the Whiteboard deamon via dedicated DBUS interface. For each discovered SIB a SIBServer structure is created. The SIBServer structure contains a pointer to WhiteBoardSIBAccess GObject which provides methods and GSignals to send and receive DBus messages to and from the Whiteboard daemon.
 *
 * For each transaction a new thread is created. 
 *
 * @section install_sec Building and installing SIB access
 *
 * @subsection install_sbox_sec Scratchbox environment
 *
 * > ./autogen.sh
 *
 * > ./configure --with-debug --prefix=/usr/local --libexecdir=/usr/local/lib/whiteboard/libexec [--enable-avahi=yes]
 *
 * > make
 *
 * > make install
 *
 * The configure script checks whether Whiteboard library (libwhiteboard) is installed. If the check fails and you think that you have installed the Whiteboard library, make sure that your PKG_CONFIG_PATH environment variable contains path where the pkg-config file of the libwhiteboard is installed. Usually this is PREFIX/lib/pkgconfig, where PREFIX is the value of the parameter --prefix used when building Whiteboard library. Normally PREFIX used is /usr or /usr/local. The parameter --with-avahi defines wheter Avahi mDNS service discovery is used for searching Smart spaces.
 *
 * @subsection install_n800_sec N800 device
 *
 * Building and installing Whiteboard SIB access component to N800 device is straightforward. First, one must create the installation .deb package, transfer it to the memory card of the N800 device and, finally, install the package with N800's Application Manager.
 *
 * Before you start building the installation package, make sure that you have selected correct target and Whiteboard library and Whiteboard daemon are built and installed to the target. The compile target is selected with following command:
 *
 * > sb-conf select SDK_ARMEL
 *
 * Then change your working directory to the root directory of the Whiteboard SIB access code tree and type command:
 *
 * > dpkg-buildpackage -rfakeroot
 *
 * which will create the .deb installation file.  The dpkg-buildpackage reads parameters given for the configure script from the debian/rules files. So you might want to check the contents of the rules file prior running the dpkg-buildpackage. Especially, the --libexecdir parameter should match the one used when building Whiteboard daemon.
 *
 * The installation file can be transferred to the memory card of the N800 for example via USB cable. When connecting the USB cable to your PC the memory cards of the N800 are seen as removable disks. The installation is done be browsing the location where you copied the .deb file with the File Manager and double clicking the .deb file which will launch the Application manager. Prior installation of the Whiteboard SIB access, the Whiteboard library and the Whiteboard daemon must be properly installed.
 *
 * @section ss_discover_sec Discovering smartspaces
 *
 * Currently there are two ways to discover existing Smartspaces, either hardcoded (i.e. no discovery) or via multicast DNS technology a.k.a. Avahi, Bonjour or Zeroconf.
 *
 * The Avahi service discovery is enabled with --enable-avahi parameter to the configure script. The Avahi service discovery works in Ubuntu and on N8x0 with Maemo OS2008 environments. Currently Avahi can not be run on scratchbox or on N8x0 with Maemo OS2007 environments.

 * When using fixed SIB addresses, the sib_controller_search() schedules a timer that creates one SIBServer instance with a priori known discovery information. The SIB URI, frienly name, IP address and TCP port number are defined at the top of the source file sib_controller.c. In order to match your development environment modify the source file accordingly.
 *
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <glib.h>
#include <whiteboard_log.h>

#include "sib_service.h"

static SIBService* service = NULL;

void main_signal_handler(int sig)
{
  static volatile sig_atomic_t signalled = 0;
  
  whiteboard_log_debug_fb();
  
  if ( 1 == signalled )
    {
      signal(sig, SIG_DFL);
      raise(sig);
    }
  else
    {
      sib_service_stop(service);
      signalled = 1;
    }

  whiteboard_log_debug_fe();
}

int main(int argc, char** argv)
{
  g_type_init();
  g_thread_init(NULL);
  dbus_g_thread_init();

  whiteboard_log_debug("Starting SIB access component\n");

  /* Set signal handlers */
  signal(SIGINT, main_signal_handler);
  signal(SIGTERM, main_signal_handler);

  service = sib_service_get();
  g_return_val_if_fail(service != NULL, -1);
  
  /* This will block until g_main_loop_quit() is called */
  if (sib_service_start(service) < 0)
    {
      /* Unable to start the service */
      sib_service_stop(service);
    }
  
  sib_service_destroy(service);
  service = NULL;
  
  return 0;
}
