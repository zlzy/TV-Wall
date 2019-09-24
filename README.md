f## Details
This project is for building TV video wall application. This has two components
1. Server : With an encoder functionality. It takes an UVC Camera as input, and pushes out multicast output stream
2. Client : Client which receives multicast stream input and decodes and renders a part of the video based on the configuration
Usually, there would be 1 server and 4 clients for a 2x2 TV wall. Server could be running on one of the client device as long as CPU load permits

## Setting up
1. Install Rasperrry Pi 3B+ with Raspbain stretch
2. Install the following dependencies
   #### Tools for building gstreamer
   ```
   apt-get install autoconf autopoint libtool
   ```

   #### Gstreamer critical dependencies
   ```
   apt install gstreamer-tools-1.0
   apt-get install bison flex gettext libffi6 libffi-dev glib-2.0 libglib2.0-dev
   apt install libgstreamer-plugins-base1.0-dev
   ```

   #### Install gst-mmal
   ```
   git clone https://gitlab.com/gstmmal/gst-mmal.git
   cd gst-mmal
   LDFLAGS='-L/opt/vc/lib' CPPFLAGS='-I/opt/vc/include -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux' ./autogen.sh --disable-gtk-doc
    make
    sudo make install
   ```

   #### Install cJSON
   ```
   git clone https://github.com/DaveGamble/cJSON.git
   cd cJSON/
   make install
   ```

   #### Install header files from gstreamer playback(Needed to choose between hardware and software decoder)
   ```
   wget https://cgit.freedesktop.org/gstreamer/gst-plugins-base/plain/gst/playback/gstplay-enum.h?h=1.14
   mv gstplay-enum.h?h=1.14 /usr/include/gstreamer-1.0/gst/gstplay-enum.h
   ```



## Building the code
   1. Clone this project
      git clone https://github.com/guojin62/TVwall.git
   2. Run the following commands
      ```
      ./autogen.sh
      make install
      ```
   This should install two binaries tvw_server and tvw_client in /usr/local/bin folder

## Executing
   1. Set env variables
   ```
   export LD_LIBRARY_PATH=/usr/local/lib/gstreamer-1.0:/usr/local/lib/arm-linux-gnueabihf/gstreamer-1.0:/usr/local/lib:/usr/local/lib64:/usr/lib64:/lib64:/opt/vc/lib:$LD_LIBRARY_PATH;
   export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/local/lib/arm-linux-gnueabihf/pkgconfig:/usr/lib64/pkgconfig:/opt/vc/lib/pkgconfig:$PKG_CONFIG_PATH;
   export GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0:/usr/lib/arm-linux-gnueabihf/gstreamer-1.0;
   sudo ldconfig
   ```
   2. Connect a UVC Camera on Server machine. This should be seen as /dev/video device
   3. Run the server as
      ```
      tvw_server <server.conf>
      ```
      conf file is present in conf directory of project
   4. Run clients on multiple machines as
      ```
      tvw_client <client.conf>
      ```
   Parameters can be adjusted in conf file based on client position in the TV wall

## Conf params
   conf folder has configuration files for both server and client. Server has one file while client has 4 files for 2x2 configuration
   Most of the params are self explanatory.
   1. decoder_lib : This can take following values
      ```
      mmal    : Uses mmal H264 hardware decoder
      omx     : Uses OMX H264 hardware decoder
      libjpeg : This is valid for jpeg stream. Uses software decoder from libjpeg
      ffmpeg  : Uses mmpeg software decoder
      ```
   2. video_driver : This can take following values
      ```
      mmal    : mmal graphics driver
      opengl  : Opengl driver
      x       : X window system
      ```
   3. iface : This is multicast ethernet interface. Can be either eth0 or wlan0, or any other value that is seen with ifconfig command
   4. sync : These are a set of parameters for multi device synchronization through clock
      The values are as follows
      ```
      clock_server : Could either be "ntp", "system", "dist", or "auto"
      When "ntp", NTP server's address and port information to be set.
      When "dist", out dist server's address and port information to be set.
      When "system", server/client uses its system clock
      When "auto", cleint uses its display device's clock(mostly mmal)
      ```
      In addition to using  amaster clock, server/client can redistribute its clock by setting dist=1 and its dist port
      Under that circumstance, any other machine can use this clock by setting its clock as dist and then dist server address and port
## Logging
   Log files will be generated in the log directory mentioned in conf file. There will be two log files generated, one for application and one for gstreamer backend


