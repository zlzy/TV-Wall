/* This is proprietery code and no permission is granted to copy or redistribute this
 */
#ifndef _GST_COMMON_H
#define _GST_COMMON_H

#include <gst/gst.h>

typedef enum _CAMERA_TYPE_E_
{
    CAMERA_NA,
    CAMERA_UVC
}CAMERA_TYPE_E;

typedef enum _VIDEO_CODEC_E_
{
    VIDEO_CODEC_NA,
    VIDEO_CODEC_PASSTHROUGH,
    VIDEO_CODEC_RAW,
    VIDEO_CODEC_MJPEG,
    VIDEO_CODEC_H264
}VIDEO_CODEC_E;

typedef enum _MUX_E_
{
    MUX_NONE,
    MUX_MPEGTS
}MUX_E;

typedef enum _PKTZR_E_
{
    PKTZR_NONE,
    PKTZR_RTP
}PKTZR_E;

typedef enum _VIDEO_DRIVER_E_
{
    VIDEO_DRIVER_NONE = -1,
    VIDEO_DRIVER_X,
    VIDEO_DRIVER_OPENGL,
    VIDEO_DRIVER_MMAL
}VIDEO_DRIVER_E;

typedef enum {
    /* Hardware clocks */
    CLOCK_AUTO,
    CLOCK_SYSTEM,
    CLOCK_CAPTURE_DEVICE,
    CLOCK_RENDER_DEVICE,

    /* Network clocks */
    CLOCK_NTP,
    CLOCK_DIST /* Distributed clock */
}GSTL_CLOCK_E;

typedef struct pipeline_obj_t_
{
    GstElement *ps_pipeline;
    volatile int i_stopped;
    void  (*cb_sig_handler) (int);
}pipeline_obj_t;

GstBusSyncReply cb_message(GstBus *ps_bus, GstMessage *ps_message, gpointer user_data);
GstElement *element_create(pipeline_obj_t *ps_pipeline_obj, char *pc_name);
int pipeline_create(pipeline_obj_t *ps_pipeline_obj, void  (*cb_sig_handler) (int), gint32 i_clock_type,
                    char *pc_host, int i_port, int i_dist, int i_dist_port);
void pipeline_delete(pipeline_obj_t *ps_pipeline_obj);

int  param_str_to_enum(char *pc_type, char *pc_str, int *pi_int);

#endif //_GST_COMMON_H