/* This is proprietery code and no permission is granted to copy or redistribute this
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


#include "common/utils.h"
#include "common/gst_utils.h"
#include "gst/gstplay-enum.h"
#include "client.h"

typedef struct client_obj_t_
{
    client_config_t s_config;
    pipeline_obj_t s_pipeline_obj;

    /* For muxer */
    GstElement *ps_linker;
    int i_linked;
    int i_sw_dec_skip;
}client_obj_t;

void client_get_default(client_config_t *ps_client_config)
{
    memset(ps_client_config, 0, sizeof(client_config_t));
    ps_client_config->i_log_level       = LOG_INFO;
}

void cb_decodebin_pad_added(GstElement *ps_decode_bin, GstPad *ps_new_pad, gpointer ps_data)
{
    client_obj_t *ps_obj = (client_obj_t *) ps_data;
    GstCaps *ps_caps;
    GstStructure *ps_str;
    int i_status = TRUE;
    char *pc_pad_name;

    /* Check the properties of new pad, whether it is audio or video or anything else and based on that connect the pads with tee */
    ps_caps = gst_pad_query_caps (ps_new_pad, NULL);
    ps_str  = gst_caps_get_structure (ps_caps, 0);

    if((g_strrstr(gst_structure_get_name (ps_str), "video")))
    {
        if(ps_obj->i_linked == 1)
        {
            log_write(LOG_WARNING, "Ignoring signal. Video already linked");
            return;
        }
        log_write(LOG_INFO, "Video stream added");
    }
    else if((g_strrstr(gst_structure_get_name (ps_str), "audio")))
    {
        log_write(LOG_WARNING, "Ignoring the signal, invalid media");
    }

    pc_pad_name = gst_pad_get_name(ps_new_pad);
    i_status = gst_element_link_pads(ps_decode_bin, pc_pad_name, ps_obj->ps_linker, "sink");
    if(i_status != TRUE)
    {
        log_write(LOG_ERROR,"Cannot link with muxer pad, error : %d", i_status);
        return;
    }
    g_free(pc_pad_name);

    /* Unref the caps */
    gst_caps_unref(ps_caps);
	return;
}

static GstAutoplugSelectResult cb_decodebin_autoplug_select(GstElement *ps_decodebin, GstPad *ps_pad,
    GstCaps *ps_caps, GstElementFactory *ps_factory, client_obj_t *ps_obj)
{
    /* Check if we need to use hardware or software client */
    gboolean b_isvideodec = gst_element_factory_list_is_type (ps_factory,
        GST_ELEMENT_FACTORY_TYPE_DECODER |
        GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO |
        GST_ELEMENT_FACTORY_TYPE_MEDIA_IMAGE);
    if(b_isvideodec)
    {
        char *ps_name = GST_OBJECT_NAME (ps_factory);
        if((strncasecmp(ps_name, "omx", 3) == 0) && (strcmp(ps_obj->s_config.ac_decoder_lib, "omx") == 0))
            return GST_AUTOPLUG_SELECT_TRY;
        else if((strncasecmp(ps_name, "mmal", 4) == 0) && (strcmp(ps_obj->s_config.ac_decoder_lib, "mmal") == 0))
            return GST_AUTOPLUG_SELECT_TRY;
        else if((strncasecmp(ps_name, "jpegdec", 7) == 0) && (strcmp(ps_obj->s_config.ac_decoder_lib, "libjpeg") == 0))
            return GST_AUTOPLUG_SELECT_TRY;
        else if((strncasecmp(ps_name, "avdec", 5) == 0) && (strcmp(ps_obj->s_config.ac_decoder_lib, "ffmpeg") == 0))
            return GST_AUTOPLUG_SELECT_TRY;
        else
            return GST_AUTOPLUG_SELECT_SKIP;
    }
    else {
        return GST_AUTOPLUG_SELECT_TRY;
    }
}

gint32 client_check(client_config_t *ps_config)
{
    if(ps_config->ac_name[0] == '\0')
    {
        printf("Instance name cannot be empty\n");
        return(-1);
    }
    return(0);
}


/*
  Client will mainly have following elements
  src  - Multicast UDP input
  capsfilter - For RTP Caps
  depayloader for rtp - Based on codec type
  decodebin - mpegtsmux
  videocrop
  videoconvert
  Videosink
*/
void* client_start(client_config_t *ps_config)
{
    client_obj_t *ps_obj = NULL;
    GstElement *ps_src, *ps_cf, *ps_dpktzr, *ps_decodebin, *ps_vcrop, *ps_vconvert, *ps_sink = NULL;
    GstCaps *ps_caps;
    int i_status = 0;
    char ac_name[64];

    if(client_check(ps_config) != 0)
        return(NULL);

    ps_obj = malloc(sizeof(client_obj_t));
    ERR_CHECK_LOG_RET(ps_obj == NULL, NULL, "Unable to allocate memory for object");
    memset(ps_obj, 0, sizeof(client_obj_t));
    ps_obj->s_config = *ps_config;
    i_status = pipeline_create(&ps_obj->s_pipeline_obj, ps_config->cb_sig_handler, ps_config->i_clock_type,
                    ps_config->ac_clock_host, ps_config->i_clock_port, ps_config->i_dist, ps_config->i_dist_port);
    ERR_CHECK_LOG_RET(i_status != 0, NULL, "Error in creating pipeline");

    /* Create the elements and then start linking them */
    /* Src */
    ps_src = element_create(&ps_obj->s_pipeline_obj, "udpsrc");
    ERR_CHECK_LOG_RET(ps_src == NULL, NULL, "Element creation failed");
    g_object_set (ps_src, "port", ps_config->i_port, NULL);
    g_object_set (ps_src, "address", ps_config->ac_ip, NULL);
    g_object_set (ps_src, "multicast-iface", ps_config->ac_iface, NULL);

    /* Capsfilter for deciding caps based on codec type */
    ps_cf = element_create(&ps_obj->s_pipeline_obj, "capsfilter");
    ERR_CHECK_LOG_RET(ps_cf == NULL, NULL, "Element creation failed");
    if(ps_config->i_dpktzr == PKTZR_RTP)
    {
        ps_caps = gst_caps_new_empty_simple("application/x-rtp");
        gst_caps_set_simple(ps_caps, "clock-rate",  G_TYPE_INT, 90000, NULL);
        if(ps_config->i_codec == VIDEO_CODEC_MJPEG)
        {
            gst_caps_set_simple(ps_caps, "payload", G_TYPE_INT, 26, NULL);
            ps_dpktzr = element_create(&ps_obj->s_pipeline_obj, "rtpjpegdepay");
        }
        else if(ps_config->i_codec == VIDEO_CODEC_H264)
        {
            gst_caps_set_simple(ps_caps, "payload", G_TYPE_INT, 96, NULL);
            ps_dpktzr = element_create(&ps_obj->s_pipeline_obj, "rtph264depay");
        }
        else
            ps_dpktzr = NULL;
        g_object_set(G_OBJECT(ps_cf), "caps", ps_caps, NULL);
        gst_caps_unref(ps_caps);
    }
    else
    {
        ps_dpktzr = element_create(&ps_obj->s_pipeline_obj, "identity");
    }
    ERR_CHECK_LOG_RET(ps_dpktzr == NULL, NULL, "Element creation failed");

    ps_decodebin = element_create(&ps_obj->s_pipeline_obj, "decodebin");
    ERR_CHECK_LOG_RET(ps_decodebin == NULL, NULL, "Element creation failed");
    g_signal_connect(ps_decodebin, "pad-added", G_CALLBACK(cb_decodebin_pad_added), ps_obj);
    g_signal_connect(ps_decodebin, "autoplug-select", G_CALLBACK(cb_decodebin_autoplug_select), ps_obj);


    if(ps_config->i_out_driver == VIDEO_DRIVER_X)
        ps_sink = element_create(&ps_obj->s_pipeline_obj, "ximagesink");
    else if(ps_config->i_out_driver == VIDEO_DRIVER_OPENGL)
        ps_sink = element_create(&ps_obj->s_pipeline_obj, "glimagesink");
    else if(ps_config->i_out_driver == VIDEO_DRIVER_MMAL)
        ps_sink = element_create(&ps_obj->s_pipeline_obj, "mmalvideosink");
    else
        ps_sink = NULL;
    ERR_CHECK_LOG_RET(ps_sink == NULL, NULL, "Element creation failed");

    /* Connect src to parsebin, and muxer to sink, codec parser to queue. Rest of the link happen on the fly */
    i_status = gst_element_link_many(ps_src, ps_cf, ps_dpktzr, ps_decodebin, NULL);
    ERR_CHECK_LOG_RET(i_status != TRUE, NULL, "Link1 inking failed");
    /* If the decoder is mmal, it cant accept anything between decoder and sink. Else, add crop and convert */
    if(strcmp(ps_obj->s_config.ac_decoder_lib, "mmal") == 0) {
        ps_obj->ps_linker = ps_sink;
    }
    else {
        ps_vcrop = element_create(&ps_obj->s_pipeline_obj, "videocrop");
        ERR_CHECK_LOG_RET(ps_vcrop == NULL, NULL, "Element creation failed");
        g_object_set (ps_vcrop, "top", ps_config->i_top, "bottom", ps_config->i_bottom, "left", ps_config->i_left, "right", ps_config->i_right, NULL);
        ps_obj->ps_linker = ps_vcrop;

        ps_vconvert = element_create(&ps_obj->s_pipeline_obj, "videoconvert");
        ERR_CHECK_LOG_RET(ps_vconvert == NULL, NULL, "Element creation failed");

        i_status = gst_element_link_many(ps_vcrop, ps_vconvert, ps_sink, NULL);
        ERR_CHECK_LOG_RET(i_status != TRUE, NULL, "Link2 link failed");
    }

    /* Set the pipeline to playing */
    i_status = gst_element_set_state (ps_obj->s_pipeline_obj.ps_pipeline, GST_STATE_PLAYING);
    ERR_CHECK_LOG_RET(i_status == GST_STATE_CHANGE_FAILURE, NULL, "Error in setting pipeline state to playing");
    sprintf(ac_name, "%s_starting", ps_config->ac_name);
    gst_debug_bin_to_dot_file (GST_BIN(ps_obj->s_pipeline_obj.ps_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, ac_name);

    return(ps_obj);
}

void client_stop(void *pv_stream)
{
    client_obj_t *ps_obj = (client_obj_t *)pv_stream;
    char ac_name[128];
    if(ps_obj)
    {
        sprintf(ac_name, "%s_stopping", ps_obj->s_config.ac_name);
        gst_debug_bin_to_dot_file (GST_BIN(ps_obj->s_pipeline_obj.ps_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, ac_name);
            pipeline_delete(&ps_obj->s_pipeline_obj);
            free(ps_obj);
    }
    log_write(LOG_INFO, "Completed the client process");
    log_write(LOG_INFO, "---------------------------------------------------------");
}
