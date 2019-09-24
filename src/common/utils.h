/* This is proprietery code and no permission is granted to copy or redistribute this
 */
#ifndef _UTILS_H_
#define _UTILS_H_

#include <gst/gst.h>
#include <cjson/cJSON.h>


extern int  gi_log_level;
extern char gac_log_file[128];
extern char gac_name[128];
extern volatile int gi_stop;

typedef enum _STOP_E
{
    STOP_NONE = 0,
    STOP_SIGINT,
    STOP_FATAL_ERROR,
    STOP_EOS
}STOP_E;

typedef enum _LOG_LEVEL_E_
{
    LOG_ERROR = 0,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG
}LOG_LEVEL_E;

/* utility macro for error checks */
#define ERR_CHECK_LOG_RET(expression, ret, format, ...)  if(expression) {if(gi_log_level == -1) printf(format, ##__VA_ARGS__);else log_write(LOG_ERROR, format, ##__VA_ARGS__);return(ret);};
#define ERR_CHECK_LOG(expression, ret, format, ...)  if(expression) {if(gi_log_level == -1) printf(format, ##__VA_ARGS__);else log_write(LOG_ERROR, format, ##__VA_ARGS__);};
#define ERR_CHECK_RET(expression, ret)               if(expression) {return(ret);};

/* Signal related */
#define SIG_FATAL_ERROR -100
#define SIG_EOS        0x100

/* API functions */
int log_init(int i_log_level, char *pc_log_dir, char *pc_name);
void log_write(int i_level, const char *ac_format, ...);

void sig_handler(int i_sig);
void sig_setup();

char *file_read_full(char *pc_file);

int json_get_int(cJSON *ps_parent, char *pc_name, int *out);
int json_get_double(cJSON *ps_parent, char *pc_name, double *out);
int json_get_string(cJSON *ps_parent, char *pc_name, char *out_string);

#endif //_UTILS_H_