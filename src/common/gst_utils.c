/* This is proprietery code and no permission is granted to copy or redistribute this
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"
#include "gst_utils.h"
#include <gst/net/gstnet.h>

GstBusSyncReply cb_bus_message(GstBus *ps_bus, GstMessage *ps_message, gpointer user_data)
{
    pipeline_obj_t *ps_pipeline_obj = (pipeline_obj_t *)user_data;
	GError *ps_err = NULL;
	gchar *debug   = NULL;

    /* Check what the message type is */
    switch(GST_MESSAGE_TYPE (ps_message))
    {
    case GST_MESSAGE_ERROR:
		gst_message_parse_error (ps_message, &ps_err, &debug);
        log_write(LOG_ERROR, "%s", ps_err->message);
        gst_debug_bin_to_dot_file (GST_BIN(ps_pipeline_obj->ps_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "error");
        ps_pipeline_obj->i_stopped = 1;
        ps_pipeline_obj->cb_sig_handler(SIG_FATAL_ERROR);
		break;
	case GST_MESSAGE_WARNING:
    	gst_message_parse_warning (ps_message, &ps_err, &debug);
        log_write(LOG_WARNING, "%s", ps_err->message);
		break;
	case GST_MESSAGE_EOS:
        log_write(LOG_INFO, "Reached EOS");
        ps_pipeline_obj->i_stopped = 1;
        ps_pipeline_obj->cb_sig_handler(SIG_EOS);
        break;
    default:
        log_write(LOG_DEBUG, "Unknown message");
        break;
    }
	if(ps_err)
        g_error_free (ps_err);
	if(debug)
        g_free (debug);
    /* remove message from the queue */
    return GST_BUS_PASS;
}

int pipeline_create(pipeline_obj_t *ps_pipeline_obj, void  (*cb_sig_handler) (int), gint32 i_clock_type,
                    char *pc_host, int i_port, int i_dist, int i_dist_port)
{
    GstBus *ps_bus = NULL;
    memset(ps_pipeline_obj, 0, sizeof(pipeline_obj_t));
    GstClock *ps_clock = NULL;

    /* Init Gstreamer components */
    gst_init(0, NULL);

    /* Create the pipeline */
    ps_pipeline_obj->ps_pipeline = gst_pipeline_new("pipeline");
    ERR_CHECK_LOG_RET(ps_pipeline_obj->ps_pipeline == NULL, -1, "Error in creating pipeline");

	/* Add a bus to the pipeline */
    ps_bus = gst_pipeline_get_bus (GST_PIPELINE(ps_pipeline_obj->ps_pipeline));
    gst_bus_set_sync_handler (ps_bus, cb_bus_message, (gpointer) ps_pipeline_obj, NULL);
    gst_object_unref(ps_bus);

    ps_pipeline_obj->cb_sig_handler = cb_sig_handler;

    if(i_clock_type == CLOCK_NTP) {
        ps_clock = gst_ntp_clock_new("NTPclock", pc_host, i_port, 0);
        ERR_CHECK_LOG_RET(ps_clock == NULL, -1, "Could not fetch NTP clock");
    }
    else if(i_clock_type == CLOCK_DIST) {
        ps_clock = gst_net_client_clock_new("DISTclock", pc_host, i_port, 0);
        ERR_CHECK_LOG_RET(ps_clock == NULL, -1, "Could not fetch Dist clock");
    }
    else if(i_clock_type == CLOCK_SYSTEM) {
        ps_clock = gst_system_clock_obtain();
        ERR_CHECK_LOG_RET(ps_clock == NULL, -1, "Could not fetch system clock");
    }
    else
        ps_clock = NULL;
    /* Set it on the pipeline */
    if(ps_clock) {
        gst_pipeline_use_clock(GST_PIPELINE(ps_pipeline_obj->ps_pipeline), ps_clock);
        /* If the clock is to be sitributed, then we configure clock server here */
        if(i_dist) {
            GstNetTimeProvider *ps_nettime = gst_net_time_provider_new(ps_clock, NULL, i_dist_port);
            ERR_CHECK_LOG_RET(ps_nettime == NULL, -1, "Could not distribute the clock");
        }
    }
    return(0);
}

GstElement *element_create(pipeline_obj_t *ps_pipeline_obj, char *pc_name)
{
    GstElement *ps_element = gst_element_factory_make(pc_name, NULL);
    if(ps_element == NULL)
    {
        log_write(LOG_ERROR, "Error in creating element %s", pc_name);
        return(NULL);
    }
    gst_bin_add_many(GST_BIN(ps_pipeline_obj->ps_pipeline), ps_element, NULL);
    return(ps_element);
}

void pipeline_delete(pipeline_obj_t *ps_pipeline_obj)
{
    int i_status = 0;

    /* Send an EOS */
    if(ps_pipeline_obj->i_stopped == 0)
    {
        gst_element_send_event (ps_pipeline_obj->ps_pipeline, gst_event_new_eos ());

        /* Wait for EOS to be caught */
        while(ps_pipeline_obj->i_stopped == 0)
            usleep(1000);
    }

    i_status = gst_element_set_state (ps_pipeline_obj->ps_pipeline, GST_STATE_NULL);
    if(i_status == GST_STATE_CHANGE_FAILURE)
        log_write(LOG_ERROR, "Error in setting pipeline state to NULL");
}

int param_str_to_enum(char *pc_type, char *pc_str, int *pi_int)
{
    if(strcasecmp(pc_type, "camera") == 0)
    {
        if(strcasecmp(pc_str, "uvc") == 0)
            *pi_int = CAMERA_UVC;
        else
            *pi_int = CAMERA_NA;
    }
    else if(strcasecmp(pc_type, "codec") == 0)
    {
        if(strcasecmp(pc_str, "passthrough") == 0)
            *pi_int = VIDEO_CODEC_PASSTHROUGH;
        else if(strcasecmp(pc_str, "raw") == 0)
            *pi_int = VIDEO_CODEC_RAW;
        else if(strcasecmp(pc_str, "mjpeg") == 0)
            *pi_int = VIDEO_CODEC_MJPEG;
        else if(strcasecmp(pc_str, "h264") == 0)
            *pi_int = VIDEO_CODEC_H264;
        else
            *pi_int = CAMERA_NA;
    }
    else if(strcasecmp(pc_type, "mux") == 0)
    {
        if(strcasecmp(pc_str, "mpegts") == 0)
            *pi_int = MUX_MPEGTS;
        else
            *pi_int = MUX_NONE;
    }
    else if(strcasecmp(pc_type, "pktzr") == 0)
    {
        if(strcasecmp(pc_str, "rtp") == 0)
            *pi_int = PKTZR_RTP;
        else
            *pi_int = PKTZR_NONE;
    }
    else if(strcasecmp(pc_type, "log_level") == 0)
    {
        if(strcasecmp(pc_str, "error") == 0)
            *pi_int = LOG_ERROR;
        else if(strcasecmp(pc_str, "warning") == 0)
            *pi_int = LOG_WARNING;
        else if(strcasecmp(pc_str, "info") == 0)
            *pi_int = LOG_INFO;
        else
            *pi_int = LOG_DEBUG;
    }
    else if(strcasecmp(pc_type, "video_driver") == 0)
    {
        if(strcasecmp(pc_str, "x") == 0)
            *pi_int = VIDEO_DRIVER_X;
        else if(strcasecmp(pc_str, "opengl") == 0)
            *pi_int = VIDEO_DRIVER_OPENGL;
        else if(strcasecmp(pc_str, "mmal") == 0)
            *pi_int = VIDEO_DRIVER_MMAL;
        else
            *pi_int = VIDEO_DRIVER_NONE;
    }
    else if(strcasecmp(pc_type, "clock_type") == 0)
    {
        if(strcasecmp(pc_str, "auto") == 0)
            *pi_int = CLOCK_AUTO;
        else if(strcasecmp(pc_str, "system") == 0)
            *pi_int = CLOCK_SYSTEM;
        else if(strcasecmp(pc_str, "capture_device") == 0)
            *pi_int = CLOCK_CAPTURE_DEVICE;
        else if(strcasecmp(pc_str, "render_device") == 0)
            *pi_int = CLOCK_RENDER_DEVICE;
        else if(strcasecmp(pc_str, "ntp") == 0)
            *pi_int = CLOCK_NTP;
        else if(strcasecmp(pc_str, "dist") == 0)
            *pi_int = CLOCK_DIST;
        else
            *pi_int = CLOCK_AUTO;
    }
    else
    {
        log_write(LOG_WARNING, "Unknown enum type %s", pc_type);
    }
    return 0;
}
