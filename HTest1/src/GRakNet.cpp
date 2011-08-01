#include "GRakNet.h"
#include "GDebugger.h"
#include "HKObject.h"
#include "HKCharacterController.h"
#include "HKCursor.h"
#include "HKPrimitive.h"

using namespace DataStructures;
using namespace RakNet;

class HKObjectConnection : public RakNet::Connection_RM2
{
public:
	HKObjectConnection() {}
	virtual ~HKObjectConnection() {}

	// Callback used to create objects
	// See Connection_RM2::Construct in ReplicaManager2.h for a full explanation of each parameter
	RakNet::Replica2* Construct(RakNet::BitStream *replicaData, SystemAddress sender, RakNet::SerializationType type,RakNet::ReplicaManager2 *replicaManager, RakNetTime timestamp, NetworkID networkId, bool networkIDCollision)
	{
		char objectName[128];
		RakNet::StringTable::Instance()->DecodeString(objectName,128,replicaData);
		GDebugger::GetSingleton().WriteToLog("  - RakNet: Looking for factory object[%s]...",objectName);
		if (strcmp(objectName,"character_controller")==0)
		{
			GDebugger::GetSingleton().WriteToLog("Character Controller Found...");
			return HKCharacterController::Create();
		} else  if (strcmp(objectName,"cross_screen_cursor")==0) {
			GDebugger::GetSingleton().WriteToLog("Cross Screen Cursor Found...");
			return HKCursor::Create();
		} else  if (strcmp(objectName,"primitive")==0) {
			GDebugger::GetSingleton().WriteToLog("Primitive Found...");
			return HKPrimitive::Create();
		} else {
			GDebugger::GetSingleton().WriteToLog("Not found...");
		}
		GDebugger::GetSingleton().WriteToLog("Complete.\n");
		return 0;
	}
};

// This is a required class factory, that creates and destroys instances of ReplicaManager2DemoConnection
class ObjectFactory : public RakNet::Connection_RM2Factory {
	virtual Connection_RM2* AllocConnection(void) const {return new HKObjectConnection;}
	virtual void DeallocConnection(Connection_RM2* s) const {delete s;}
};

void GRakNet::AddObject(HKObject *p){
	_vObjectList.push_back(p);
}

void GRakNet::Update(double dt){
	Packet *packet;
	for (packet = _rakPeer->Receive(); packet; _rakPeer->DeallocatePacket(packet), packet = _rakPeer->Receive())
	{
		switch (packet->data[0])
		{
		case ID_CONNECTION_ATTEMPT_FAILED:
			GDebugger::GetSingleton().WriteToLog("      - ID_CONNECTION_ATTEMPT_FAILED\n");
			break;
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			GDebugger::GetSingleton().WriteToLog("      - ID_NO_FREE_INCOMING_CONNECTIONS\n");
			break;
		case ID_CONNECTION_REQUEST_ACCEPTED:
			GDebugger::GetSingleton().WriteToLog("      - ID_CONNECTION_REQUEST_ACCEPTED\n");
			break;
		case ID_NEW_INCOMING_CONNECTION:
			GDebugger::GetSingleton().WriteToLog("      - ID_NEW_INCOMING_CONNECTION from %s\n", packet->systemAddress.ToString());
			break;
		case ID_DISCONNECTION_NOTIFICATION:
			GDebugger::GetSingleton().WriteToLog("      - ID_DISCONNECTION_NOTIFICATION\n");
			break;
		case ID_CONNECTION_LOST:
			GDebugger::GetSingleton().WriteToLog("      - ID_CONNECTION_LOST\n");
			break;
		}
	}

	for(int i = 0; i < _vObjectList.size(); i++){
		_vObjectList[i]->Step(dt);
	}
}

void GRakNet::Init(SocketDescriptor &sd){
	try {
		_iTimeBetweenUpdates = 30;
		sprintf(_sServerIP,"192.168.0.191");
		_usServerPort = 12345;

		LoadIni("HTest1.ini");

		GDebugger::GetSingleton().WriteToLog("  - Intializing RakNet\n");
		GDebugger::GetSingleton().WriteToLog("      - Server IP: %s\n",_sServerIP);
		GDebugger::GetSingleton().WriteToLog("      - Server Port: %d\n",_usServerPort);
		GDebugger::GetSingleton().WriteToLog("      - Server Delay(ms): %d\n",_iTimeBetweenUpdates);

		_networkIdManager.SetIsNetworkIDAuthority(_bIsServer);
		// Start RakNet, up to 32 connections if the server
		_rakPeer = RakNetworkFactory::GetRakPeerInterface();
		_rakPeer->Startup(_bIsServer ? 32 : 1,30,&sd,1);

		GDebugger::GetSingleton().WriteToLog("      - Peer started\n");

		_rakPeer->SetNetworkIDManager(&_networkIdManager);

		
		GDebugger::GetSingleton().WriteToLog("      - Adding factories:\n");
		
		// Register our custom connection factories
		ReplicaManager2 *repChar = new ReplicaManager2();
		ObjectFactory *fac = new ObjectFactory();
		repChar->SetConnectionFactory(fac);
		repChar->SetDefaultPacketPriority(MEDIUM_PRIORITY);//SYSTEM_PRIORITY);
		repChar->SetDefaultPacketReliability(RELIABLE_ORDERED);//RELIABLE_SEQUENCED);
		repChar->SetDoReplicaAutoSerializeUpdate(true);

		_rakPeer->AttachPlugin(repChar);
		_vReplicatorList["character_controller"] = repChar;
		_vReplicatorList["cross_screen_cursor"] = repChar;
		_vReplicatorList["primitive"] = repChar;

		GDebugger::GetSingleton().WriteToLog("           1.)character_controller\n");
		GDebugger::GetSingleton().WriteToLog("           2.)cross screen cursor\n");
		GDebugger::GetSingleton().WriteToLog("           3.)primitive\n");

		// The server should allow systems to connect. Clients do not need to unless you want to use RakVoice or for some other reason want to transmit directly between systems.
		if (_bIsServer)
			_rakPeer->SetMaximumIncomingConnections(32);
		else
			_rakPeer->Connect(_sServerIP,_usServerPort,0,0);

		GDebugger::GetSingleton().WriteToLog("      - Connections set.\n");
		
		// StringTable has to be called after RakPeer started, or else first call StringTable::AddRef() yourself
		StringTable::Instance()->AddString("Object",false);
		StringTable::Instance()->AddString("character_controller",false);
		StringTable::Instance()->AddString("Character_controller",false);
		StringTable::Instance()->AddString("Cross_screen_cursor",false);
		StringTable::Instance()->AddString("cross_screen_cursor",false);
		StringTable::Instance()->AddString("primitive",false);
		GDebugger::GetSingleton().WriteToLog("      - String table complete\n");
	} catch (Ogre::Exception &ex) {
		GDebugger::GetSingleton().WriteToLog("[ERROR] in GRakNet::Init(): %s\n",ex.getDescription());
	}
}
RakNet::ReplicaManager2 *GRakNet::GetReplicaManager(Ogre::String str){
	std::map<Ogre::String,RakNet::ReplicaManager2*>::const_iterator iter;

	iter = _vReplicatorList.find(str);
	
	if(iter == _vReplicatorList.end()){
		GDebugger::GetSingleton().WriteToLog("  - ERROR.  Could not find ReplicaManager: %s\n",str.c_str());
		return NULL;
	}  else {
		GDebugger::GetSingleton().WriteToLog("  - RakNet: Found replicator list item for %s: %s\n",str.c_str(),iter->first.c_str());
	}
	return iter->second;
}

bool GRakNet::LoadIni(char *file)
{
	char s[6];

	// Load ini file
	CSimpleIniCaseA ini(true, true, true);
	ini.Reset();
	SI_Error rc = ini.LoadFile(file);
	
	if(rc<0) {
		printf("Couldn't find %s\n", file);
		return false;
	}
	// Get section names (Currently not necessary, we only have a 'Configuration' section
	CSimpleIniCaseA::TNamesDepend sections;
	ini.GetAllSections(sections);

	// Get FOV keys
	CSimpleIniCaseA::TNamesDepend keys;
	ini.GetAllKeys("Networking", keys);
	if(keys.size() <= 0) {
		printf("Networking section not found.\n");
		return false;
	}
	CSimpleIniCaseA::TNamesDepend::const_iterator i; // list iterator
	for(i=keys.begin(); i!=keys.end(); ++i) {
		const char* pszValue = ini.GetValue("Networking",i->pItem, NULL);
			
		LOOKUP("UpdateSpeed",		_iTimeBetweenUpdates,INTEGER)
		ELOOKUP("ServerIP",			_sServerIP,STRING)
		ELOOKUP("ServerPort",		_usServerPort,INTEGER)
	}
	return true;
}