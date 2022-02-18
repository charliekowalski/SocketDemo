#include "RoboCatPCH.h"
#include <iostream>
#include <thread>
#include <mutex>

// Problem: Game Loop
//
// updateInput(); (make sure to not block here!)
//		conn->Receive(); !!! This blocks !!!
//			Two solutions:
//				- Non-Blocking Mode
//					On Receive(), check for -10035; that means "nothings wrong, but I didn't receive any data either"
// update();
// render();
// goto beginning;

string username;
void Formating()
{
	std::cout << "\n\n\n\n\n\n\n\n\n\n\n\n--------------------Welcome!--------------------\n\n\n\n\n";
}

void SetUsername(TCPSocketPtr socket)
{
	std::cout << "Please enter your username: ";
	std::string tempName;
	std::getline(std::cin, tempName);

	std::cout << "Your username is now " << tempName << std::endl;
	socket->Send(tempName.c_str(), tempName.length());

	char buffer[4096];
	int32_t bytesReceived = socket->Receive(buffer, 4096);

	if (bytesReceived <= 0)
	{
		SocketUtil::ReportError("Receiving");
		return;
	}

	std::string receivedMsg(buffer, bytesReceived);
	username = receivedMsg;
}

void DoTcpServer()
{
	// Create socket
	TCPSocketPtr listenSocket = SocketUtil::CreateTCPSocket(SocketAddressFamily::INET);
	if (listenSocket == nullptr)
	{
		SocketUtil::ReportError("Creating listening socket");
		ExitProcess(1);
	}

	//listenSocket->SetNonBlockingMode(true);

	LOG("%s", "Listening socket created");

	// Bind() - "Bind" socket -> tells OS we want to use a specific address

	SocketAddressPtr listenAddress = SocketAddressFactory::CreateIPv4FromString("0.0.0.0:8080");
	if (listenAddress == nullptr)
	{
		SocketUtil::ReportError("Creating listen address");
		ExitProcess(1);
	}

	if (listenSocket->Bind(*listenAddress) != NO_ERROR)
	{
		SocketUtil::ReportError("Binding listening socket");
		// This doesn't block!
		ExitProcess(1);
	}

	LOG("%s", "Bound listening socket");

	// Blocking function call -> Waits for some input; halts the program until something "interesting" happens
	// Non-Blocking function call -> Returns right away, as soon as the action is completed

	// Listen() - Listen on socket -> Non-blocking; tells OS we care about incoming connections on this socket
	if (listenSocket->Listen() != NO_ERROR)
	{
		SocketUtil::ReportError("Listening on listening socket");
		ExitProcess(1);
	}

	LOG("%s", "Listening on socket");

	// Accept() - Accept on socket -> Blocking; Waits for incoming connection and completes TCP handshake

	LOG("%s", "Waiting to accept connections...");
	SocketAddress incomingAddress;
	TCPSocketPtr connSocket = listenSocket->Accept(incomingAddress);
	while (connSocket == nullptr)
	{
		connSocket = listenSocket->Accept(incomingAddress);
		// SocketUtil::ReportError("Accepting connection");
		// ExitProcess(1);
	}

	LOG("Accepted connection from %s", incomingAddress.ToString().c_str());

	Formating();
	SetUsername(connSocket);

	    bool sQuit = false;
	    std::thread sendThreadServer([&]()
		{
			while (!sQuit)
			{
				std::string msg;
				std::getline(std::cin, msg);

				if (msg == "/exit")
				{
					sQuit = true;
					connSocket->~TCPSocket();
					break;
				}
				else
				{
					connSocket->Send(msg.c_str(), msg.length());
				}

			}
		});

		bool rQuit = false;
		std::thread receiveThreadServer([&]() { // don't use [&] :)
		while (!rQuit) // Need to add a quit here to have it really exit!
		{
			char buffer[4096];
			int32_t bytesReceived = connSocket->Receive(buffer, 4096);
			if (bytesReceived <= 0)
			{
				//SocketUtil::ReportError("Receiving");
				std::cout << "Disconnected";
				return;
			}

			std::string receivedMsg(buffer, bytesReceived);
			std::cout << "Received message from " << username << "(" << incomingAddress.ToString() << "): " << receivedMsg << std::endl;
		}
		});

	//std::cout << "Press enter to exit at any time!\n";
	//std::cin.get();
	//rQuit = true;
	//connSocket->~TCPSocket(); // Forcibly close socket (shouldn't call destructors like this -- make a new function for it!
	receiveThreadServer.join();
	sendThreadServer.join();
}

void DoTcpClient(std::string port)
{
	// Create socket
	TCPSocketPtr clientSocket = SocketUtil::CreateTCPSocket(SocketAddressFamily::INET);
	if (clientSocket == nullptr)
	{
		SocketUtil::ReportError("Creating client socket");
		ExitProcess(1);
	}

	LOG("%s", "Client socket created");

	// Bind() - "Bind" socket -> tells OS we want to use a specific address
	
	std::string address = StringUtils::Sprintf("127.0.0.1:%s", port.c_str());
	SocketAddressPtr clientAddress = SocketAddressFactory::CreateIPv4FromString(address.c_str());
	if (clientAddress == nullptr)
	{
		SocketUtil::ReportError("Creating client address");
		ExitProcess(1);
	}

	if (clientSocket->Bind(*clientAddress) != NO_ERROR)
	{
		SocketUtil::ReportError("Binding client socket");
		// This doesn't block!
		ExitProcess(1);
	}

	LOG("%s", "Bound client socket");

	// Connect() -> Connect socket to remote host

	SocketAddressPtr servAddress = SocketAddressFactory::CreateIPv4FromString("127.0.0.1:8080");
	if (servAddress == nullptr)
	{
		SocketUtil::ReportError("Creating server address");
		ExitProcess(1);
	}

	if (clientSocket->Connect(*servAddress) != NO_ERROR)
	{
		SocketUtil::ReportError("Connecting to server");
		ExitProcess(1);
	}

	LOG("%s", "Connected to server!");

	Formating();

	SetUsername(clientSocket);

	bool sQuit = false;
	std::thread sendThreadClient([&]()
		{
			while (!sQuit)
			{
				std::string msg;
				std::getline(std::cin, msg);

				if (msg == "/exit")
				{
					sQuit = true;
					clientSocket->~TCPSocket();

					break;
				}
				else
				{
					clientSocket->Send(msg.c_str(), msg.length());
				}
			}
		});

	bool rQuit = false;
	std::thread receiveThreadClient([&]() { // don't use [&] :)
		while (!rQuit) // Need to add a quit here to have it really exit!
		{
			char buffer[4096];
			int32_t bytesReceived = clientSocket->Receive(buffer, 4096);
			if (bytesReceived <= 0)
			{
				//SocketUtil::ReportError("Receiving");
				std::cout << "Disconnected";
				return;
			}

			std::string receivedMsg(buffer, bytesReceived);
			std::cout << "Received message from " << username << "(" << servAddress->ToString() <<") : " << receivedMsg << std::endl;
		}
		});

	receiveThreadClient.join();
	sendThreadClient.join();

	//
	/*while (true)
	{
		std::string msg("Hello server! How are you?");
		clientSocket->Send(msg.c_str(), msg.length());
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}*/
}

std::mutex coutMutex;

void DoCout(std::string msg)
{
	for (int i = 0; i < 5; i++)
	{
		coutMutex.lock();  // can block!
		std::cout << msg << std::endl;
		coutMutex.unlock();
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	std::cout << "Exiting loop gracefully\n";
}

bool gQuit;

void DoCoutLoop(std::string msg)
{
	while (!gQuit)
	{
		coutMutex.lock();  // can block!
		std::cout << msg << std::endl;
		coutMutex.unlock();
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	std::cout << "Exiting loop gracfully\n";
}

void DoCoutLoopLocal(std::string msg, const bool& quit)
{
	while (!quit)
	{
		coutMutex.lock();  // can block!
		std::cout << msg << std::endl;
		coutMutex.unlock();
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	std::cout << "Exiting loop gracfully\n";
}

void DoCoutAndExit(std::string msg)
{
	std::cout << msg << std::endl;

	std::cout << "Exiting 'loop' gracefully\n";
}

void DoThreadExample()
{
	// Thread Example

	// Ex. 1: Two cout's at once

	//DoCout();
	gQuit = false;
	bool quit = false;

	// Lambdas = anonymous functions = Functions with no name.
	//		max(5, 7) <- two 'anonymous' ints
	//			int five = 5, seven = 7; max(five, seven);
	//
	//	Lambda syntax: [](args) {body} <- a lambda!
	//		[] -> captures (can use variables from outside scope of function

	//  TcpSocketPtr s;
	//	std::thread receiveThread([&s]() {
	//			s->Receive(...);
	//		});
	//
	//  ReceiveOnSocket() {
	//		s->Receive		// Not global! What are we referencing here?
	//	}

	std::thread t1(DoCoutLoopLocal, "Hello from thread 1!", std::ref(quit));
	std::thread t2(DoCoutLoopLocal, "Thread 2 reporting in!", std::ref(quit));
	std::thread t3([&quit](std::string msg)
		{
			while (!quit)
			{
				std::cout << msg << std::endl;

				std::cout << "Exiting 'loop' gracefully\n";
			}
		}, "Thread 3 here!");

	std::cout << "Hello from the main thread!\n";

	std::cout << "Press enter to exit at any time.\n\n";
	std::cin.get();

	gQuit = true;
	quit = true;

	t1.join();
	t2.join();
	t3.join();
}

#if _WIN32
int main(int argc, const char** argv)
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
#else
const char** __argv;
int __argc;
int main(int argc, const char** argv)
{
	__argc = argc;
	__argv = argv;
#endif

	// WinSock2.h
	//    https://docs.microsoft.com/en-us/windows/win32/api/winsock/


	SocketUtil::StaticInit();

	//DoThreadExample();


	bool isServer = StringUtils::GetCommandLineArg(1) == "server";

	if (isServer)
	{
		// Server code ----------------
		//		Want P2P -- we'll get to that :)
		DoTcpServer();
	}
	else
	{
		// Client code ----------------
		DoTcpClient(StringUtils::GetCommandLineArg(2));
	}


	SocketUtil::CleanUp();

	return 0;
}
