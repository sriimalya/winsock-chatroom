#include<iostream>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<tchar.h>
#include<thread>
#include<vector>
#include<mutex>

using namespace std;
#pragma comment(lib,"ws2_32.lib")
mutex mtx;

/*

// Socket Programming

// Initialize winsock library
// Create the socket
// Get the IP and Port(on which the client will connect)
// Bind the IP and Port wuth the Scoket
// Listen on the Socket
// Accept (blocking only in this project)
// Transfer client on thread
// Recieve and Send
// Close the Socket
// Cleanup the winsock

*/

// winsock initialization
bool Initialize() {
	WSADATA data;
	return WSAStartup(MAKEWORD(2, 2), &data) == 0;
}

void InteractWithClient(SOCKET clientSocket, vector<SOCKET>& clients, mutex& mtx) {
	char buffer[4096];
	// receive the client's name
	int namebytesrecvd = recv(clientSocket, buffer, sizeof(buffer), 0);
	if (namebytesrecvd <= 0) {
		cout << "client disconnected" << endl;
		closesocket(clientSocket);
		return;
	}

	string clientName(buffer, namebytesrecvd);
	cout << "client connected: " << clientName << endl;

	while (1) {
		int bytesrecvd = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (bytesrecvd <= 0) {
			cout << clientName << " disconnected" << endl;
			break;
		}
		string message(buffer, bytesrecvd);
		cout << "message from "<< clientName<< " : " << message << endl;

		// Lock the mutex for sending messages to prevent concurrent access
		{
			lock_guard<mutex> lock(mtx);
			//fwd the msg rcvd from this client to rest of the connected client
			for (auto client : clients) {
				//to prevent fwdng the message the same client itself
				if (client != clientSocket) {
					send(client, message.c_str(), message.length(), 0);
				}
			}
		} // Mutex is automatically unlocked when the lock_guard goes out of scope
	}

	// Lock the mutex before modifying the clients vector
	{
		lock_guard<mutex> lock(mtx);
		auto it = find(clients.begin(), clients.end(), clientSocket);
		if (it != clients.end()) {
			clients.erase(it);
		}
	}  // Mutex is automatically unlocked here

	closesocket(clientSocket);
}

int main() {
	if (!Initialize()) {
		cout << "winsock initialization failed" << endl;
		return 1;
	}
	cout << "Server Program" << endl;

	//socket creation
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET) {
		cout << "socket creation failed" << endl;
		return 1;
	}

	// create address structure
	int port = 8080;
	sockaddr_in serveradd;
	serveradd.sin_family = AF_INET;
	serveradd.sin_port = htons(8080); // htons API converts the port number(given integer value) from host byte order (little/big-endian; depending on your machine's architecture) to network byte order (big-endian)

	// Convert string representation of ip add to binary format
	// local host ip(0.0.0.0) conversion to binary format
	// then put it inside the sin_family
	// 0.0.0.0: For accepting connections from any network interface (local, LAN, or external); (Bind to All Interfaces)
	if (InetPton(AF_INET, _T("0.0.0.0"), &serveradd.sin_addr) != 1) {
		cout << "setting address structure failed" << endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	// binding the IP and Port
	if (bind(listenSocket, reinterpret_cast<sockaddr*> (&serveradd), sizeof(serveradd)) == SOCKET_ERROR) {
		cout << "bind failed" << endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	// listen
	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		cout << "listen failed" << endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	cout << "server has started listening on port: " << port << endl;

	vector<SOCKET> clients;

	// to accept client connection requests
	while (1) {
		SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
		if (clientSocket == INVALID_SOCKET) {
			cout << "invalid client socket" << endl;
			continue;
		}
		{
			lock_guard<mutex> lock(mtx);
			clients.push_back(clientSocket);
		}

		// transfer client on a new thread
		thread t1(InteractWithClient, clientSocket, ref(clients), ref(mtx));
		t1.detach();
	}	

	closesocket(listenSocket);
	WSACleanup();
	return 0;
}