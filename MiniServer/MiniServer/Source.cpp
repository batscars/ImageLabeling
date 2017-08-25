#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <string>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

static HANDLE m_mutex = INVALID_HANDLE_VALUE;

DWORD WINAPI AnswerThread(LPVOID  lparam)
{
	SOCKET ClientSocket = (SOCKET)(LPVOID)lparam;
	int bytesRecv;
	while (1)
	{
		bytesRecv = SOCKET_ERROR;
		char sendbuff[3] = "ok";
		char recvbuf[20] = "";
		for (int i = 0; i<(int)strlen(recvbuf); i++)
		{
			recvbuf[i] = '\0';
		}
		while (bytesRecv == SOCKET_ERROR)
		{
			bytesRecv = recv(ClientSocket, recvbuf, sizeof(recvbuf), 0);
		}
		string recved = recvbuf;
		if (recved == "op_begin")
		{
			WaitForSingleObject(m_mutex, INFINITE);
			cout << "op_begin" << endl;
			send(ClientSocket, sendbuff, sizeof(sendbuff), 0);
		}
		if (recved == "op_end")
		{
			cout << "op_end" << endl;
			ReleaseMutex(&m_mutex);
			closesocket(ClientSocket);	
			return 0;
		}
	}
	return  0;
}
int main()
{
	//initialize Winsock  
	WSADATA  wsaData;
	int  iRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iRet != NO_ERROR)
		printf("Error at WSAStartup()\n");

	//create a socket  
	SOCKET  m_socket;
	m_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_socket == INVALID_SOCKET)
	{
		printf("Error at socket():%ld\n", WSAGetLastError());
		WSACleanup();
		return  0;
	}

	//bind a socket  
	SOCKADDR_IN  service;
	service.sin_family = AF_INET;
	service.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	service.sin_port = htons(2501);

	if (bind(m_socket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR)
	{
		printf("bind() failed.\n");
		closesocket(m_socket);
		return  0;
	}
	else
		printf("bind ok.\n");

	//listen  on  a  socket  
	if (listen(m_socket, 20) == SOCKET_ERROR)
		printf("Error listening on socket.\n");
	else
		printf("listening ok.\n");

	//accept  a  connection  
	SOCKET  AcceptSocket;
	printf("waiting for a client to connect...\n");
	m_mutex = CreateMutex(NULL, FALSE, L"Mutex");
	if (!m_mutex)
	{
		cout << "Failed to CreateMutex !" << endl;
		return 0;
	}
	int count = 0;
	while (1)
	{
		AcceptSocket = SOCKET_ERROR;
		while (AcceptSocket == SOCKET_ERROR)
		{
			AcceptSocket = accept(m_socket, NULL, NULL);
		}
		count++;
		printf("client num %d connected.\n", count);


		DWORD  dwThreadId;
		HANDLE  hThread;

		hThread = CreateThread(NULL, NULL, AnswerThread, (LPVOID)AcceptSocket, 0, &dwThreadId);
		if (hThread == NULL)
		{
			printf("CreatThread AnswerThread() failed.\n");
		}
		else
		{
			printf("create thread %d ok.\n", count);
		}
		CloseHandle(hThread);
	}
	closesocket(m_socket);
	WSACleanup();
}