all: httpbulk

httpbulk:
	g++ -L/usr/local/lib/ -I/opt/halon/include/ -I/usr/local/include/ -fPIC -shared httpbulk.cpp -lcurl -ljlog -o httpbulk.so
