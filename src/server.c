/* This is proprietery code and no permission is granted to copy or redistribute this
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "common/utils.h"
#include "common/gst_utils.h"
#include <gst/net/gstnet.h>
#include "server.h"

typedef struct server_obj_t_
{
    server_config_t s_config;
    pipeline_obj_t s_pipeline_obj;
}server_obj_t;

void server_get_default(server_config_t *ps_server_config)
{
    memset(ps_server_config, 0, sizeof(server_config_t));
    ps_server_config->i_log_level = LOG_INFO;
}

gint32 server_check(server_config_t *ps_config)
{
    if(ps_config->ac_name[0] == '\0')
    {
        printf("Instance name cannot be empty\n");
        return(-1);
    }
    return(0);
}

/*
  Server will mainly have following elements
  src    - Either uvch264src or v4l2src
  capsfilter - For deciding camera output caps
  parser - Based on codec type
  mux/packetizer - rtppayloader or mpegtsmux
  Sink - udpsink
*/
void* server_start(server_config_t *ps_config)
{
    server_obj_t *ps_obj = NULL;
    GstElement *ps_src, *ps_cf, *ps_parse, *ps_mux, *ps_pktzr, *ps_sink;
    GstCaps *ps_caps;
    int i_status = 0;

    if(server_check(ps_config) != 0)
        return(NULL);

    ps_obj = malloc(sizeof(server_obj_t));
    ERR_CHECK_LOG_RET(ps_obj == NULL, NULL, "Unable to allocate memory for object");
    memset(ps_obj, 0, sizeof(server_obj_t));
    ps_obj->s_config = *ps_config;
    i_status = pipeline_create(&ps_obj->s_pipeline_obj, ps_config->cb_sig_handler,  ps_config->i_clock_type,
                    ps_config->ac_clock_host, ps_config->i_clock_port, ps_config->i_dist, ps_config->i_dist_port);
    ERR_CHECK_LOG_RET(i_status != 0, NULL, "Error in creating pipeline");

    /* Create the elements and then start linking them */
    /* Src */
    if(ps_config->i_in_codec == VIDEO_CODEC_H264)
    {
        /* We were creating uvch264src but that has some issues with other src pads being connected(vfsrc)*/
        ps_src = element_create(&ps_obj->s_pipeline_obj, "v4l2src");
        ps_caps = gst_caps_new_empty_simple("video/x-h264");
    }
    else
    {
        ps_src = element_create(&ps_obj->s_pipeline_obj, "v4l2src");
        ps_caps = gst_caps_new_empty_simple("image/jpeg");
    }
    ERR_CHECK_LOG_RET(ps_src == NULL, NULL, "Failed to create element source");
    g_object_set(G_OBJECT(ps_src), "device", ps_config->ac_device, NULL);

    /* Capsfilter for deciding caps based on codec type */
    ps_cf = element_create(&ps_obj->s_pipeline_obj, "capsfilter");
    ERR_CHECK_LOG_RET(ps_cf == NULL, NULL, "Failed to create element capsfilter");
    gst_caps_set_simple(ps_caps, "width",  G_TYPE_INT, ps_config->i_in_width, NULL);
    gst_caps_set_simple(ps_caps, "height", G_TYPE_INT, ps_config->i_in_height, NULL);
    g_object_set(G_OBJECT(ps_cf), "caps", ps_caps, NULL);
    gst_caps_unref(ps_caps);

    /* Parser */
    if(ps_config->i_in_codec == VIDEO_CODEC_H264)
        ps_parse = element_create(&ps_obj->s_pipeline_obj, "h264parse");
    else
        ps_parse = element_create(&ps_obj->s_pipeline_obj, "jpegparse");
    ERR_CHECK_LOG_RET(ps_parse == NULL, NULL, "Failed to create element videoparser");

    /* Muxer */
    if(ps_config->i_mux == MUX_MPEGTS)
        ps_mux = element_create(&ps_obj->s_pipeline_obj, "mpegtsmux");
    else
        ps_mux = element_create(&ps_obj->s_pipeline_obj, "identity");
    ERR_CHECK_LOG_RET(ps_parse == NULL, NULL, "Failed to create element muxer");

    /* Packetizer */
    if(ps_config->i_pktzr == PKTZR_NONE)
        ps_pktzr = element_create(&ps_obj->s_pipeline_obj, "identity");
    else if(ps_config->i_in_codec == VIDEO_CODEC_H264)
        ps_pktzr = element_create(&ps_obj->s_pipeline_obj, "rtph264pay");
    else
        ps_pktzr = element_create(&ps_obj->s_pipeline_obj, "rtpjpegpay");
    ERR_CHECK_LOG_RET(ps_parse == NULL, NULL, "Failed to create element pktzr");

    /* Output sink */
    ps_sink = element_create(&ps_obj->s_pipeline_obj, "udpsink");
    ERR_CHECK_LOG_RET(ps_sink == NULL, NULL, "Failed to create element udpsink");
    g_object_set (ps_sink, "port", ps_config->i_port, "host", ps_config->ac_ip, "multicast-iface", ps_config->ac_iface, NULL);

    i_status = gst_element_link_many(ps_src, ps_cf, ps_parse, ps_mux, ps_pktzr, ps_sink, NULL);
    ERR_CHECK_LOG_RET(i_status == FALSE, NULL, "Failed to link elements");

    /* Set the pipeline to playing */
    log_write(LOG_INFO, "Initization done");
    i_status = gst_element_set_state (ps_obj->s_pipeline_obj.ps_pipeline, GST_STATE_PLAYING);
    ERR_CHECK_LOG_RET(i_status == GST_STATE_CHANGE_FAILURE, NULL, "Error in setting pipeline state to playing");
    gst_debug_bin_to_dot_file (GST_BIN(ps_obj->s_pipeline_obj.ps_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, ps_config->ac_name);
    return(ps_obj);
}

void server_stop(void *pv_stream)
{
    server_obj_t *ps_obj = (server_obj_t *)pv_stream;
    if(ps_obj)
    {
        pipeline_delete(&ps_obj->s_pipeline_obj);
        free(ps_obj);
    }
    log_write(LOG_INFO, "Completed the server process");
    log_write(LOG_INFO, "---------------------------------------------------------");
}