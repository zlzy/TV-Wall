# name of your binary
bin_PROGRAMS = tvw_server tvw_client

# list of source files
# the prefix is the name of the binary
tvw_server_SOURCES = tvw_server.c server.c common/gst_utils.c common/utils.c 
tvw_client_SOURCES = tvw_client.c client.c common/gst_utils.c common/utils.c 

# list of headers we're not going to install
noinst_HEADERS = server.h client.h

# our CFLAGS and LDFLAGS used for compiling and linking
# make sure you prefix these with the name of your binary
tvw_server_CFLAGS = $(GST_CFLAGS)
tvw_server_LDFLAGS = $(GST_LIBS) -lcjson -lgstnet-1.0
tvw_client_CFLAGS = $(GST_CFLAGS)
tvw_client_LDFLAGS = $(GST_LIBS) -lcjson -lgstnet-1.0
