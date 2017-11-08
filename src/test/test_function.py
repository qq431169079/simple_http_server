#!/usr/bin/env python
# coding=utf-8

import json
import time
import httplib, urllib
    
#""" 功能测试
#进房间
print "第一次进房间，3个大房间，每个31人"
for i in range(0, 3):
    for j in range(0, 31):
        headers = {"Content-type":"application/json"}
        conn = httplib.HTTPConnection("192.168.116.128:7788")  
        roomid = str(i)
        userid = str(j)
        tmp1 = { "cmd": "XY_ROOM_REQUEST_IN", "roomid": "roomid" + roomid, "userid": userid }
        #print json.dumps(tmp1)
        conn.request("POST", "http://192.168.116.128:7788/in_room", body=json.dumps(tmp1), headers=headers)
        response = conn.getresponse()  
        print response.status, response.reason
        print response.read()

#roomid0号大出房间出3个人
print "0号大房间0号子房间出3个人"
for j in range(0, 3):
    headers = {"Content-type":"application/json"}
    conn = httplib.HTTPConnection("192.168.116.128:7788")
    userid = str(j)
    tmp1 = { "cmd": "XY_ROOM_REQUEST_IN", "roomid": "roomid0", "userid": userid }
    #print json.dumps(tmp1)
    conn.request("POST", "http://192.168.116.128:7788/out_room", body=json.dumps(tmp1), headers=headers)
    response = conn.getresponse()  
    print response.status, response.reason
    print response.read()

#roomid0号大出房间出3个人，会报错
print "0号大房间0号子房间再出3个人，会报错"
for j in range(0, 3):
    headers = {"Content-type":"application/json"}
    conn = httplib.HTTPConnection("192.168.116.128:7788")
    userid = str(j)
    tmp1 = { "cmd": "XY_ROOM_REQUEST_IN", "roomid": "roomid0", "userid": userid }
    #print json.dumps(tmp1)
    conn.request("POST", "http://192.168.116.128:7788/out_room", body=json.dumps(tmp1), headers=headers)
    response = conn.getresponse()  
    print response.status, response.reason
    print response.read()   

#time.sleep(5)

print "0号大房间0号子房间出4个人"
#roomid0号大出房间出4个人
for j in range(3, 8):
    headers = {"Content-type":"application/json"}
    conn = httplib.HTTPConnection("192.168.116.128:7788")
    userid = str(j)
    tmp1 = { "cmd": "XY_ROOM_REQUEST_IN", "roomid": "roomid0", "userid": userid }
    #print json.dumps(tmp1)
    conn.request("POST", "http://192.168.116.128:7788/out_room", body=json.dumps(tmp1), headers=headers)
    response = conn.getresponse()  
    print response.status, response.reason
    print response.read() 

#roomid0号大房间在进人
print "0号大房间0号子房间出1个人"
for j in range(0, 1):
    headers = {"Content-type":"application/json"}
    conn = httplib.HTTPConnection("192.168.116.128:7788")  
    userid = str(j)
    tmp1 = { "cmd": "XY_ROOM_REQUEST_IN", "roomid": "roomid0", "userid": userid }
    #print json.dumps(tmp1)
    conn.request("POST", "http://192.168.116.128:7788/in_room", body=json.dumps(tmp1), headers=headers)
    response = conn.getresponse()  
    print response.status, response.reason
    print response.read()   

#查roomid0号大房间userid为0的人
print "查0号大房间"
for j in range(0, 1):
    headers = {"Content-type":"application/json"}
    conn = httplib.HTTPConnection("192.168.116.128:7788") 
    userid = str(j)
    tmp1 = { "cmd": "XY_ROOM_REQUEST_IN", "roomid": "roomid0", "userid": userid }
    #print json.dumps(tmp1)
    conn.request("POST", "http://192.168.116.128:7788/query_room", body=json.dumps(tmp1), headers=headers)
    response = conn.getresponse()  
    print response.status, response.reason

#查roomid1号大房间userid为0的人
#print "查1号大房间"
for j in range(10, 15):
    headers = {"Content-type":"application/json"}
    conn = httplib.HTTPConnection("192.168.116.128:7788") 
    userid = str(j)
    tmp1 = { "cmd": "XY_ROOM_REQUEST_IN", "roomid": "roomid1", "userid": userid }
    #print json.dumps(tmp1)
    conn.request("POST", "http://192.168.116.128:7788/query_room", body=json.dumps(tmp1), headers=headers)
    response = conn.getresponse()  
    print response.status, response.reason
    print response.read()   
#"""

