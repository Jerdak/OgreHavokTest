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

HKClient::HKClient(void)
{
}

HKClient::~HKClient(void)
{
}

void HKClient::Initialize(int TYPE){
	m_dataBuffer = new Byte[10];
	m_result = NULL;
	m_pfnCallBack = NULL;
	m_clientSocket = NULL;
	m_qWorldState = new System::Collections::Generic::Queue<System::String*>();

	_iType = TYPE;
	_strDestIP = new String("127.0.0.1");
	_iSendPort = 8000;
	_bNewData = 0;
	_iLostPackets=0;
	
	bInitd = 1;
	bRecvd = 0;
	bStop = 0;
	bPause = 0;
	bSocketStarted = 0;
	isValid = false;
	_iSocketType = -1;

	_timer = new ProfTimer;
	_timer->Start();
	_timer->Stop();
	_dTimeSinceLastPk1 = _timer->GetDurationInMSecs();

	LoadIni();

	log_errors("Client Destination: %s\n",_strDestIP);
	log_errors("Client Port: %d\n",_iSendPort);
}

void HKClient::LoadIni(){
	//Load ini file
	CSimpleIniCaseA ini(true,true,true);
	ini.Reset();
	SI_Error rc = ini.LoadFile("OCConfig.ini");

	if (rc < 0){
		log_errors("Couldn't find OCConfig.ini\n");
		return;
	}	
	//Get Section Names (Currently not necessary, we only have a 'Configuration' section.)
	CSimpleIniCaseA::TNamesDepend sections;
	ini.GetAllSections(sections);
	
	//Get Configuration keys  
	CSimpleIniCaseA::TNamesDepend keys;
	ini.GetAllKeys("OCNetworking.HKClient", keys);
	
	if(keys.size() <= 0){
		log_errors("Configuration section not found\n");
		return;
	}
	CSimpleIniCaseA::TNamesDepend::const_iterator i;	//list iterator

	//Loop through keys and get assoc. value.
	for (i = keys.begin(); i != keys.end(); ++i) { 
		const char * pszValue = ini.GetValue("OCNetworking.HKClient",i->pItem, NULL);
		
		//Look for .ini values.  LOOKUP and ELOOKUP macros defined in ParseIni.h
		LOOKUP_MM("DestinationIP",_strDestIP,STRING)
		ELOOKUP_MM("SendPort",_iSendPort,INTEGER)
		ELOOKUP_MM("RecvPort",_iRecvPort,INTEGER)
	}
}
/*
int HKClient::OnStartClient()
{
    try
    {
        // Create the socket instance
		m_clientSocket = new Socket(AddressFamily::InterNetwork, SocketType::Stream, ProtocolType::Tcp);
     
		// Create the end point 
		IPEndPoint *ipEnd = new IPEndPoint(IPAddress::Parse(_strDestIP), _iSendPort);
        // Connect to the remote host
        m_clientSocket->Connect(ipEnd);

        if (m_clientSocket->Connected)
        {
            //Wait for data asynchronously 
            WaitForData();
        }
        return 1;
    }
    catch (SocketException *se)
    {
		log_errors("HKClient::OnStartClient() failed -> %s\n",se->Message);
        return 0;
    }
}

void HKClient::WaitForData()
{
	try
	{
		if  ( m_pfnCallBack == NULL ) 
		{
			m_pfnCallBack = new AsyncCallback(this,&HKClient::OnDataReceived);
		}
		SocketPacket *theSocPkt = new SocketPacket();
		theSocPkt->m_currentSocket = m_clientSocket;
		// Start listening to the data asynchronously
		m_result = m_clientSocket->BeginReceive (theSocPkt->dataBuffer,
		                                        0, theSocPkt->dataBuffer->Length,
												SocketFlags::None, 
		                                        m_pfnCallBack, 
		                                        theSocPkt);
		 
	}
	catch(SocketException *se)
	{
		log_errors("HKClient::WaitForData() failed -> %s\n",se->Message);
	}

}
void HKClient::OnDataReceived(IAsyncResult *asyn)
{
	try
	{
		SocketPacket *socketData = __try_cast<SocketPacket*>(asyn->AsyncState);
		
		int iRx  = socketData->m_currentSocket->EndReceive (asyn);
		String *s = Encoding::ASCII->GetString(socketData->dataBuffer,0,iRx);

		_timer->Stop();
		_dTimeSinceLastPk2 = _timer->GetDurationInSecs();
		log_errors("Time since last packet: %f\n",_dTimeSinceLastPk2 - _dTimeSinceLastPk1);
		Sift(s);

		_timer->Stop();
		_dTimeSinceLastPk1 = _timer->GetDurationInSecs();

        WaitForData();
	}
	catch(SocketException *se)
	{
  		log_errors("HKClient::OnDataReceived() failed -> %s\n",se->Message);
	}
}
void HKClient::Sift(String *str){
	Char ch[] = {' '};
	String *split[] = str->Split(ch);
	IEnumerator *myEnum = split->GetEnumerator();
	StringBuilder *args = new StringBuilder("");
	Boolean bPkStart = false, bPkEnd = false, bPkComplete = false;

	try {
		TIMER_BEGIN(TIME);
		TIMER_BEFORE(TIME);
		log_errors("-- New Packet Stack --\n");
		
		TIMER_BEGIN(TIMEPK);
		TIMER_BEFORE(TIMEPK);
		while ( myEnum->MoveNext() )
		{
			
			String *s = __try_cast<String*>(myEnum->Current);

			//1.)  Find start tag
			if (  s->Trim()->Equals( "st" ) ){
				//Check for previous packet.  If found ignore
				if(bPkStart){
					//log_errors("Dropped packet(no end): %s\n", args->ToString());
					args->Remove(0,args->Length);	//Clear our buffer
					_iLostPackets++;
				}
				bPkStart = true;
				bPkEnd = false;
				bPkComplete = false;
			} else if (  s->Trim()->Equals( "end" ) ){
				//Check for previous packet.  If found ignore
				if(!bPkStart){
					//log_errors("Dropped packet(no start): %s\n", args->ToString());
					args->Remove(0,args->Length);	//Clear our buffer
					bPkComplete = false;
					_iLostPackets++;
				} else {
					bPkComplete = true;
				}
				bPkEnd = true;
				bPkStart = false;
			} else {
				if(bPkStart) {
					if (  !s->Trim()->Equals( "" ) ){
						args->Append(s);
						args->Append(" ");
					}
				}
				
			}
			if(bPkComplete){
				/*if(m_qWorldState->Count >= 50){
					//log_errors("HKClient::Sift() - World state stack full, clearing\n");
					FlushStack();
				}
				m_qWorldState->Push(args->ToString());	//Push Data to stack
				log_errors("   Complete Packet: %s\n",args->ToString());
				
				

				args->Remove(0,args->Length);			//Clear the argument string

				//Thread-safe signal that new data has arrived
				*//*
				Marshal::FreeHGlobal((int)sUnmanagedData);		//Don't clear this variable anywhere but here.
				sUnmanagedData = (char *)Marshal::StringToHGlobalAnsi(args->ToString()->Trim()).ToPointer();

				Interlocked::Exchange(&_bNewData,1);
				bPkComplete = false;
				TIMER_AFTER(TIMEPK);
				log_errors("    Time per packet: %f\n",TIMER_GET(TIMEPK));
				TIMER_BEFORE(TIMEPK);//Reset timer
			}
		}
		#ifdef _DEBUG_STACK_CONTENT1
			System::Collections::Generic::Stack<System::String*>::Enumerator iter = m_qWorldState->GetEnumerator();
			log_errors("-- Lost: %d --------\n",_iLostPackets);
			while(iter.MoveNext()){
				String *s = __try_cast<String*>(iter.Current);
				log_errors("Stack Content: %s\n",s->ToString());
			}
			log_errors("-----------------\n\n");
		#endif
			TIMER_AFTER(TIME);
			log_errors("    Time to sift: %f\n",TIMER_GET(TIME));
			log_errors("-- End Packet Stack --\n");
	} catch (Exception *ex){
		log_errors("HKClient::Sift() failed -> %s\n",ex->Message);
	}
}
void HKClient::SendMessageToServer(String *msg) {
    try {
        NetworkStream *networkStream = new NetworkStream(m_clientSocket);
		System::IO::StreamWriter *streamWriter = new System::IO::StreamWriter(networkStream);
        streamWriter->WriteLine(msg);
        streamWriter->Flush();
		streamWriter->Close();
    }
    catch (SocketException *se) {
        log_errors("HKClient::SendMessageToServer() failed -> %s\n",se->Message);
    }	
}

const char *HKClient::GetNewData(){
	//No data on the stack, return nothing.
//	if(_bNewData==0)return NULL;
//	if(m_qWorldState->Count <= 0)return NULL;

//	String * tmp = m_qWorldState->Pop()->ToString()->Trim();
	
	//Provide unmanaged memory pointer
//	Marshal::FreeHGlobal((int)sUnmanagedData);		//Don't clear this variable anywhere but here.
//	sUnmanagedData = (char *)Marshal::StringToHGlobalAnsi(tmp).ToPointer();
	
//	if(m_qWorldState->Count <= 0)Interlocked::Exchange(&_bNewData,0);
	Interlocked::Exchange(&_bNewData,0);
	return sUnmanagedData;
}

void HKClient::FlushStack(){
	m_qWorldState->Clear();
}
*/
const char *HKClient::GetNewData(){
	return sUnmanagedData;
}
void HKClient::SocketSetup(int iSocketType){
	

	if(bInitd != 1){
		printf("  - Error, class not initialized\n");
		return;
	}
	if(bSocketStarted != 0) {
		printf("  - Socket has already been started\n");
		return;
	}
	printf("-- Socket Setup --\n");
	printf("  - Socket Type:");
	_iSocketType = iSocketType;

	if(iSocketType == SOCKET_TYPE_SYNC){
		try {
			log_errors(" Synchronous\n");
			log_errors("  - Server IP: %s\n",_strDestIP);
			log_errors("  - Server Port: %d\n",_iRecvPort);
			
			bStop = bPause = 0;		//Set threading variables.

			//Initialize the threading pool
			log_errors("Initializing thread pool...");
				_hThreadHandleSt = new ThreadStart(this,&HKClient::thSyncSocket);
				_hThreadHandle = new Thread(_hThreadHandleSt);
			log_errors("Complete.\n");

			//Start thread
			log_errors("  - Starting Thread...");
				_hThreadHandle->Start();
			log_errors("Complete.\n");

			bSocketStarted = 1;	//Socket has started
		} catch (Exception *ep) {
			log_errors("Error in HKClient::SocketSetup: %s\n",ep->ToString());
			throw;
		}
	}
}
void HKClient::thSyncSocket(){
	TIMER_BEGIN(SyncTimer)	//Start our timer

	_timer->Stop();
	_dTimeSinceLastPk1 = _timer->GetDurationInSecs();
	
	try {
		IPEndPoint *listenerIP = new IPEndPoint(IPAddress::Parse(_strDestIP), _iRecvPort); 
		Byte opt[];

		dgram = new Byte[1024];
		serverSocket = new Socket(AddressFamily::InterNetwork,SocketType::Dgram,ProtocolType::Udp);
		serverSocket->Bind(listenerIP);
		
		while(bStop == 0){

			TIMER_BEFORE(SyncTimer);	//Set before time
			int nRecvd = serverSocket->Receive(dgram);
			TIMER_AFTER(SyncTimer);		//Set after time
			TIMER_OUTPUT(SyncTimer);	//Display output
			
			String *s = Encoding::ASCII->GetString(dgram,0,nRecvd);

			Marshal::FreeHGlobal((int)sUnmanagedData);
			sUnmanagedData = (char *)Marshal::StringToHGlobalAnsi(s).ToPointer();

			bRecvd = 1;	//Let the system know we've received new data.
			log_errors("Pk received: %s\n",sUnmanagedData);

			_timer->Stop();
			_dTimeSinceLastPk1 = _timer->GetDurationInSecs();
			m_qWorldState->Enqueue(s);

			if(m_qWorldState->get_Count() > 2){
				m_qWorldState->Dequeue();
				isValid = true;
			}
			//Pause a moment
			Sleep(15);
			while(bPause)Thread::Sleep(1);
		}
	} catch (Exception *ep){
		log_errors("  - Error in thSynchSocket thread: %s\n", ep->ToString());
		throw;
	}
	return;
}
double HKClient::timeSinceLastPacket(){
	_timer->Stop();
	_dTimeSinceLastPk2 = _timer->GetDurationInSecs();

	return _dTimeSinceLastPk2 - _dTimeSinceLastPk1;
}
bool HKClient::GetPosition(float *ar){
	if(!isValid)return isValid;

	Char ch[] = {' '};
	String *array2[] = m_qWorldState->ToArray();
	String *split0[] = array2[0]->Split(ch);
	String *split1[] = array2[1]->Split(ch);

	double v[3];
	double time = timeSinceLastPacket();

	//Divide velocity by the known time between server packets.
	v[0] = (Convert::ToDouble(split1[3])  - Convert::ToDouble(split0[3]))/0.03;
	v[1] = (Convert::ToDouble(split1[4])  - Convert::ToDouble(split0[4]))/0.03;
	v[2] = (Convert::ToDouble(split1[5])  - Convert::ToDouble(split0[5]))/0.03;

	log_errors("[%f] Velocity: %f %f %f\n",time,v[0],v[1],v[2]);
	ar[0] = Convert::ToDouble(split1[3]) + v[0] * time;// + 0.5 * time * time;
	ar[1] = Convert::ToDouble(split1[4]) + v[1] * time;// + 0.5 * time * time;
	ar[2] = Convert::ToDouble(split1[5]) + v[2] * time;// + 0.5 * time * time;
	return true;
}