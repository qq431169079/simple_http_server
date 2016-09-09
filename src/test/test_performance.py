#!/usr/bin/env python
# coding=utf-8
# 
import threading
from time import ctime,sleep
import time
import json
import time
#import httplib, urllib
import pycurl
from StringIO import StringIO
import multiprocessing

#ip = "10.10.40.107"
ip = "192.168.116.128"
#ip = "123.59.127.126"

def write_data(return_data):
    pass

def in_room(str_start_userid, str_end_userid, roomid):
    start_userid = int(str_start_userid)
    end_userid = int(str_end_userid)

    buffer = StringIO()
    c = pycurl.Curl()
    c.setopt(c.URL, "http://" + ip + ":7788/in_room")
    c.setopt(pycurl.HTTPHEADER, ['Content-type: application/json'])
    c.setopt(pycurl.HTTPHEADER, ['Proxy-Connection: Proxy-Connection'])
    c.setopt(pycurl.WRITEFUNCTION, write_data)

    for x in xrange(start_userid, end_userid):
        userid = str(x)
        tmp1 = { "cmd": "XY_ROOM_REQUEST_IN", "roomid": roomid, "userid": userid }

        c.setopt(c.POSTFIELDS, json.dumps(tmp1))
        c.perform()
        #body = buffer.getvalue()
        #print body
    c.close()
    #print "time: %f, exec in_room() end, start_userid: %d, end_userid: %d" % (time.time(), start_userid, end_userid)



if __name__ == "__main__":
    start_time = time.time()

    process_handles = []
    process_num = 3 
    request_num = 101
    for i in xrange(0, process_num):
        roomid = "roomid" + str(i)
        handle = multiprocessing.Process(target = in_room, args = (u'0', str(request_num), roomid, ))
        process_handles.append(handle)

    for i in range(0, process_num):
        process_handles[i].start()

    for i in range(0, process_num):
        process_handles[i].join()


    end_time = time.time()
    interval_time = float(end_time - start_time)
    all_request_num = (float)(process_num * request_num)
    speed = -1
    if (interval_time != 0.0):
        speed = all_request_num/interval_time

    print "speed: %f, interval_time: %f, process_num:%d, request_num: %d, all_request_num: %d" \
        % (speed, interval_time, process_num, request_num, all_request_num)
