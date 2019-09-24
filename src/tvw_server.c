/* This is proprietery code and no permission is granted to copy or redistribute this
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/types.h>

#include "common/utils.h"
#include "common/gst_utils.h"
#include "server.h"

void print_help()
{
    printf("--conf : Configuration file\n");
}

int parse_server_config(char *pc_conf_file, server_config_t *ps_conf)
{
    char *pc_json, ac_temp[128];
    cJSON *ps_json_conf = NULL, *ps_parent;
    pc_json = file_read_full(pc_conf_file);
    ERR_CHECK_LOG_RET(pc_json == NULL, -1, "Error in reading conf file");

    /* Parse the json using C json API */
    ps_parent = ps_json_conf  = cJSON_Parse(pc_json);
    json_get_string(ps_parent, "name", ps_conf->ac_name);

    ps_parent = cJSON_GetObjectItem(ps_json_conf, "input");
    json_get_string(ps_parent, "camera", ac_temp);
    param_str_to_enum("camera", ac_temp, &ps_conf->i_in_camera);
    json_get_string(ps_parent, "codec", ac_temp);
    param_str_to_enum("codec", ac_temp, &ps_conf->i_in_codec);
    json_get_int   (ps_parent, "width", &ps_conf->i_in_width);
    json_get_int   (ps_parent, "height", &ps_conf->i_in_height);
    json_get_int   (ps_parent, "bitrate", &ps_conf->i_in_bitrate);
    json_get_string(ps_parent, "device", ps_conf->ac_device);

    ps_parent = cJSON_GetObjectItem(ps_json_conf, "output");
    json_get_string(ps_parent, "codec", ac_temp);
    param_str_to_enum("codec", ac_temp, &ps_conf->i_out_codec);
    json_get_int   (ps_parent, "bitrate", &ps_conf->i_out_bitrate);
    json_get_string(ps_parent, "mux", ac_temp);
    param_str_to_enum("mux", ac_temp, &ps_conf->i_mux);
    json_get_string(ps_parent, "pktzr", ac_temp);
    param_str_to_enum("pktzr", ac_temp, &ps_conf->i_pktzr);
    json_get_string(ps_parent, "ip", ps_conf->ac_ip);
    json_get_string(ps_parent, "iface", ps_conf->ac_iface);
    json_get_int   (ps_parent, "port", &ps_conf->i_port);

    ps_parent = cJSON_GetObjectItem(ps_json_conf, "sync");
    json_get_string(ps_parent, "clock_type", ac_temp);
    param_str_to_enum("clock_type", ac_temp, &ps_conf->i_clock_type);
    json_get_string(ps_parent, "clock_host", ps_conf->ac_clock_host);
    json_get_int(ps_parent, "clock_port", &ps_conf->i_clock_port);
    json_get_int(ps_parent, "dist", &ps_conf->i_dist);
    json_get_int(ps_parent, "dist_port", &ps_conf->i_dist_port);

    ps_parent = cJSON_GetObjectItem(ps_json_conf, "logging");
    json_get_string(ps_parent, "log_dir",ps_conf->ac_log_dir);
    json_get_string(ps_parent, "log_level", ac_temp);
    param_str_to_enum("log_level", ac_temp, &ps_conf->i_log_level);
    return(0);
}

/* Entry point. Takes in arguments from command line and uses this as configuration */
int main (int argc, char *argv[])
{
    int i_stop_reason = STOP_NONE, i_status;
    void *pv_server_obj = NULL;
    server_config_t s_conf;

    if(argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
    {
        print_help();
        return(0);
    }

    /* Set all params to default */
    server_get_default(&s_conf);

    /* Parse the arguments */
    i_status = parse_server_config(argv[1], &s_conf);
    ERR_CHECK_LOG_RET(i_status != 0, -1, "Error in server conf file. Exiting");

    /* Initialize log */
    log_init(s_conf.i_log_level, s_conf.ac_log_dir, s_conf.ac_name);

    /* Setup signals */
    sig_setup();
    s_conf.cb_sig_handler = sig_handler;

    while(1)
    {
        pv_server_obj = server_start(&s_conf);
        if(pv_server_obj == NULL)
        {
            log_write(LOG_ERROR, "Error in creating server. Exiting");
            server_stop(pv_server_obj);
            return(-1);
        }
        log_write(LOG_ERROR, "Server started");
        printf("Server started\n");

        while(gi_stop == STOP_NONE)
            usleep(10000);
        /* Save this since gi_stop can change later */
        i_stop_reason = gi_stop;
        server_stop(pv_server_obj);
        printf("Server stopped\n");
        if(i_stop_reason == STOP_SIGINT)
            break;
        else
            gi_stop = STOP_NONE;
    }
    return(0);
}
