/*
 * room_server.cpp
 *
 *  Created on: 2016年9月1日
 *      Author: wanghao
 */

#include "room_server.h"
#include "cjson/cJSON.h"
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

static int s_http_port = 7788;
static std::string s_http_addr = "0.0.0.0";

/**< 大房间号到大房间结构体的映射 */
static std::map<std::string, struct xy_big_room_t *> s_all_big_room;

/**< 子房间最多的人数 */
static int s_most_sub_room_user_num = 1000;

/**< 子房间人数少于最多子房间人数的比例 */
static int s_least_user_percent = 30;

static void print_all_userid()
{
	std::map<std::string, struct xy_big_room_t *>::iterator big_room_it;
	std::map<int, struct xy_sub_room_t *>::iterator sub_room_it;
	std::map<std::string, int>::iterator user_it;
	struct xy_big_room_t *pbig_room = NULL;
	struct xy_sub_room_t *psub_room = NULL;

	for (big_room_it = s_all_big_room.begin(); big_room_it != s_all_big_room.end(); ++big_room_it)
	{
		pbig_room = big_room_it->second;
		for (sub_room_it = pbig_room->sub_roomid_map.begin(); sub_room_it != pbig_room->sub_roomid_map.end(); ++sub_room_it)
		{
			psub_room = sub_room_it->second;
			dzlog_notice("big_roomid: %s, sub_roomid: %d, user_num: %d", psub_room->big_roomid.c_str(), psub_room->sub_roomid, psub_room->user_num);
		}
		for (user_it = pbig_room->userid_map_sub_roomid.begin(); user_it != pbig_room->userid_map_sub_roomid.end(); ++user_it)
		{
			dzlog_notice("sub_roomid: %d, userid: %s", user_it->second, user_it->first.c_str());
		}
	}
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

static void get_string(cJSON *pjson, std::string &str)
{
	char *ptmp = cJSON_Print(pjson);
	assert(ptmp != NULL);
	str = ptmp;
	free(ptmp);
}

static int http_request_to_room_head(struct evhttp_request *req, struct room_head_t *phead)
{
	assert(req != NULL);
	assert(phead != NULL);
	
	char *pbuff = NULL;
	cJSON *proot = NULL, *pbig_roomid = NULL, *puserid = NULL;


	pbuff = (char *)EVBUFFER_DATA(req->input_buffer);
	proot = cJSON_Parse(pbuff);
	if (proot == NULL)
	{
		dzlog_debug("call CJSON_Parse() failed");
		goto ERR;
	}
	pbig_roomid = cJSON_GetObjectItem(proot, "roomid");
	if (pbig_roomid == NULL)
	{
		dzlog_debug("cJSON_GetObjectItem() failed, get roomid failed");
		goto ERR;
	}
	puserid = cJSON_GetObjectItem(proot, "userid");
	if (puserid == NULL)
	{
		dzlog_debug("cJSON_GetObjectItem() failed, get userid failed");
		goto ERR;
	}
	get_string_and_chomp(pbig_roomid, phead->big_roomid);
	get_string_and_chomp(puserid, phead->userid);

	dzlog_debug("cmd: %s, big_roomid: %s, userid: %s", phead->request_cmd.c_str(), phead->big_roomid.c_str(), phead->userid.c_str());

	cJSON_Delete(proot);
	return 0;
ERR:
	if (proot != NULL)
	{
		cJSON_Delete(proot);
	}
	dzlog_debug("http request buff: %s", pbuff);
	return -1;
}

static void generate_reply_msg(struct xy_reply_msg_t *preply_msg, std::string &result_msg)
{
	assert(preply_msg != NULL);

	std::vector<int>::iterator it;
	cJSON *proot = NULL, *pleast_user = NULL;

	proot=cJSON_CreateObject();
	cJSON_AddStringToObject(proot, "cmd", "XY_ROOM_REPLY_IN");
	cJSON_AddNumberToObject(proot, "sub_roomid", preply_msg->sub_roomid);
	cJSON_AddNumberToObject(proot, "most_user_sub_roomid", preply_msg->most_user_sub_roomid);
	cJSON_AddItemToObject(proot, "least_user_sub_roomid", pleast_user = cJSON_CreateArray());

	for (it = preply_msg->least_user_sub_roomid.begin(); it != preply_msg->least_user_sub_roomid.end(); ++it)
	{
		cJSON_AddItemToArray(pleast_user, cJSON_CreateNumber(*it));
	}	
	get_string(proot, result_msg);
	cJSON_Delete(proot);
}

static struct xy_big_room_t * new_xy_big_room(std::string &big_roomid)
{
	struct xy_big_room_t *pbig_room = NULL;

	pbig_room = new struct xy_big_room_t;
	assert(pbig_room != NULL);
	pbig_room->big_roomid = big_roomid;
	return pbig_room;
}

static struct xy_sub_room_t * new_xy_sub_room(struct xy_big_room_t *pbig_room)
{
	struct xy_sub_room_t *psub_room = NULL;
	int sub_room_size = pbig_room->sub_roomid_map.size();

	psub_room = new struct xy_sub_room_t;
	assert(psub_room != NULL);
	psub_room->big_roomid = pbig_room->big_roomid;
	psub_room->sub_roomid = sub_room_size; /* ·¿¼äºÅ´Ó0¿ªÊ¼ */
	psub_room->user_num = 1;

	pbig_room->sub_roomid_map.insert(std::make_pair(psub_room->sub_roomid, psub_room));

	return psub_room;
}

static void delete_xy_big_room(struct xy_big_room_t *pbig_room)
{
	assert(pbig_room != NULL);

	s_all_big_room.erase(pbig_room->big_roomid);
	delete(pbig_room);
}

static void delete_xy_sub_room(struct xy_big_room_t *pbig_room, struct xy_sub_room_t *psub_room)
{
	assert(NULL != pbig_room);
	assert(NULL != psub_room);

	pbig_room->sub_roomid_map.erase(psub_room->sub_roomid);
	delete psub_room;
}


static struct xy_big_room_t * find_xy_big_room(std::string &big_roomid)
{
	std::map<std::string, struct xy_big_room_t *>::iterator it;

	it = s_all_big_room.find(big_roomid);
	if (it == s_all_big_room.end())
	{
		return NULL;
	}
	return it->second;
}

static struct xy_sub_room_t * find_sub_room(struct xy_big_room_t *pbig_room, int sub_roomid)
{
	assert(pbig_room != NULL);

	std::map<int, struct xy_sub_room_t *>::iterator it;

	it = pbig_room->sub_roomid_map.find(sub_roomid);
	if (it == pbig_room->sub_roomid_map.end())
	{
		return NULL;
	}
	return it->second;
}

static int find_sub_roomid_from_userid(struct xy_big_room_t *pbig_room, std::string &userid)
{
	assert(pbig_room != NULL);

	std::map<std::string, int>::iterator it;

	it = pbig_room->userid_map_sub_roomid.find(userid);
	if (it == pbig_room->userid_map_sub_roomid.end())
	{
		return -1;
	}
	return it->second;
}

static int find_most_user_and_assign_sub_roomid(struct xy_big_room_t *pbig_room, struct xy_reply_msg_t *preply_msg)
{
	assert(pbig_room != NULL);
	assert(preply_msg != NULL);

	std::map<int, struct xy_sub_room_t *>::iterator it;
	struct xy_sub_room_t *psub_room = NULL;
	int most_sub_room_user_num = 0;

	for (it = pbig_room->sub_roomid_map.begin(); it != pbig_room->sub_roomid_map.end(); ++it)
	{
		psub_room = it->second;
		if (preply_msg->sub_roomid == -1 && psub_room->user_num < s_most_sub_room_user_num)
		{
			preply_msg->sub_roomid = psub_room->sub_roomid;
		}
		if (psub_room->user_num > most_sub_room_user_num)
		{
			most_sub_room_user_num = psub_room->user_num;
			preply_msg->most_user_sub_roomid = psub_room->sub_roomid;
		}
		if (preply_msg->sub_roomid != -1 && most_sub_room_user_num == s_most_sub_room_user_num)
		{
			break;
		}
	}
	return most_sub_room_user_num;
}

static int find_most_user_sub_roomid(struct xy_big_room_t *pbig_room, struct xy_reply_msg_t *preply_msg)
{
	assert(pbig_room != NULL);
	assert(preply_msg != NULL);

	std::map<int, struct xy_sub_room_t *>::iterator it;
	struct xy_sub_room_t *psub_room = NULL;
	int most_sub_room_user_num = 0;

	for (it = pbig_room->sub_roomid_map.begin(); it != pbig_room->sub_roomid_map.end(); ++it)
	{
		psub_room = it->second;
		if (psub_room->user_num > most_sub_room_user_num)
		{
			most_sub_room_user_num = psub_room->user_num;
			preply_msg->most_user_sub_roomid = psub_room->sub_roomid;
		}
		if (most_sub_room_user_num >= s_most_sub_room_user_num)
		{
			break;
		}
	}
	return most_sub_room_user_num;
}

static void find_least_user_sub_roomid(struct xy_big_room_t *pbig_room, struct xy_reply_msg_t *preply_msg, int most_sub_room_user_num)
{
	assert(pbig_room != NULL);
	assert(preply_msg != NULL);

	std::map<int, struct xy_sub_room_t *>::iterator it;
	struct xy_sub_room_t *psub_room = NULL;
	int tmp_num = most_sub_room_user_num * s_least_user_percent / 100;

	for (it = pbig_room->sub_roomid_map.begin(); it != pbig_room->sub_roomid_map.end(); ++it)
	{
		psub_room = it->second;
		if (psub_room->user_num <= tmp_num)
		{
			preply_msg->least_user_sub_roomid.push_back(psub_room->sub_roomid);
		}
	}
}

static int do_process_in_room(struct room_head_t *phead, struct xy_reply_msg_t *preply_msg)
{
	assert(phead != NULL);
	assert(preply_msg != NULL);

	
	struct xy_big_room_t *pbig_room = NULL;
	struct xy_sub_room_t *psub_room = NULL, *pnew_sub_room = NULL;
	int most_sub_room_user_num = 0;

	pbig_room = find_xy_big_room(phead->big_roomid);
	if (NULL == pbig_room)
	{	/* µÚÒ»¸öuser */
		pbig_room = new_xy_big_room(phead->big_roomid);
		psub_room = new_xy_sub_room(pbig_room);

		pbig_room->userid_map_sub_roomid.insert(std::make_pair(phead->userid, psub_room->sub_roomid));
		s_all_big_room.insert(std::make_pair(phead->big_roomid, pbig_room));

		preply_msg->sub_roomid = psub_room->sub_roomid;
		preply_msg->most_user_sub_roomid = psub_room->sub_roomid;
	}
	else
	{
		if (-1 != find_sub_roomid_from_userid(pbig_room, phead->userid))
		{	/* 该用户已经进入过房间了 */
			dzlog_notice("users already in the room, userid: %d", phead->userid.c_str());
			return -1;
		}

		preply_msg->sub_roomid = -1;
		most_sub_room_user_num = find_most_user_and_assign_sub_roomid(pbig_room, preply_msg);
		if (preply_msg->sub_roomid == -1)
		{
			/* 当前子房间都已经满了，所以重新new一个子房间 */
			pnew_sub_room = new_xy_sub_room(pbig_room);
			preply_msg->sub_roomid = pnew_sub_room->sub_roomid;
		}
		else
		{
			psub_room = find_sub_room(pbig_room, preply_msg->sub_roomid);
			psub_room->user_num += 1;
		}
		/* 保存userid与子房间号映射 */
		pbig_room->userid_map_sub_roomid.insert(std::make_pair(phead->userid, preply_msg->sub_roomid));
		
		find_least_user_sub_roomid(pbig_room, preply_msg, most_sub_room_user_num);
	}
	preply_msg->reply_cmd = "XY_ROOM_REPLY_IN";
	return 0;
}

static int do_process_out_room(struct room_head_t *phead)
{
	assert(phead != NULL);

	struct xy_big_room_t *pbig_room = NULL;
	struct xy_sub_room_t *psub_room = NULL;
	int sub_roomid = -1;

	pbig_room = find_xy_big_room(phead->big_roomid);
	if (NULL == pbig_room)
	{
		return -1;
	}
	sub_roomid = find_sub_roomid_from_userid(pbig_room, phead->userid);
	if (-1 == sub_roomid)
	{
		return -1;
	}
	pbig_room->userid_map_sub_roomid.erase(phead->userid);

	psub_room = find_sub_room(pbig_room, sub_roomid);
	assert(NULL != psub_room);
	psub_room->user_num -= 1;
	
	if (psub_room->user_num == 0)
	{
		/* 子房间已经没有人了，回收子房间 */
		delete_xy_sub_room(pbig_room, psub_room);
		if (pbig_room->sub_roomid_map.size() == 0)
		{
			/* 大房间已经没有人了，回收大房间 */
			assert(pbig_room->userid_map_sub_roomid.size() == 0);
			delete_xy_big_room(pbig_room);
		}
	}
	return 0;
}

static int do_process_query_room(struct room_head_t *phead, struct xy_reply_msg_t *preply_msg)
{
	assert(phead != NULL);
	assert(preply_msg != NULL);

	struct xy_big_room_t *pbig_room = NULL;
	int sub_roomid = -1;
	int most_sub_room_user_num = 0;

	pbig_room = find_xy_big_room(phead->big_roomid);
	if (NULL == pbig_room)
	{
		return -1;
	}
	sub_roomid = find_sub_roomid_from_userid(pbig_room, phead->userid);
	if (-1 == sub_roomid)
	{
		return -1;
	}
	most_sub_room_user_num = find_most_user_sub_roomid(pbig_room, preply_msg);
	find_least_user_sub_roomid(pbig_room, preply_msg, most_sub_room_user_num);
	preply_msg->sub_roomid = sub_roomid;
	preply_msg->reply_cmd = "XY_ROOM_REPLY_QUERY";
	return 0;
}

static void process_in_room_cb(struct evhttp_request *req, void *arg)
{
	assert(req != NULL);

	struct room_head_t room_head;
	struct xy_reply_msg_t reply_msg;
	std::string result_msg;

	dzlog_debug("start in room");
	if (-1 == http_request_to_room_head(req, &room_head))
	{
		goto ERR;
	}
	if (-1 == do_process_in_room(&room_head, &reply_msg))
	{
		goto ERR;
	}
	generate_reply_msg(&reply_msg, result_msg);	
	return reply(req, result_msg.c_str());
ERR:
	return reply(req, "error");
}

static void process_out_room_cb(struct evhttp_request *req, void *arg)
{
	assert(req != NULL);

	struct room_head_t room_head;

	if (-1 == http_request_to_room_head(req, &room_head))
	{
		goto ERR;
	}
	if (-1 == do_process_out_room(&room_head))
	{
		goto ERR;
	}
	return reply(req, "success");
ERR:
	return reply(req, "error");
}

static void process_query_room_cb(struct evhttp_request *req, void *arg)
{
	assert(req != NULL);

	struct room_head_t room_head;
	std::string result_msg;
	struct xy_reply_msg_t reply_msg;

	if (-1 == http_request_to_room_head(req, &room_head))
	{
		goto ERR;
	}
	if (-1 == do_process_query_room(&room_head, &reply_msg))
	{
		goto ERR;
	}
	generate_reply_msg(&reply_msg, result_msg);
	return reply(req, result_msg.c_str());
ERR:
	return reply(req, "error");
}

static void process_query_all_cb(struct evhttp_request *req, void *arg)
{
	assert(req != NULL);
	print_all_userid();
	return reply(req, "success");
}

static int load_config()
{
	const char *conf_file = "../conf/room_server.json";
	FILE *fp = NULL;
	struct stat stat_buff;
	char *pbuff = NULL;
	cJSON *proot = NULL, *paddr = NULL, *pport = NULL, *psub_room_most_user = NULL, *pleast_user_percent = NULL;

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
	psub_room_most_user = cJSON_GetObjectItem(proot, "sub_room_num");
	if (NULL != psub_room_most_user)
	{
		s_most_sub_room_user_num = psub_room_most_user->valueint;
	}
	pleast_user_percent = cJSON_GetObjectItem(proot, "least_user_percent");
	if (NULL != pleast_user_percent)
	{
		s_least_user_percent = pleast_user_percent->valueint;
	}
	dzlog_notice("addr: %s, port: %d, sub_room_most_user: %d, least_user_percent: %d",
		s_http_addr.c_str(), s_http_port, s_most_sub_room_user_num, s_least_user_percent);

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

static void uninit()
{
	std::map<std::string, struct xy_big_room_t *>::iterator big_room_it;
	std::map<int, struct xy_sub_room_t *>::iterator sub_room_it;
	struct xy_big_room_t *pbig_room = NULL;
	struct xy_sub_room_t *psub_room = NULL;

	for (big_room_it = s_all_big_room.begin(); big_room_it != s_all_big_room.end(); ++big_room_it)
	{
		pbig_room = big_room_it->second;
		for (sub_room_it = pbig_room->sub_roomid_map.begin(); sub_room_it != pbig_room->sub_roomid_map.end(); ++sub_room_it)
		{
			psub_room = sub_room_it->second;
			delete_xy_sub_room(pbig_room, psub_room);
		}
		delete_xy_big_room(pbig_room);
	}
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
	/* 进房间 */
	evhttp_set_cb(http_server, "/in_room", process_in_room_cb, NULL);
	/* 出房间 */
	evhttp_set_cb(http_server, "/out_room", process_out_room_cb, NULL);
	/* 查询房间 */
	evhttp_set_cb(http_server, "/query_room", process_query_room_cb, NULL);
	/* 输出所有房间信息到日志 */
	evhttp_set_cb(http_server, "/query_all", process_query_all_cb, NULL);
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
	uninit();
	dzlog_info("end");

	zlog_fini();
	return 0;

ERR:
	zlog_fini();
	return -1;
}
