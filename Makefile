all: httpbulk

httpbulk:
	clang++ -L/usr/local/lib/ -I/opt/halon/include/ -I/usr/local/include/ -lcurl -ljlog -fPIC -shared httpbulk.cpp -o httpbulk.so
