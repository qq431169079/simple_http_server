#ifndef _ROOM_SERVER_H_
#define _ROOM_SERVER_H_


#include <string>
#include <vector>
#include <map>
#include <stdint.h>

/** �洢�ӷ�����Ϣ */
struct xy_sub_room_t
{
    std::string     big_roomid;     /**< �󷿼�id */
    int             sub_roomid;     /**< �ӷ���id */
    int             user_num;       /**< �ӷ������� */
};

/** �洢�󷿼���Ϣ */
struct xy_big_room_t
{
	std::string								big_roomid;				/**< �󷿼�� */
    std::map<int, struct xy_sub_room_t *>	sub_roomid_map;			/**< �ӷ����ӳ���ӷ�����Ϣ */
    std::map<std::string, int>				userid_map_sub_roomid;  /**< useridӳ�䵽�ӷ���� */
};

struct xy_reply_msg_t
{
	std::string				        reply_cmd;
	int                             sub_roomid;				/**< ������ӷ���� */
    int								most_user_sub_roomid;   /**< �ӷ������������ӷ���� */
    std::vector<int>				least_user_sub_roomid;  /**< ����������Ҫ����ӷ��� */
};

/** ���ⲿ�Ľ������� */
enum
{
    XY_ROOM_REQUEST_IN      = 1,  /* δʹ�� */
    XY_ROOM_REPLY_IN,
    XY_ROOM_REQUEST_OUT,	/* δʹ�� */
    XY_ROOM_REPLY_OUT,		/* δʹ�� */
    XY_ROOM_REQUEST_QUERY,	/* δʹ�� */
    XY_ROOM_REPLY_QUERY,
};

/** ��biz�������ݰ� */
struct room_head_t
{
	std::string     request_cmd;
	std::string     big_roomid;
	std::string     userid;
};


#endif
