#include "HKTracker.h"
#include "HKCursor.h"
#include "HKCharacterController.h"

using namespace RakNet;

HKTracker::HKTracker(void)
{
	Init();
}

HKTracker::~HKTracker(void)
{
}

void HKTracker::Init(){
	
	//Defaults
	_rakPeer=NULL;
	_dMph=0.0f;
	_vScreenCoords.x = _vScreenCoords.y = 0.5f;
	
	_hkCharCont = NULL;
	_hkCursor = NULL;

	_sIP = "127.0.0.1";
	_iPort = 11111;
	_timer.Start();
	_dTimeSinceLastPacket = 0;
	_bIsServer = false;
	_bTrackerConnected = false;
	_bConnected = false;
	_bAcceptInput = false;
	_dSendDelay = 30;

	LoadIni("HTest1.ini");
}
/*Register functions on the server*/
void HKTracker::RegisterFunctions(RakPeerInterface *rakpeer){
	if(!_bIsServer){
		GDebugger::GetSingleton().WriteToLog("Can't register functions on the client.\n");
		return;
	}
	REGISTER_CLASS_MEMBER_RPC(rakpeer,HKTracker,clientRPC);
}

/*
ConnectServerToClient():  Set 'this' HKTracker as the server and open a connect with any client on the specified port.
*/
void HKTracker::ConnectServerToClient(){
	if(_bConnected){
		if(_bIsServer)GDebugger::GetSingleton().WriteToLog("HKTracker is already connected from server to client\n");
		else		 GDebugger::GetSingleton().WriteToLog("HKTracker is already connected from client to server\n");
	}
	try {
		GDebugger::GetSingleton().WriteToLog("  - HKTracker Server --\n");

		_bIsServer = true;
			
		_rakPeer=RakNetworkFactory::GetRakPeerInterface();
		GDebugger::GetSingleton().WriteToLog("      - RakPeer created\n");

		SocketDescriptor sd;
		sd.port = 11111;
		if (_rakPeer->Startup(1,30,&sd, 1)==false)
		{
			GDebugger::GetSingleton().WriteToLog("Start call failed!\n");
			return;
		}
		GDebugger::GetSingleton().WriteToLog("      - RakPeer started\n");

		_rakPeer->SetMaximumIncomingConnections(1);
	DBGLINE
		_networkIDManager.SetIsNetworkIDAuthority(true);  // The server has the authority to create NetworkIDs
	DBGLINE
		_rakPeer->SetNetworkIDManager(&_networkIDManager);
	DBGLINE
	this->SetNetworkIDManager(&_networkIDManager);
	DBGLINE
		GDebugger::GetSingleton().WriteToLog("      - RakPeer set as network authority;\n");

		RegisterFunctions(_rakPeer);
		GDebugger::GetSingleton().WriteToLog("      - Registered class member functions for RPC\n");
		_bConnected = true;
	} catch (Ogre::Exception &ex){
		dprintf("[ERROR] HKTracker::ConnecterServerToClient(): %s\n",ex.getDescription());
	}
}

/*
ConnectServerToClient():  Set 'this' HKTracker as a client.  It was weird to me at first to think of the client
being the component that updates the server but whatever.  It's 1:00 a.m. and I don't give a rats ass.
*/
void HKTracker::ConnectClientToServer(){
	if(_bConnected){
		if(_bIsServer)GDebugger::GetSingleton().WriteToLog("HKTracker is already connected from server to client\n");
		else		 GDebugger::GetSingleton().WriteToLog("HKTracker is already connected from client to server\n");
	}

	GDebugger::GetSingleton().WriteToLog("  - HKTracker Client --\n");
	_bIsServer = false;

	_rakPeer = RakNetworkFactory::GetRakPeerInterface();

	GDebugger::GetSingleton().WriteToLog("      - RakPeer created\n");

	SocketDescriptor sd;
	sd.port=0;

	_rakPeer->Startup(1, 30, &sd, 1);
	if (_rakPeer->Connect(_sIP.c_str(), _iPort, 0, 0)==false)
	{
		GDebugger::GetSingleton().WriteToLog("  - Could not open RakNet client.\n");
		return;
	}
	GDebugger::GetSingleton().WriteToLog("      - RakPeer started and connected\n");

	_networkIDManager.SetIsNetworkIDAuthority(false);
	this->SetNetworkIDManager(&_networkIDManager);
	GDebugger::GetSingleton().WriteToLog("      - RakPeer set as network id slave\n");

	_bConnected = true;
	GDebugger::GetSingleton().WriteToLog("Complete.\n");
}

/*Custom remote procedure call:  Uses serialized data*/
void HKTracker::clientRPC(RPCParameters *rpcParameters){
	BitStream b(rpcParameters->input, BITS_TO_BYTES(rpcParameters->numberOfBitsOfData), false);

	bool bRead;
	b.Read(bRead);
	b.ReadAlignedBytes((unsigned char*)&_vScreenCoords,sizeof(_vScreenCoords));
	b.ReadAlignedBytes((unsigned char*)&_dMph,sizeof(_dMph));

	_timer.Stop();
	double dur = _timer.GetDurationInSecs();

	GDebugger::GetSingleton().WriteToLog("[Time: %f]Read values[%d]: %f %f %f\n",dur - _dTimeSinceLastPacket,bRead,_dMph,_vScreenCoords.x,_vScreenCoords.y);
	_dTimeSinceLastPacket = dur;
}

bool HKTracker::LoadIni(char *file){
	return true;
}

int ct = 0;
/*
Update():  Updates the packet state and clears the data;
*/
void HKTracker::Update(double dt){
	if(!_bConnected)return;

	
	Packet *p;
	if(_bIsServer){
		for (p=_rakPeer->Receive(); p; _rakPeer->DeallocatePacket(p), p=_rakPeer->Receive())
		{
			switch (p->data[0])
			{
				case ID_DISCONNECTION_NOTIFICATION:
					GDebugger::GetSingleton().WriteToLog("ID_DISCONNECTION_NOTIFICATION\n");
					break;
				case ID_ALREADY_CONNECTED:
					GDebugger::GetSingleton().WriteToLog("ID_ALREADY_CONNECTED\n");
					break;
				case ID_CONNECTION_ATTEMPT_FAILED:
					GDebugger::GetSingleton().WriteToLog("Connection attempt failed\n");
					break;
				case ID_NO_FREE_INCOMING_CONNECTIONS:
					GDebugger::GetSingleton().WriteToLog("ID_NO_FREE_INCOMING_CONNECTIONS\n");
					break;
				case ID_CONNECTION_REQUEST_ACCEPTED:
					// This tells the client they have connected
					GDebugger::GetSingleton().WriteToLog("ID_CONNECTION_REQUEST_ACCEPTED\n");
					break;
				case ID_NEW_INCOMING_CONNECTION:
				{
					GDebugger::GetSingleton().WriteToLog("      - Client connected sending network id....");

					RakNet::BitStream bs;
					GDebugger::GetSingleton().WriteToLog("1...");
					bs.Write(unsigned char(ID_USER_SERVERTRACKER_CREATED));
					GDebugger::GetSingleton().WriteToLog("2...");
					bs.Write(this->GetNetworkID());
					GDebugger::GetSingleton().WriteToLog("3....");
					_rakPeer->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, p->systemAddress, false);
					GDebugger::GetSingleton().WriteToLog("Complete\n");
					break;
				}
			}
		}
		_rakPeer->DeallocatePacket(_rakPeer->Receive());
		if(_bAcceptInput){
			//Update cursor and treadmill on server
			if(_hkCursor!=NULL)_hkCursor->SetPosition(Ogre::Vector3(_renderWindow->getWidth()*_vScreenCoords.x,_renderWindow->getHeight()*_vScreenCoords.y,0.0));
			
			if(_hkCharCont!=NULL){
				GDebugger::GetSingleton().WriteToLog("Setting movement speed: %f\n",_dMph);
				_hkCharCont->SetMovementSpeed(abs(_dMph));
			}
			if(_dMph > 0)if(_hkCharCont!=NULL)_hkCharCont->Forward(1);
			if(_dMph < 0)if(_hkCharCont!=NULL)_hkCharCont->Backward(1);
		}
	} else {
		for (p=_rakPeer->Receive(); p; _rakPeer->DeallocatePacket(p), p=_rakPeer->Receive())
		{
			switch (p->data[0])
			{
				case ID_DISCONNECTION_NOTIFICATION:
					GDebugger::GetSingleton().WriteToLog("ID_DISCONNECTION_NOTIFICATION\n");
					break;
				case ID_ALREADY_CONNECTED:
					GDebugger::GetSingleton().WriteToLog("ID_ALREADY_CONNECTED\n");
					break;
				case ID_CONNECTION_ATTEMPT_FAILED:
					GDebugger::GetSingleton().WriteToLog("Connection attempt failed\n");
					break;
				case ID_NO_FREE_INCOMING_CONNECTIONS:
					GDebugger::GetSingleton().WriteToLog("ID_NO_FREE_INCOMING_CONNECTIONS\n");
					break;
				case ID_CONNECTION_REQUEST_ACCEPTED:
					// This tells the client they have connected
					GDebugger::GetSingleton().WriteToLog("ID_CONNECTION_REQUEST_ACCEPTED\n");
					break;
				case ID_NEW_INCOMING_CONNECTION:
				{
					GDebugger::GetSingleton().WriteToLog("New incoming connection\n");
					break;
				}
				case ID_USER_SERVERTRACKER_CREATED:
					RakNet::BitStream bs(p->data, p->length, false);
					// Ignore the message ID
					bs.IgnoreBits(8);
					bs.Read(_netID);
					this->SetNetworkID(_netID);
					GDebugger::GetSingleton().WriteToLog("Tracker created on server: %d\n",_netID);
					_bTrackerConnected = true;
					const char *hello="Hello World!!!!!";
					_rakPeer->RPC(CLASS_MEMBER_ID(HKTracker,clientRPC), hello, (unsigned int) (strlen(hello)+1)*8, HIGH_PRIORITY, RELIABLE_ORDERED,0, UNASSIGNED_SYSTEM_ADDRESS, true, 0, this->GetNetworkID(),0);
					
					break;
			}
		}
		_rakPeer->DeallocatePacket(_rakPeer->Receive());
	}
	if(_bTrackerConnected && !_bIsServer){
		BitStream outgoingBitstream;
		outgoingBitstream.Write(true);
		outgoingBitstream.WriteAlignedBytes((const unsigned char*)&_vScreenCoords,sizeof(_vScreenCoords));
		outgoingBitstream.WriteAlignedBytes((const unsigned char*)&_dMph,sizeof(_dMph));
		bool success = _rakPeer->RPC(CLASS_MEMBER_ID(HKTracker,clientRPC),&outgoingBitstream, HIGH_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true, 0, this->GetNetworkID(), 0); // broadcast to everyone, which happens to be our one client
	}
}