room_server:clean
	g++ -o room_server -g room_server.cpp cjson/cJSON.c -I/usr/local/include/libxml2/ -levent -lzlog -W -Wall -Werror -Wno-unused-parameter
	cp -rf room_server ../bin/
clean:
	-rm -rf room_server
	-rm -rf *.o
