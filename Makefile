all: http-bulk

http-bulk:
	g++ -L/usr/local/lib/ -I/opt/halon/include/ -I/usr/local/include/ -fPIC -shared http-bulk.cpp -lcurl -ljlog -o http-bulk.so
