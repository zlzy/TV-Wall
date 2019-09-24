/* This is proprietery code and no permission is granted to copy or redistribute this
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

int   gi_log_level = -1;
char  gac_log_file[128];
char  gac_name[128];
volatile int gi_stop = STOP_NONE;

int log_init(int i_log_level, char *pc_log_dir, char *pc_name)
{
    int i_status = 0;
    gchar ac_gst_log_file[256], ac_log_dir[256];

    /* Get log file names and initialize globals */
    if(pc_log_dir[0] == '\0')
        strcpy(ac_log_dir, "/root");
    else
        strcpy(ac_log_dir, pc_log_dir);
    sprintf(gac_log_file, "%s/%s.log", pc_log_dir, pc_name);
    sprintf(ac_gst_log_file, "%s/%s_gst.log", pc_log_dir, pc_name);
    gi_log_level = i_log_level;
    strcpy(gac_name, pc_name);

    /* Set gstreamer specific environmental variables */
    i_status = setenv("GST_DEBUG_FILE", ac_gst_log_file, 1);
    ERR_CHECK_LOG_RET(i_status != 0, -1, "Error in setting env variable1");
    i_status = setenv("GST_DEBUG", "2", 1);
    ERR_CHECK_LOG_RET(i_status != 0, -1, "Error in setting env variable2");
    return 0;
}

void log_write(int i_level, const char *ac_format, ...)
{
    char ac_temp[1024], *ac_prefix[4] = {"ERR : ", "WRN : ", "INF : ", "DBG : "};
    FILE *fp_log = NULL;

    /* Go ahead only if log needs to be printed based on level */
    if(i_level > gi_log_level)
        return;

    char buff[20];
    struct tm *sTm;

    time_t now = time (0);
    sTm = gmtime (&now);

    strftime (buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", sTm);

	va_list va_args;
    va_start(va_args, ac_format);
	vsprintf(ac_temp, ac_format, va_args);
    va_end(va_args);

    /* If log pointer is present, then log into file. Else, log into console */
    if((fp_log = fopen(gac_log_file, "a")) != NULL)
    {
        if(gac_name[0] != '\0')
        {
            fwrite(gac_name, strlen(gac_name), 1, fp_log);
            fwrite(" - ", strlen(" - "), 1, fp_log);
        }
        fwrite(buff, strlen(buff), 1, fp_log);
        fwrite(" - ", strlen(" - "), 1, fp_log);
        fwrite(ac_prefix[i_level], strlen(ac_prefix[i_level]), 1, fp_log);
        fwrite(ac_temp, strlen(ac_temp), 1, fp_log);
        fwrite("\n", 1, 1, fp_log);
        fclose(fp_log);
    }
    else
    {
        printf("%s%s\n", ac_prefix[i_level], ac_temp);
    }
}

void sig_handler(int i_sig)
{
    switch(i_sig)
    {
    /* System signals */
    case SIGINT:
        /* Set the interrupt flag for main context to exit */
		gi_stop = STOP_SIGINT;
        log_write(LOG_INFO, "Received SIGINT. Stopping streaming");
        break;
    case SIG_FATAL_ERROR:
        /* Set the interrupt flag for main context to exit */
		gi_stop = STOP_FATAL_ERROR;
        log_write(LOG_INFO, "Received FATAL Error. Stopping streaming");
        break;
    case SIG_EOS:
        /* Set the interrupt flag for main context to exit */
		gi_stop = STOP_EOS;
        log_write(LOG_INFO, "Received EOS. Stopping streaming");
        break;
    default:
        log_write(LOG_INFO, "Received unknown signal. Ignoring");
        break;
    }
}

void sig_setup()
{
    /* This is to handle interrupt */
    struct sigaction action;

    /* To make child terminate instantly after shutting down */
    signal(SIGCHLD, SIG_IGN);

    memset (&action, 0, sizeof (action));
    action.sa_handler = sig_handler;
    sigaction(SIGINT,  &action, NULL);
}

char *file_read_full(char *pc_file)
{
    int i_size = 0;
    char *pc_data;

    /* Open the file and read the whole json in a string */
    FILE *fp = fopen(pc_file, "r");
    ERR_CHECK_LOG_RET(fp == NULL, NULL, "Error in opening file");

    fseek(fp, 0, SEEK_END);
    i_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    pc_data = (char *)malloc(i_size + 1);
    ERR_CHECK_LOG_RET(pc_data == NULL, NULL, "Error in allocation memory");

    fread(pc_data, 1, i_size, fp);
    pc_data[i_size] = 0;
    fclose(fp);

    return(pc_data);
}

int json_get_int(cJSON *ps_parent, char *pc_name, int *out)
{
    cJSON *ps_param;
    ps_param = cJSON_GetObjectItem(ps_parent, pc_name);
    ERR_CHECK_LOG_RET(ps_param == NULL, -1, "Field(int) missing in json : %s", pc_name);
    *out = ps_param->valueint;
    return(0);
}

int json_get_double(cJSON *ps_parent, char *pc_name, double *out)
{
    cJSON *ps_param;
    ps_param = cJSON_GetObjectItem(ps_parent, pc_name);
    ERR_CHECK_LOG_RET(ps_param == NULL, -1, "Field(double) missing in json : %s", pc_name);
    *out = ps_param->valuedouble;
    return(0);
}

int json_get_string(cJSON *ps_parent, char *pc_name, char *out_string)
{
    cJSON *ps_param;
    ps_param = cJSON_GetObjectItem(ps_parent, pc_name);
    ERR_CHECK_LOG_RET(ps_param == NULL, -1, "Field(string) missing in json : %s", pc_name);
    strcpy(out_string, ps_param->valuestring);
    return(0);
}

