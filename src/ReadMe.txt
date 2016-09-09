对外提供的接口:
    1. 用户进房间
        /in_room
        传入: { "roomid": "roomid", "userid": "userid" }
        失败："error"
        成功：{
                "cmd":  "XY_ROOM_REPLY_IN",
                "sub_roomid":   1,
                "most_user_sub_roomid": 0,
                "least_user_sub_roomid":    [1]
              }

    2. 用户出房间
        /out_room
        传入: { "roomid": "roomid", "userid": "userid" }
        失败："error"
        成功："success"

    3. 查询用户所在房间
        /query_room
        传入: { "roomid": "roomid", "userid": "userid" }
        失败："error"
        成功：{
                "cmd":  "XY_ROOM_REPLY_QUERY",
                "sub_roomid":   1,
                "most_user_sub_roomid": 0,
                "least_user_sub_roomid":    [1]
               }

    4. 热加载配置文件
        /reload_conf
        传入：不需要传入参数
        失败："error"
        成功："success"

    5. 输出程序用户所在房间的信息到日志中
        /query_all
        传入：不需要传入参数
        失败："error"
        成功："success"

    6. 退出程序
        /exit
        传入：不需要传入参数
        失败：不失败
        成功：无返回值

部署：
1. 依赖的第三方库在./room_server/3rdparty中，room_server依赖libevent，zlog库，先安装这两个库
2. 部署完成后设置环境变量
    export LD_LIBRARY_PATH=":/usr/local/lib"
3. 编辑crontable，加入
   */1 * * * * cd /usr/local/sandai/room_server/bin && ./room_server.sh monitor &>/dev/null

性能测试：
    tw04055物理环境，24核心cpu，cpu型号Intel(R) Xeon(R) CPU E5-2620 v3 @ 2.40GHz
    测试程序test_performance.py
    测试程序开30个线程，每个线程发送20000个请求，最后room_server的处理请求在2W/s。此测试结果与cpu的性能有正相关的关系。
    在实际部署环境中可以多部署几个room_server，采用不同的端口号，可以成倍的提高性能。



