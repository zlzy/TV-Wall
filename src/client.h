/* This is proprietery code and no permission is granted to copy or redistribute this
 */

#ifndef _CLIENT_H_
#define _CLIENT_H_

typedef struct _client_config_t
{
    char ac_name[1024];

    /* Input settings */
    char ac_ip[16];
    int  i_port;
    char ac_iface[16];
    int  i_dpktzr;
    int  i_dmux;
    int  i_codec;

    /* Output settings */
    char ac_decoder_lib[16];
    int  i_top;
    int  i_bottom;
    int  i_left;
    int  i_right;
    int  i_out_driver;

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
}client_config_t;

void  client_get_default(client_config_t *ps_stream_config);
void* client_start(client_config_t *ps_stream);
void  client_stop(void *pv_stream);

#endif /* _CLIENT_H_ */

