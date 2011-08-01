#pragma once

#include <stdio.h>
#include <windows.h>
#include "util.h"

#define _TIMER_USE_MACROS
#include "ProfTimer.h"

#include <vcclr.h>
#using <mscorlib.dll>
#using <System.dll>

#define uint unsigned char
#define NULL 0

namespace HKNetworking {
	enum OCNETTYPES {OCNET_SERVER,OCNET_CLIENT};
	enum SOCKETTYPES {SOCKET_TYPE_ASYNC,SOCKET_TYPE_SYNC};

	//Our server class
	__gc class HKServer
	{
	public:
		HKServer(void);
		~HKServer(void);

		void CloseSockets();
		void Initialize(int TYPE);
		void LoadIni();

		void SendMsgToClient(System::String *msg, int clientNumber);
		void SendMsgToClients(System::String *msg);
		void SendUdpMessage(System::String *msg);

		void Sift(System::String *str);
		void SocketSetup();
		void StartListen();
	protected:
		void OnClientConnect(System::IAsyncResult *asyn);
		void OnDataReceived(System::IAsyncResult *asyn);
		void WaitForData(System::Net::Sockets::Socket *soc, int clientNumber);
	private:

		int _iType;
		int _iSocketType;
		int _iSendPort;
		int _iRecvPort;
		int _nPackets;

		System::String *_strDestIP;
		System::AsyncCallback *pfnWorkerCallBack;
 		System::Net::Sockets::Socket *m_mainSocket;
		System::Threading::Thread *utThread;
		System::Collections::ArrayList *m_workerSocketList;// =ArrayList.Synchronized(new System.Collections.ArrayList());
		System::Net::Sockets::UdpClient* _sClient;
		static int m_clientCount;// = 0;
		static uint _bNewData;

		ProfTimer _timer;
	};
	//Our client class
	__gc class HKClient
	{
	public:
		HKClient(void);
		~HKClient(void);

		void Initialize(int TYPE);
		void LoadIni();
		const char *GetNewData();
		bool GetPosition(float *ar);
		//void FlushStack();

		//void Sift(System::String *str);
		//int OnStartClient();
		//void SendMessageToServer(System::String *msg);
		void SocketSetup(int iSocketType);
		void thSyncSocket();
		double timeSinceLastPacket();

	protected:
	
		//void OnDataReceived(System::IAsyncResult *asyn);
		//void WaitForData();
	
	private:
		System::Byte m_dataBuffer[];// = new byte [10];
		System::IAsyncResult *m_result;
        System::AsyncCallback *m_pfnCallBack;
		System::Net::Sockets::Socket *m_clientSocket;
		System::Byte dgram[];
		System::Collections::Generic::Queue<System::String*> *m_qWorldState;

        uint bExit;
		
		int _iType;
		int _iSocketType;
		int _iSendPort;
		int _iRecvPort;
		int _iLostPackets; 

		UINT bRecvd;
		UINT bInitd;
		UINT bStop;
		UINT bPause;
		UINT bSocketStarted;
		UINT isValid;

		ProfTimer *_timer;
		TIMER_CLASS_INIT(_PacketTime);
		double _dTimeSinceLastPk1,_dTimeSinceLastPk2;
		
		System::String *_strDestIP;
		System::Threading::ThreadStart *_hThreadHandleSt;
		System::Threading::Thread *_hThreadHandle;
		System::Net::Sockets::Socket *serverSocket;
		System::Net::EndPoint* ep;

		static int _bNewData;

		char *sUnmanagedData;
  	};

	//Packet information
	__gc struct SocketPacket
	{
	public:
		SocketPacket(){
			dataBuffer = new System::Byte[1024];
		}
		// Constructor which takes a Socket and a client number
		SocketPacket(System::Net::Sockets::Socket *socket, int clientNumber)
		{
			m_currentSocket = socket;
			m_clientNumber  = clientNumber;
			dataBuffer = new System::Byte[300];
		}
		System::Net::Sockets::Socket *m_currentSocket;
		int m_clientNumber;
		System::String *m_clientName;
		// Buffer to store the data sent by the client
		System::Byte dataBuffer[];		
	};
};