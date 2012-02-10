#include "Server.h"

Server::Server()
{
	wsa_ready = false;
	tcp_ready = false;
	udp_ready = false;

	gameStarted = false;
	track = "Track1";

	numClients = 0;
}

Server::~Server()
{
	closeConnection();
}

/**
 * Listens on the given port for incomming connections
 * From: http://www.codeproject.com/Articles/13071/Programming-Windows-TCP-Sockets-in-C-for-the-Begin
 */

int Server::setupWSA()
{
	if (wsa_ready){
		std::cout << "Error: Attempting to setup Winsock but it is already setup!" << std::endl;
		return 0;
	}

	int error = WSAStartup(0x0202, &w); //Fill in WSA info

	if (error)
	{
		std::cout << "Error: error when calling WSAStartup()" << std::endl;
		wsa_ready = false;
		return error;
	}

	if(w.wVersion != 0x0202) //Wrong winsock version?
	{
		std::cout << "Error: Invalid version of Winsock, need 2.2." << std::endl;
		wsa_ready = false;
		WSACleanup();
		return 0;
	} 
	
	wsa_ready = true;
	std::cout << "WSA successfully setup." << std::endl;
	return 0;
}

int Server::setupUDPSocket()
{
	if (udp_ready)
	{
		std::cout << "Error: Attempting to setup a UDP socket when one is already setup!" << std::endl;
		return 0;
	}

	if (!wsa_ready){
		std::cout << "Error: Attempting to setup a UDP socket when WSA is not ready." << std::endl;
		return 0;
	}

	SOCKADDR_IN addr; //The address struct for TCP socket

	addr.sin_family = AF_INET; //Set address family
	addr.sin_port = htons(NETWORK_PORT); //Assign port to socket
	addr.sin_addr.s_addr = htonl(INADDR_ANY); //Accept connection from any IP
	sUDP = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // Create socket

	if (sUDP == INVALID_SOCKET)
	{
		std::cout << "Error: Failed to create a valid UDP socket." << std::endl;
		udp_ready = false;
		return 0;
	}

	if(bind(sUDP, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		std::cout << "Error: Failed to bind UDP socket." << std::endl;
		udp_ready = false;
		return 0;
	}

	u_long iMode=1; //set FIONBIO arg to 1, which means making these sockets NON-BLOCKING
	if (ioctlsocket(sUDP,FIONBIO,&iMode) != 0){
		std::cout << "Error: Failed to make UDP socket non-blocking." << std::endl;
		udp_ready = false;
		return 0;
	}

	//success
	udp_ready = true;
	std::cout << "UDP socket successfully created." << std::endl;
	return 0; 
}

int Server::setupTCPSocket()
{
	if (tcp_ready)
	{
		std::cout << "Error: Attempting to setup a TCP socket when one is already setup!" << std::endl;
		return 0;
	}

	if (!wsa_ready){
		std::cout << "Error: Attempting to setup a TCP socket when WSA is not ready." << std::endl;
		return 0;
	}

	SOCKADDR_IN addr; //The address struct for TCP socket

	addr.sin_family = AF_INET; //Set address family
	addr.sin_port = htons(NETWORK_PORT); //Assign port to socket

	addr.sin_addr.s_addr = htonl(INADDR_ANY); //Accept connection from any IP
	sTCP = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Create socket

	if (sTCP == INVALID_SOCKET)
	{
		std::cout << "Error: Failed to create a valid TCP socket." << std::endl;
		tcp_ready = false;
		return 0;
	}

	if(bind(sTCP, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		std::cout << "Error: Failed to bind TCP socket." << std::endl;
		tcp_ready = false;
		return 0;
	}

	u_long iMode=1; //set FIONBIO arg to 1, which means making these sockets NON-BLOCKING
	if (ioctlsocket(sTCP,FIONBIO,&iMode) != 0){
		std::cout << "Error: Failed to make TCP socket non-blocking." << std::endl;
		tcp_ready = false;
		return 0;
	}

	//Allows max # of connections as possible, start listening
	if (listen(sTCP, SOMAXCONN) != 0){
		std::cout << "Error: Failed to start listening on TCP socket." << std::endl;
		tcp_ready = false;
		return 0;
	};

	//success
	tcp_ready = true;
	std::cout << "TCP socket successfully created." << std::endl;
	return 0;
}

int Server::lobbyListen()
{
	//tcp for joining clients and for lobby communications
	if (!tcp_ready || !wsa_ready)
	{
		std::cout << "Error: Trying to listen on TCP socket when Winsock or TCP socket is not ready." << std::endl;
		return 0;
	}
	
	int size = sizeof(SOCKADDR);
	clients[numClients].sock = accept(sTCP, (LPSOCKADDR) &clients[numClients].addr, &size); //Accept connection and store data in struct

	//check if there was a connection
	if (clients[numClients].sock != INVALID_SOCKET)
	{
		std::cout << "A client has joined the lobby!" << std::endl;
		clients[numClients].id = numClients;
		clients[numClients].ready = false;
		clients[numClients].color = numClients;
		sendID(numClients);
		numClients++; //Increment the number of clients connected
	}

	getTCPMessages();

	return 0;
}

int Server::raceListen()
{
	//udp for vehicle control and tcp for game control
	if (!udp_ready || !tcp_ready || !wsa_ready)
	{
		std::cout << "Error: Trying to listen on TCP/UDP sockets when Winsock or sockets are not ready." << std::endl;
		return 0;
	}
	getTCPMessages();
	getUDPMessages();

	return 0;
}

/**
 * Closes the socket and any connection on it
 * From: http://www.codeproject.com/Articles/13071/Programming-Windows-TCP-Sockets-in-C-for-the-Begin
 */
void Server::closeConnection()
{
	//Close the socket if it exists
	if(tcp_ready)
	{
		closesocket(sTCP);
	}

	if(udp_ready)
	{
		closesocket(sUDP);
	}

	if(wsa_ready)
	{
		WSACleanup(); //Clean up Winsock
	}
}

/**
 * Sends the given message over TCP to the client with the given ID
 */
int Server::sendTCPMessage(const char* message, int length, int clientID)
{
	return send(clients[clientID].sock,message,length, 0);
}

/**
 * Sends the given message over UDP to the client with the given ID
 */
int Server::sendUDPMessage(std::string message, int clientID)
{
	return sendto(sUDP, message.c_str(), message.length(), 0, (SOCKADDR*) &clients[clientID].addr, sizeof(clients[clientID].addr));
}


/**
 * Server sends to all clients that the track has been changed
 */
int Server::changeTrack(std::string track)
{
	//Get size of message and split into chars
	int size = track.length() + 5;

	char* buffer = new char[size];
	buffer[0] = 'T';
	memcpy(buffer+1,&size,sizeof(int));
	memcpy(buffer+5,track.c_str(),track.length());

	//Send each client a message
	for(int i = 0; i < numClients; i++)
	{
		int result = sendTCPMessage(buffer,size,i); //Send the message
		if(result == -1)
			result = WSAGetLastError();
		std::cout << "Returned " << result << "\n";
	}

	delete[] buffer;

	return true;
}

/**
 * Server lets all clients know the game is starting, and gives them their starting position
 */
int Server::startGame()
{
	gameStarted = true;

	int size = 5;
	char buffer[5];

	buffer[0] = START;
	memcpy(buffer+1,&size,sizeof(int));
	
	//Send each client a message
	for(int i = 0; i < numClients; i++)
	{
		sendTCPMessage(buffer, size, i); //Send the message
	}

	std::cout << "Successfully sent Start Game message to clients." << std::endl;

	return true;
}

/**
 * Server lets all clients know the game is over and gives the final positions
 */
int Server::endGame()
{
	//Set up buffer to send
	int size = 5;
	char buffer[5];
	buffer[0] = 'E';
	memcpy(buffer+1,&size,sizeof(int));

	//Send each client a message
	for(int i = 0; i < numClients; i++)
	{
		sendTCPMessage(buffer,size,i); //Send the message
	}

	std::cout << "Successfully sent End Game message to clients." << std::endl;

	return true;
}

/**
 * Sends client info to all clients
 */
int Server::sendLobbyInfo()
{
	//Set up buffer to send
	int size = sizeof(ClientInfo)*numClients + 5 + sizeof(int);
	char* buffer = new char[size];
	buffer[0] = 'L';
	memcpy(buffer+1,&size,sizeof(int));
	memcpy(buffer+5,&numClients,sizeof(int));

	for(int i = 0; i < numClients; i++)
	{
		memcpy(buffer+9+i*sizeof(ClientInfo),&clients[i],sizeof(ClientInfo)); //Put clientinfo into buffer
	}

	for(int i = 0; i < numClients; i++)
	{
		sendTCPMessage(buffer,size,i);
	}

	delete[] buffer;

	return true;
}

/**
 * Server updates all clients with state of the world
 */
int Server::update(std::string worldState)
{
	//Set up buffer to send
	int size = worldState.length() + 5;
	char* buffer = new char[size];
	buffer[0] = 'W';
	memcpy(buffer+1,&size,sizeof(int));
	memcpy(buffer+5,worldState.c_str(),worldState.length());

	//Send each client a message
	for(int i = 0; i < numClients; i++)
	{
		sendTCPMessage(buffer,size,i); //Send the message
	}

	delete[] buffer;

	return true;
}

/**
 * Server sends the client their ID
 */
int Server::sendID(int id)
{
	//Set up buffer to send
	int size = 9;
	char* buffer = new char[size];
	buffer[0] = 'I';
	memcpy(buffer+1,&size,sizeof(int));
	memcpy(buffer+5,&id,sizeof(int));

	sendTCPMessage(buffer,size,id); //Send the player their ID

	return true;
}

/**
 * Receives any TCP messages in the buffer
 */
void Server::getTCPMessages()
{
	char buff[1000];
	bool changes = false;

	for(int i = 0; i < numClients; i++)
	{
		int err = 0;

		while(err != WSAEWOULDBLOCK)
		{
			//Find out the size of the message
			err = recv(clients[i].sock, buff, 5, MSG_PEEK);
			if(err == -1)
			{
				err = WSAGetLastError();
			}

			if(err != WSAEWOULDBLOCK)
			{
				int size = *((int*)(buff+1)); //Get size

				//Get the message
				err = recv(clients[i].sock, buff, size, 0);
				if(err == -1)
				{
					err = WSAGetLastError();
				}

				//Call the correct function depending on the message
				switch(buff[0])
				{
				case READY: //The client is ready
					std::cout << "Player " << i << " is ready!\n";
					clients[i].ready = true;
					changes = true;
					break;

				case UNREADY: //The client is no longer ready
					clients[i].ready = false;
					changes = true;
					break;

				case COLOR: //The message is the client changing their colour
					int color;
					memcpy(&color,buff+5,sizeof(int)); //Get colour

					//Make sure colour is not in use
					bool used = false;
					for(int j = 0; j < numClients; i++)
					{
						if(clients[j].color == color)
						{
							used = true;
							j = numClients;
						}
					}

					//If the color is not in use, set the player's color to it
					if(!used)
					{
						clients[i].color = color;
						changes = true;
					}
					break;
				}
			}
		}
	}

	//If there are changes, send them
	if(changes)
	{
		sendLobbyInfo();

		//Check if all clients are ready, if they are, send start
		bool allReady = true;
		if(!gameStarted)
		{
			for(int i = 0; i < numClients; i++)
			{
				if(!clients[i].ready)
				{
					std::cout << "All players are ready. Starting game.\n";
					allReady = false;
					i = numClients;
				}
			}
		}

		//If all clients are ready, start game
		if(allReady)
		{
			int pos[MAXCLIENTS];
			for(int i = 0; i < numClients; i++)
			{
				pos[clients[i].id] = i+1;
			}
			startGame();
		}
	}
}

/**
 * Receives any UDP messages in the buffer
 */
void Server::getUDPMessages()
{
	char buff[1000];
	bool changes = false;

	for(int i = 0; i < numClients; i++)
	{
		int err = 0;

		while(err != WSAEWOULDBLOCK)
		{
			//Find out the size of the message
			err = recv(sUDP, buff, 5, MSG_PEEK);
			if(err == -1)
			{
				err = WSAGetLastError();
			}

			if(err != WSAEWOULDBLOCK)
			{
				int size = *((int*)(buff+1)); //Get size

				//Get the message
				err = recv(sUDP, buff, size, 0);
				if(err == -1)
				{
					err = WSAGetLastError();
				}

				//Call the correct function depending on the message
				switch(buff[0])
				{
				case BUTTON: //Message is button press
					int id;
					memcpy(&id,buff+5,sizeof(int)); //Get player's ID
					memcpy(&intents[id],buff+9,sizeof(Intention)); //Get the button presses
					std::cout << "Player " << id << " has the following button state:\n" << intents[id].toStr() << std::endl;
					break;

				default:
					std::cout << "Not button.\n";
					break;
				}
			}
			else
			{
				//std::cout << "Buffer empty.\n";
			}
		}
	}
}
	