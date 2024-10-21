#include<iostream>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<tchar.h>
#include<thread>
#include<string>

using namespace std;

#pragma comment(lib,"ws2_32.lib")

/*

  //initialize winsock
  //create socket
  //connect to the server
  //send/rcv
  //close the socket

*/

bool Initialize() {
	WSADATA data;
	return WSAStartup(MAKEWORD(2, 2), &data) == 0;
}

void SendMsg(SOCKET s, const string& name) {
	string message;

	while (1) {
		getline(cin, message);
		string msg = name + ":" + message;
		int bytesent = send(s, msg.c_str(), msg.length(), 0);
		if (bytesent == SOCKET_ERROR) {
			cout << "message sending failed" << endl;
			break;
		}
		if (message == "quit") {
			cout << "quiting application" << endl;
			break;
		}
	}
	closesocket(s);
	WSACleanup();
}

void ReceiveMsg(SOCKET s) {
	char buffer[4096];
	int rcvLength;
	string receivedMsg = "";
	while (1) {
		rcvLength = recv(s, buffer, sizeof(buffer), 0);
		if (rcvLength <= 0) {
			cout << "disconnected from the server" << endl;
			break;
		}
		else {
			receivedMsg = string(buffer, rcvLength);
			cout << receivedMsg << endl;

		}
	}
	closesocket(s);
	WSACleanup();

}

int main() {
	if (!Initialize()) {
		cout << "winsock initialization failed" << endl;
		return 1;
	}
	cout << "client program" << endl;

	// creating socket
	SOCKET s;
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET) {
		cout << "invalid socket created" << endl;
		return 1;
	}

	// socket address structure
	// defines where (i.e., to which server IP and port) the client will connect
	int port = 8080;

	// 127.0.0.1 is the loopback address or localhost, 
	// which means the client will connect to a server running on the same machine
	sockaddr_in serveradd;
	serveradd.sin_family = AF_INET;
	serveradd.sin_port = htons(port);
	InetPton(AF_INET, _T("127.0.0.1"), &(serveradd.sin_addr));

	// request a connection to the server
	if (connect(s, reinterpret_cast<sockaddr*>(&serveradd), sizeof(serveradd)) == SOCKET_ERROR) {
		cout << "connection with server failed" << endl;
		closesocket(s);
		WSACleanup();
		return 1;
	}
	cout << "connection with server established" << endl;

	// Send the client's name to the server
	cout << "enter your chat name: " << endl;
	string name;
	getline(cin, name);
	send(s, name.c_str(), name.length(), 0);  

	thread senderThread(SendMsg, s, name);
	thread recieverThread(ReceiveMsg, s);

	senderThread.join();
	recieverThread.join();

	return 0;
}