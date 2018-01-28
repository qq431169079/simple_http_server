/*
 * room_server.cpp
 *
 *  Created on: 2016年9月1日
 *      Author: wanghao
 */

#include <zlog.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <event.h>
#include <evhttp.h>
#include <errno.h>
#include <string>
#include "cjson/cJSON.h"

static int s_http_port = 7788;
static std::string s_http_addr = "0.0.0.0";

static void get_string_and_chomp(cJSON *pjson, std::string &str)
{
    char *ptmp = cJSON_Print(pjson);
    assert(ptmp != NULL);
    str = ptmp;
    free(ptmp);

    /* 因为从外部收到的内容是 "xxx", 要去掉 " */
    str.erase(0, 1);
    str.erase(str.end()-1);
}


static void reply(struct evhttp_request *req, const char *pbuff)
{
    assert(req != NULL);
    assert(pbuff != NULL);

    /* 设置http长连接 */
    evhttp_add_header(req->output_headers, "Proxy-Connection", "keep-alive");

    struct evbuffer *reply_buffer = evbuffer_new();
    evbuffer_add_printf(reply_buffer, pbuff);
    evhttp_send_reply(req, HTTP_OK, "OK", reply_buffer);
    evbuffer_free(reply_buffer);
    dzlog_debug("end");
}

static int load_config()
{
    const char *conf_file = "../conf/room_server.json";
    FILE *fp = NULL;
    struct stat stat_buff;
    char *pbuff = NULL;
    cJSON *proot = NULL, *paddr = NULL, *pport = NULL;

    fp = fopen(conf_file, "r");
    if (NULL == fp)
    {
        dzlog_error("open file failed, conf_file: %s, err: %s", conf_file, strerror(errno));
        goto ERR;
    }
    if (-1 == stat(conf_file, &stat_buff))
    {
        dzlog_error("call stat() failed, conf_file: %s, err: %s", conf_file, strerror(errno));
        goto ERR;
    }
    pbuff = (char *)calloc(1, stat_buff.st_size);
    assert(pbuff != NULL);

    if (1 != fread(pbuff, stat_buff.st_size, 1, fp))
    {
        dzlog_error("call fread failed, st_size: %d", stat_buff.st_size);
        goto ERR;
    }
    pbuff[stat_buff.st_size -1] = '\0';
    dzlog_debug("conf_file: %s", pbuff);

    proot = cJSON_Parse(pbuff);
    if (NULL == proot)
    {
        dzlog_error("call cJSON_Parse() failed, pbuff: %s", pbuff);
        goto ERR;
    }
    paddr = cJSON_GetObjectItem(proot, "ip");
    if (NULL != paddr)
    {
        get_string_and_chomp(paddr, s_http_addr);
    }
    pport = cJSON_GetObjectItem(proot, "port");
    if (NULL != pport)
    {
        s_http_port = pport->valueint;
    }
    dzlog_notice("addr: %s, port: %d", s_http_addr.c_str(), s_http_port);

    cJSON_Delete(proot);
    free(pbuff);
    fclose(fp);
    return 0;
ERR:
    if (NULL != proot)
    {
        cJSON_Delete(proot);
    }
    if (NULL != pbuff)
    {
        free(pbuff);
    }
    if (NULL != fp)
    {
        fclose(fp);
    }
    return -1;
}

static void process_reload_config_cb(struct evhttp_request *req, void *arg)
{
    if (-1 == load_config())
    {
        dzlog_error("call load_config() failed, load config faild again");
        return reply(req, "failed");
    }
    return reply(req, "success");
}

static void process_exit_cb(struct evhttp_request *req, void *arg)
{
    //return reply(req, "success");
    event_loopbreak();
}

static void process_test_null_cb(struct evhttp_request *req, void *arg)
{
    dzlog_debug("null");
    return reply(req, "success");
}

int main(int argc, char **argv)
{
    struct event_base *pbase = NULL;
    struct evhttp *http_server = NULL;

    if (-1 == dzlog_init("../conf/zlog.conf", "ROOM_SERVER"))
    {
        printf("call dzlog_init() failed\n");
        return -1;
    }
    dzlog_info("start");

    if (-1 == load_config())
    {
        dzlog_error("call load_config() failed");
        goto ERR;
    }

    pbase = event_init();
    assert(pbase != NULL);
    http_server = evhttp_start(s_http_addr.c_str(), s_http_port);
    assert(http_server != NULL);
    evhttp_set_timeout(http_server, 60);
    /* 从新加载配置文件 */
    evhttp_set_cb(http_server, "/reload_config", process_reload_config_cb, NULL);
    /* 退出程序 */
    evhttp_set_cb(http_server, "/exit", process_exit_cb, NULL);
    /* 测试空跑 */
    evhttp_set_cb(http_server, "/test_null", process_test_null_cb, NULL);


    dzlog_info("server started on port %d, enter loop ...", s_http_port);
    event_dispatch();
    dzlog_info("leave loop");

    evhttp_free(http_server);
    event_base_free(pbase);
    dzlog_info("end");

    zlog_fini();
    return 0;

ERR:
    zlog_fini();
    return -1;
}
