#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <map>



using namespace std;

map<string,int> chunck; //lista de chuncks que tiene el peer

//Check the size of a string (usuario, mensaje)
string read_size(string text)
{
	char buffer[2];
	sprintf(buffer,"%02d",(unsigned int)text.size());
	return buffer;
}

//Function for threads that writes a reply for the socket
void write_socket(int my_socket, bool &stop)
{
	string comando;
	string txt;
	string to;
	string message;	
	do{
		cout<<"---------- Welcome Peer ------------"<<endl;
		cout<<"L: Logout: "<<endl;
		cout<<"G: Get_list_of_peer():"<<endl;
		cout<<"D: Download():"<<endl;
		cout<<"GL: Get_list_if_chuncks:"<<endl;		
		cin>>comando;
		//Enviar un mensaje
		if(comando == "msg")
		{
			cout<< "to: ";
			cin>> to;
			cout<< "message: ";
			cin>> message;
			txt = "M" + read_size(to) + to + read_size(message) + message; //We add the header
			write(my_socket,txt.c_str(),txt.length());
		}
		else if(comando=="G") //lista de usuarios
		{
			txt = "I";
			write(my_socket,txt.c_str(),txt.length());
		}
		else if(comando=="L") //Salir
		{
			txt = "O";
			write(my_socket,txt.c_str(),txt.length());
			stop = true;
		}
		else if(comando=="broadcast")
		{
			cout<< "message: ";
			cin>> message;
			txt = "B" + read_size(message) + message; //We add the header
			write(my_socket,txt.c_str(),txt.length());
		}		
	}while( stop==false );
}

//Function for threads that reads The reply of the socket
void read_socket(int my_socket)
{
	int size;
	char buffer[1000];
	string type;
	int cantidad_usuarios;
	int n;
	do{
		n = read(my_socket, buffer ,1); //read just the first byte to check the message's type
		if(n==0)
		{
			continue;
		}
		if (buffer[0]=='W') //Mensaje del server
		{
			bzero(buffer,1); //cleaning the buffer of the header-command
			read(my_socket,buffer,2); //reading header with size of from
			size = atoi(buffer);
			bzero(buffer,2); //cleaning the buffer of the header-size
			read(my_socket,buffer,size); //read the right size of from		
			cout<<"Mensaje de "<< buffer << ": ";
			bzero(buffer,size); //cleaning the buffer of the header-command
			read(my_socket,buffer,2); //reading header with size of message
			size = atoi(buffer);
			bzero(buffer,2); //cleaning the buffer of the header-size
			read(my_socket,buffer,size); //read the right size of message					
			cout<< buffer << endl;
			bzero(buffer,size); //cleaning the buffer of the message
		}
		else if (buffer[0]=='B') //Mensaje del server
		{
			bzero(buffer,1); //cleaning the buffer of the header-command
			read(my_socket,buffer,2); //reading header with size of message
			size = atoi(buffer);
			bzero(buffer,2); //cleaning the buffer of the header-size
			read(my_socket,buffer,size); //read the right size of message					
			cout<< buffer << endl;
			bzero(buffer,size); //cleaning the buffer of the message
		}

		else if (buffer[0]=='i') //lista de usuarios
		{
			bzero(buffer,1); //cleaning the buffer of the header-command
			read(my_socket,buffer,2); //reading header with number of users
			cantidad_usuarios = atoi(buffer);
			bzero(buffer,2); //cleaning the buffer of the header-number of users
			cout<<"Lista de Usuarios:"<<endl;
			for(int i=0; i<cantidad_usuarios; i++)
			{
				read(my_socket,buffer,2); //reading header with size of nickname
				size = atoi(buffer);
				bzero(buffer,2); //cleaning the buffer of the header-size of nickname
				read(my_socket,buffer,size); //reading nickname
				cout<<buffer<<endl; //imprime el nickname
				bzero(buffer,size); //cleaning the buffer of the nickname
			}			
		}

		else if (buffer[0]=='a') //Update_list
		{
			bzero(buffer,1); //cleaning the buffer of the header-command
			read(my_socket,buffer,2); //reading header with number of users
			cantidad_usuarios = atoi(buffer);
			bzero(buffer,2); //cleaning the buffer of the header-number of users
			cout<<"Update-List:"<<endl;
			for(int i=0; i<cantidad_usuarios; i++)
			{
				read(my_socket,buffer,2); //reading header with size of nickname
				size = atoi(buffer);
				bzero(buffer,2); //cleaning the buffer of the header-size of nickname
				read(my_socket,buffer,size); //reading nickname
				cout<<buffer<<endl; //imprime el nickname
				bzero(buffer,size); //cleaning the buffer of the nickname
			}			
		}
		else if (buffer[0]=='A') //acknowledgments
		{
			bzero(buffer,1); //cleaning the buffer of the header-command
			read(my_socket,buffer,2);
			if(strncmp(buffer,"10", 2)==0)
			{
				cout<<"Login is ok"<<endl;
			}
			else if(strncmp(buffer,"20", 2)==0)
			{
				cout<<"Mesage has been sent"<<endl;
			}
			else if(strncmp(buffer,"30", 2)==0)
			{
				cout<<"Logout is ok"<<endl;
			}
			bzero(buffer,2); //cleaning the buffer of the type of acknowledgment
		}
		else if (buffer[0]=='E') //errors
		{
			bzero(buffer,1); //cleaning the buffer of the header-command
			read(my_socket,buffer,2);
			if(strncmp(buffer,"10", 2)==0)
			{
				cout<<"Login ERROR"<<endl;
			}
			else if(strncmp(buffer,"20", 2)==0)
			{
				cout<<"Mesage ERROR"<<endl;
			}
			else if(strncmp(buffer,"30", 2)==0)
			{
				cout<<"Logout ERROR"<<endl;
			}
			bzero(buffer,2); //cleaning the buffer of the type of error

		}
	}while( strncmp(buffer,"A30", 3) != 0 );	
}



int main()
{
	struct sockaddr_in stSockAddr;
	int Res;
	int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	int n;

	if (-1 == SocketFD)
	{
		perror("cannot create socket");
		exit(EXIT_FAILURE);
	}

	memset(&stSockAddr, 0, sizeof(struct sockaddr_in));
	stSockAddr.sin_family = AF_INET;
	stSockAddr.sin_port = htons(1104);
	Res = inet_pton(AF_INET, "0.0.0.0", &stSockAddr.sin_addr);

	if (0 > Res)
	{
		perror("error: first parameter is not a valid address family");
		close(SocketFD);
		exit(EXIT_FAILURE);
	}
	else if (0 == Res)
	{
		perror("char string (second parameter does not contain valid ipaddress");
		close(SocketFD);
		exit(EXIT_FAILURE);
	}

	if (-1 == connect(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in)))
	{
		perror("connect failed");
		close(SocketFD);
		exit(EXIT_FAILURE);
	}

	//LOG IN
	string usuario;
	string size;
	string mensaje;
	char answer_buffer[3];
	bool stop = false; //loop's control
	while(stop==false)
	{
		cout<<"- - - - - WELCOME - - - ";		
		cout<<"Login(IP): ";
		cin>>usuario;
		size = read_size(usuario);
		mensaje = "L" + size + usuario;
		write(SocketFD,mensaje.c_str(),mensaje.length()); //mandamos el mensaje de login al server	
		//comprueba respuesta del log in	
		read(SocketFD, answer_buffer, 3);
		if(strcmp(answer_buffer,"A10")==0)//login ok
		{
			cout<<"Register is OK"<<endl;
			stop = true;
		}
		else //error
		{
			cout<<"Register ERROR"<<endl;
			stop = false;
		}
	}
	

	stop = false;
	//cout<<"WELCOME-PEER : ";		
		
	//This is what I send/write
	thread(write_socket, SocketFD, ref(stop)).detach();		

	//This is what I receive/read
	thread(read_socket, SocketFD).detach();		
 
	//waiting for the end	
	while(stop==false)
	{
	}
	
	shutdown(SocketFD, SHUT_RDWR);
	close(SocketFD);
	return 0;
}
