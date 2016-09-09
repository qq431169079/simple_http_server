#ifndef _ROOM_SERVER_H_
#define _ROOM_SERVER_H_


#include <string>
#include <vector>
#include <map>
#include <stdint.h>

/** 存储子房间信息 */
struct xy_sub_room_t
{
    std::string     big_roomid;     /**< 大房间id */
    int             sub_roomid;     /**< 子房间id */
    int             user_num;       /**< 子房间人数 */
};

/** 存储大房间信息 */
struct xy_big_room_t
{
	std::string								big_roomid;				/**< 大房间号 */
    std::map<int, struct xy_sub_room_t *>	sub_roomid_map;			/**< 子房间号映射子房间信息 */
    std::map<std::string, int>				userid_map_sub_roomid;  /**< userid映射到子房间号 */
};

struct xy_reply_msg_t
{
	std::string				        reply_cmd;
	int                             sub_roomid;				/**< 分配的子房间号 */
    int								most_user_sub_roomid;   /**< 子房子人数最多的子房间号 */
    std::vector<int>				least_user_sub_roomid;  /**< 人数不符合要求的子房间 */
};

/** 与外部的交互命令 */
enum
{
    XY_ROOM_REQUEST_IN      = 1,  /* 未使用 */
    XY_ROOM_REPLY_IN,
    XY_ROOM_REQUEST_OUT,	/* 未使用 */
    XY_ROOM_REPLY_OUT,		/* 未使用 */
    XY_ROOM_REQUEST_QUERY,	/* 未使用 */
    XY_ROOM_REPLY_QUERY,
};

/** 与biz交互数据包 */
struct room_head_t
{
	std::string     request_cmd;
	std::string     big_roomid;
	std::string     userid;
};


#endif
