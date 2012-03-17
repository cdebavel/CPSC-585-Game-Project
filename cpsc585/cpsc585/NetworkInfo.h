#pragma once

//#include <WinSock.h>
//#include <inaddr.h>
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <ws2tcpip.h>

const int MAXCLIENTS = 5;
const int NETWORK_PORT = 5869;

//Client headers
const char BUTTON = 'B';
const char READY = 'R';
const char UNREADY = 'U';
const char COLOR = 'C';

//Server headers
const char TRACK = 'T';
const char START = 'S';
const char END = 'E';
const char CLIENTINFO = 'L';
const char WORLDSTATE = 'W';
const char ID = 'I';

struct ClientInfo
{
	int id;
	int color;
	SOCKET sock;
	bool ready;
	SOCKADDR_IN addr;
	addrinfo * result;
};