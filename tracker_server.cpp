#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <map>

using namespace std;


map<string,int> chunck; //lista global de los usuarios
map<string,int> clientes; //lista global de los usuarios
map<string, int>::iterator it; //iterador en la lista de usuarios
map<string, int>::iterator it2; //iterador en la lista de usuarios para la actualización


//check_if_is_a_live and sent to other peers the set of peers
/*check_if_is_a_live(){


}*/

//Check the size of a string (usuario, mensaje)
string read_size(string text)
{
	char buffer[2];
	sprintf(buffer,"%02d",(unsigned int)text.size());
	return buffer;
}

//translate the socket to nickname
string nick(int client)
{
	string resultado;
	for(it=clientes.begin(); it!=clientes.end(); ++it)
	{
		if(it->second==client)
		{
			resultado = it->first;
		}
	}
	return resultado;	
}

//translate the nickname to socket
int socket(string nickname)
{
	int resultado;
	for(it=clientes.begin(); it!=clientes.end(); ++it)
	{
		if(it->first==nickname)
		{
			resultado = it->second;
		}
	}
	return resultado;
}

//check if there is not another user with the same nickname
bool nick_ok(string nickname)
{
	bool answer = true;
	for(it=clientes.begin(); it!=clientes.end(); ++it)
	{
		if(nickname==it->first)
		{
			answer = false;
		}
	}
	return answer;
}

//Receive instructions from client
void processChatClient_thread(int socketClient)
{
	char buffer[1000];
   	string message;
   	string messages;
 	int size_nickname;
 	int size_message;
 	int socketTo;
 	int n;
 	for(;;)
 	{
 		buffer[0] = 'X';
 		n = read(socketClient, buffer, 1);
 		if(n==0)
 		{
 			continue;
 		}
 		if(buffer[0] == 'L') //Login
 		{
   			read(socketClient, buffer, 2);
			size_nickname = atoi(buffer);
			bzero(buffer,2); //cleaning the buffer of the header-size
			read(socketClient, buffer, size_nickname);
			//check if there is not another user with the same nickname
			if(nick_ok(buffer))
			{
				clientes[buffer] = socketClient; //add user to the map
				write(socketClient,"A10",3); //acknowledgment ok
						
				for(it=clientes.begin(); it!=clientes.end(); ++it)
				{
					if(it->second != socketClient) //Don't send mesage to the sender
					{
						write(it->second, message.c_str(), message.size()); //send message
						//////// send a update list when a peer in comming  /////////////
						write(it->second, "a", 1);
						sprintf(buffer,"%02d",(unsigned int)clientes.size());
						write(it->second, buffer, 2);
						for(it2=clientes.begin(); it2!=clientes.end(); ++it2)
						{
							sprintf(buffer,"%02d",(unsigned int)it2->first.length());
							write(it->second, buffer, 2);
							write(it->second, it2->first.c_str(), it2->first.length());
						}
						//////////////////////////////////////////////////////////////////
					}
				}
			}
			else
			{
				write(socketClient,"E10",3); //error: nickname is already being used
			}
			bzero(buffer,size_nickname); //cleaning the buffer of the header-size
			
		}
		else if(buffer[0] == 'I') //Lista de usuarios
		{
			write(socketClient, "i", 1);
			sprintf(buffer,"%02d",(unsigned int)clientes.size());
			write(socketClient, buffer, 2);//se envia la cantidade clientes en el buffer
			for(it=clientes.begin(); it!=clientes.end(); ++it)
			{
				sprintf(buffer,"%02d",(unsigned int)it->first.length());
				write(socketClient, buffer, 2);
				write(socketClient, it->first.c_str(), it->first.length());
			}
		}
		else if(buffer[0] == 'O') //Logout
		{
			clientes.erase(nick(socketClient));
			write(socketClient,"A30",3);
			for(it=clientes.begin(); it!=clientes.end(); ++it)
				{
					if(it->second != socketClient) //Don't send mesage to the sender
					{
						write(it->second, message.c_str(), message.size()); //send message
						//////// send a update list when a peer in comming  /////////////
						write(it->second, "a", 1);
						sprintf(buffer,"%02d",(unsigned int)clientes.size());
						write(it->second, buffer, 2);
						for(it2=clientes.begin(); it2!=clientes.end(); ++it2)
						{
							sprintf(buffer,"%02d",(unsigned int)it2->first.length());
							write(it->second, buffer, 2);
							write(it->second, it2->first.c_str(), it2->first.length());
						}
						//////////////////////////////////////////////////////////////////
					}
				}
			break; //este thread ya no es necesario
		}
		else if(buffer[0] == 'M') //Mensaje a otro cliente
		{
			message = "W";
			//From
			message = message + read_size(nick(socketClient)) + nick(socketClient);
			//to
			read(socketClient,buffer,2); //reading header with size of from
			size_nickname = atoi(buffer);
			bzero(buffer,2); //cleaning the buffer of the header-size
			read(socketClient,buffer,size_nickname); //read the right size of from
			socketTo = socket(buffer);
			bzero(buffer,size_nickname);
			//message
			read(socketClient,buffer,2); //reading header with size of message
			size_message = atoi(buffer);
			bzero(buffer,2); //cleaning the buffer of the header-size
			read(socketClient,buffer,size_message); //read the right size of mesage
			message = message + read_size(buffer) + buffer;
			bzero(buffer,size_message);
			write(socketTo, message.c_str(), message.size());
			write(socketClient,"A20",3); //acknowledgment
		}
	}
}

int main(void)
{
	struct sockaddr_in stSockAddr;
	int SocketClient = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	char buffer[1000];
	int n;

	if(-1 == SocketClient)
	{
		perror("can not create socket");
		exit(EXIT_FAILURE);
	}

	memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

	stSockAddr.sin_family = AF_INET;
	stSockAddr.sin_port = htons(1104);
	stSockAddr.sin_addr.s_addr = INADDR_ANY;

	if(-1 == bind(SocketClient,(const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in)))
	{
		perror("error bind failed");
		close(SocketClient);
		exit(EXIT_FAILURE);
	}

	if(-1 == listen(SocketClient, 10))
	{
		perror("error listen failed");
		close(SocketClient);
		exit(EXIT_FAILURE);
	}

	
	for(;;)
	{
		int ConnectFD = accept(SocketClient, NULL, NULL);
		if(0 > ConnectFD)
		{
			perror("error accept failed");
			close(SocketClient);
			exit(EXIT_FAILURE);
		}
		else
		{
			thread(processChatClient_thread,ConnectFD).detach();
		}
	}	
		
	close(SocketClient);
	return 0;
}
