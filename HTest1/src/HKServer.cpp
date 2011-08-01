#include "HKNetworking.h"
#include "SimpleIni.h"
#using <mscorlib.dll>

using namespace System;
using namespace System::Collections;
using namespace System::Text;
using namespace System::IO;
using namespace System::Net;
using namespace System::Net::Sockets;
using namespace System::Threading;
using namespace System::Runtime::Remoting;
using namespace System::Runtime::Remoting::Contexts;
using namespace System::Runtime::Remoting::Messaging;
using namespace System::Runtime::InteropServices;
using namespace HKNetworking;

HKServer::HKServer(void)
{
}

HKServer::~HKServer(void)
{
}

void HKServer::Initialize(int TYPE){
	_iType = TYPE;

	_strDestIP = new String("127.0.0.1");

	_sClient = NULL;
	_iRecvPort = 8000;
	_iSendPort = 8000;
	_bNewData = 0;

	m_clientCount = 0;
	_nPackets = 0;
	m_workerSocketList = ArrayList::Synchronized(new ArrayList());

	LoadIni();
}



void HKServer::LoadIni(){
	//Load ini file
	CSimpleIniCaseA ini(true,true,true);
	ini.Reset();
	SI_Error rc = ini.LoadFile("OCConfig.ini");

	if (rc < 0){
		printf("Couldn't find OCConfig.ini\n");
		return;
	}	
	//Get Section Names (Currently not necessary, we only have a 'Configuration' section.)
	CSimpleIniCaseA::TNamesDepend sections;
	ini.GetAllSections(sections);
	
	//Get Configuration keys  
	CSimpleIniCaseA::TNamesDepend keys;
	ini.GetAllKeys("OCNetworking.HKServer", keys);
	
	if(keys.size() <= 0){
		printf("Configuration section not found\n");
		return;
	}
	CSimpleIniCaseA::TNamesDepend::const_iterator i;	//list iterator

	//Loop through keys and get assoc. value.
	for (i = keys.begin(); i != keys.end(); ++i) { 
		const char * pszValue = ini.GetValue("OCNetworking.HKServer",i->pItem, NULL);
		
		//Look for .ini values.  LOOKUP and ELOOKUP macros defined in ParseIni.h
		LOOKUP_MM("RecvPort",_iRecvPort,INTEGER)
		ELOOKUP_MM("SendPort",_iRecvPort,INTEGER)
		ELOOKUP_MM("DestIP",_strDestIP,STRING)
	}
}

void HKServer::StartListen()
{
    try
    {
        
        // Create the listening socket...
		m_mainSocket = new Socket(AddressFamily::InterNetwork,
			SocketType::Stream,
			ProtocolType::Tcp);
		IPEndPoint *ipLocal = new IPEndPoint(IPAddress::Any, _iRecvPort);
        // Bind to local IP Address...
        m_mainSocket->Bind(ipLocal);
        // Start listening...
        m_mainSocket->Listen(4);
        // Create the call back for any client connections...
		m_mainSocket->BeginAccept(new AsyncCallback(this,&HKServer::OnClientConnect), NULL);
    }
    catch (...)
    {
       //MessageBox.Show(se.Message);
    }
}
// This is the call back function, which will be invoked when a client is connected
void HKServer::OnClientConnect(IAsyncResult *asyn)
{
	try
	{
		// Here we complete/end the BeginAccept() asynchronous call
		// by calling EndAccept() - which returns the reference to
		// a new Socket object
		Socket *workerSocket = m_mainSocket->EndAccept (asyn);

		// Now increment the client count for this client 
		// in a thread safe manner
		Interlocked::Increment(&m_clientCount);
		
		// Add the workerSocket reference to our ArrayList
		m_workerSocketList->Add(workerSocket);
//        trackSoftClient->m_workerSocketList.Add(workerSocket);

		// Send a welcome message to client
	//	string msg = "Welcome client " + m_clientCount + "\n";
	//	SendMsgToClient(msg, m_clientCount);

		// Let the worker Socket do the further processing for the 
		// just connected client
		WaitForData(workerSocket, m_clientCount);
			
		// Since the main Socket is now free, it can go back and wait for
		// other clients who are attempting to connect
		m_mainSocket->BeginAccept(new AsyncCallback ( this,&HKServer::OnClientConnect ),NULL);

		printf("Client connected: %d\n",m_clientCount);
	}
	catch(SocketException *se)
	{
		printf("OnClientConnect() Failed - %s\n",se->Message->ToString());
	}
	catch(...){
	}
}
void HKServer::WaitForData(Socket *soc, int clientNumber)
{
	try
	{
		if  ( pfnWorkerCallBack == NULL )
		{		
			// Specify the call back function which is to be 
			// invoked when there is any write activity by the 
			// connected client
			pfnWorkerCallBack = new AsyncCallback (this,&HKServer::OnDataReceived);
		}
	//	String *addr = (__try_cast<IPEndPoint*>(soc->RemoteEndPoint))->Address->ToString();
	//	IPHostEntry *hostInfo = Dns::GetHostByAddress(addr);
       //    listBoxClientList.Items.Add(hostInfo.HostName);

		SocketPacket *theSocPkt = new SocketPacket (soc, clientNumber);

//		theSocPkt->m_clientName = hostInfo->HostName;
		soc->BeginReceive (theSocPkt->dataBuffer, 0, 
			theSocPkt->dataBuffer->Length,
			SocketFlags::None,
			pfnWorkerCallBack,
			theSocPkt);
	}
	catch(SocketException *se)
	{
		printf("WaitForData() Failed - %s\n",se->Message->ToString());
	}
}
void HKServer::Sift(System::String *str){


}
void HKServer::OnDataReceived(IAsyncResult *asyn)
{
	SocketPacket *socketData = __try_cast<SocketPacket*>(asyn->AsyncState);
	try
	{
		//Complete the BeginReceive() asynchronous call by EndReceive() method
		// which will return the number of characters written to the stream 
		// by the client
		int iRx  = socketData->m_currentSocket->EndReceive (asyn);
		String *s = Encoding::ASCII->GetString(socketData->dataBuffer,0,iRx);
		printf("String Received: %s",s->ToString());

		// Send back the reply to the client
//		String *replyMsg = "Message Received:"; 
		// Convert the reply to byte array
//		Byte byData[] = System::Text::Encoding::ASCII->GetBytes(replyMsg);

//		Socket *workerSocket = __try_cast<Socket*>(socketData->m_currentSocket);
//		workerSocket->Send(byData);
/*
        if (szData->ToLower().Contains("utservreq"))
        {
            string[] vals = szData.Split(' ');
            StringBuilder args = new StringBuilder("");

            for (int i = 1; i < vals.Length; i++) args.Append(vals[i] + " ");
            
            string final = args.ToString().TrimEnd(null);
            if (args.ToString().Contains("\n"))final = final.Remove(final.Length - 3);
        
            SendMsgToClients(final);
        }*/
		// Continue the waiting for data on the Socket
		WaitForData(socketData->m_currentSocket, socketData->m_clientNumber );
	}
	catch (SocketException *se){
		if(se->ErrorCode == 10054) // Error code for Connection reset by peer
		{	
			int dec = socketData->m_clientNumber - 1;
			
			// Remove the reference to the worker socket of the closed client
			// so that this object will get garbage collected
			//m_workerSocketList->RemoveAt(dec);
			m_workerSocketList->Item[dec] = NULL;
			printf("OnDataReceived() %d disconnected\n", socketData->m_clientNumber);
        }
		else
		{
			printf("OnDataReceived() Failed - %s\n",se->Message->ToString());
		}
	}
	catch(...){
	}
}
void HKServer::SendMsgToClients(String *msg) {
    try {
		int idx = msg->IndexOf("st");
		String *tmp = Convert::ToString(_nPackets);
		tmp = String::Concat(tmp," ");
		msg= msg->Insert(idx + 3,tmp);

		Byte byData[] = System::Text::Encoding::ASCII->GetBytes(msg);
        Socket *workerSocket = NULL;
        for (int i = 0; i < m_workerSocketList->Count; i++) {
			workerSocket = __try_cast<Socket*>(m_workerSocketList->Item[i]);
            if (workerSocket != NULL) {
                if (workerSocket->Connected) {
                    workerSocket->Send(byData);
                }
            }
        }
		_nPackets++;
	}
    catch (SocketException *se) {
        printf("SendMsgToClients() Failed - %s\n",se->Message->ToString());
    }
}
void HKServer::SendMsgToClient(String *msg, int clientNumber)
{
	// Convert the reply to byte array
	Byte byData[] = System::Text::Encoding::ASCII->GetBytes(msg);
	Socket *workerSocket = __try_cast<Socket*>(m_workerSocketList->Item[clientNumber - 1]);
	workerSocket->Send(byData);
}
void HKServer::CloseSockets()
{
	if(m_mainSocket != NULL)
	{
		m_mainSocket->Close();
	}
	Socket *workerSocket = NULL;
	for(int i = 0; i < m_workerSocketList->Count; i++)
	{
		workerSocket = __try_cast<Socket*>(m_workerSocketList->Item[i]);
		if(workerSocket != NULL)
		{
			workerSocket->Close();
			workerSocket = NULL;
		}
	}	
}

void HKServer::SocketSetup(){
	_sClient = __gc new System::Net::Sockets::UdpClient();
	_sClient->Connect(System::Net::IPAddress::Broadcast/*_strDestIP*/,_iSendPort);
}
void HKServer::SendUdpMessage(String *PacketText){
	System::Byte SendBytes[];
	try 
	{
		//Convert PacketText to byte string for transmission
		SendBytes = System::Text::Encoding::ASCII->GetBytes(PacketText);
		_sClient->Send(SendBytes, SendBytes->Length);
		//log_errors("Sending: %s\n",PacketText);
	}  
	catch (System::ArgumentNullException* e)
	{
		log_errors("HKServer::SendUdpMessage() Error: %s\n", e->ToString());
	}
	catch (System::Net::Sockets::SocketException* e)
	{
		log_errors("HKServer::SendUdpMessage() Error: %s\n", e->ToString());
	}
}