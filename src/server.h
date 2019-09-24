/* This is proprietery code and no permission is granted to copy or redistribute this
 */

#ifndef _SERVER_H_
#define _SERVER_H_

typedef struct _server_config_t
{
    char ac_name[1024];

    /* Input settings */
    int  i_in_camera;
    int  i_in_codec;
    int  i_in_width;
    int  i_in_height;
    int  i_in_bitrate;
    char ac_device[1024];

    /* Output settings */
    int  i_out_codec;
    int  i_out_bitrate;
    int  i_mux;
    int  i_pktzr;
    char ac_ip[1024];
    int  i_port;
    char ac_iface[1024];

    /* Sync */
    int i_clock_type;
    char ac_clock_host[256];
    int i_clock_port;
    int i_dist;
    int i_dist_port;

    /* Logging */
    char ac_log_dir[1024];
    int  i_log_level;

    /* Sig hanlder */
    void  (*cb_sig_handler) (int);
}server_config_t;

void  server_get_default(server_config_t *ps_stream_config);
void* server_start(server_config_t *ps_stream);
void  server_stop(void *pv_stream);

#endif /* _SERVER_H_ */

