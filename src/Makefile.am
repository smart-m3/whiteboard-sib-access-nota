
libexec_PROGRAMS = whiteboard-sib-access-nota
libexecdir = $(prefix)/lib/whiteboard/libexec
ACLOCAL_AMFLAGS = -I m4

# Compiler flags
whiteboard_sib_access_nota_CFLAGS  = -Wall -I$(top_srcdir)/include
whiteboard_sib_access_nota_CFLAGS += @LIBWHITEBOARD_CFLAGS@
whiteboard_sib_access_nota_CFLAGS += @GNOME_CFLAGS@
whiteboard_sib_access_nota_CFLAGS += @NOTA_CFLAGS@

# Linker flags
whiteboard_sib_access_nota_LDFLAGS = @LIBWHITEBOARD_LIBS@

#whiteboard_sib_access_LDFLAGS += @GNOME_LIBS@ @MAEMO_LIBS@ @AVAHI_LIBS@
whiteboard_sib_access_nota_LDFLAGS += @GNOME_LIBS@
whiteboard_sib_access_nota_LDFLAGS += @NOTA_LIBS@

AM_CFLAGS = $(whiteboard_sib_access_nota_CFLAGS) -g

#STUBGEN = /usr/local/bin/nota-stubgen3.pl
STUBGEN = nota-stubgen3.pl
# Put these in alphabetical order so that they are easy to find.
whiteboard_sib_access_nota_SOURCES = \
	main.c \
	serverthread.c \
	sib_nota_access.c \
	sib_controller.c \
	sib_server.c \
	sib_service.c \
	SibAccessNota_common.h \
	SibAccessNota_user.h \
	SibAccessNota_user.c \
	nota_stub_handler.c

if RM
whiteboard_sib_access_nota_SOURCES += \
	NoTA_System_ResourceMng_R01_01_common.h \
	NoTA_System_ResourceMng_R01_01_user.c \
	NoTA_System_ResourceMng_R01_01_user.h
endif

BUILT_SOURCES = \
	SibAccessNota_common.h \
	SibAccessNota_user.h \
	SibAccessNota_user.c 
if RM
BUILT_SOURCES += NoTA_System_ResourceMng_R01_01_common.h \
	NoTA_System_ResourceMng_R01_01_user.c \
	NoTA_System_ResourceMng_R01_01_user.h 
endif

EXTRA_DIST = \
	sib.xml \
	NoTA_System_ResourceMng_R01_01_v03.xml

SibAccessNota_user.h: SibAccessNota_user.c
	@true

SibAccessNota_common.h: SibAccessNota_user.c
	@true

SibAccessNota_user.c: sib.xml
	$(STUBGEN) . sib.xml
if RM
NoTA_System_ResourceMng_R01_01_user.h: NoTA_System_ResourceMng_R01_01_user.c
	@true
NoTA_System_ResourceMng_R01_01_common.h: NoTA_System_ResourceMng_R01_01_user.c
	@true

NoTA_System_ResourceMng_R01_01_user.c: NoTA_System_ResourceMng_R01_01_v03.xml
	$(STUBGEN) . NoTA_System_ResourceMng_R01_01_v03.xml
endif
