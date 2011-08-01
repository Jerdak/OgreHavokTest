#ifndef __GRAKNET__
#define __GRAKNET__
#pragma once

#include "Singleton.h"

#include "SimpleIni.h"

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <map>

#include <RakNet3.3/Include/GetTime.h>
#include <RakNet3.3/Include/RakSleep.h>
#include <RakNet3.3/Include/RakAssert.h>
#include <RakNet3.3/Include/StringTable.h>
#include <RakNet3.3/Include/RakPeerInterface.h>
#include <RakNet3.3/Include/RakNetworkFactory.h>
#include <RakNet3.3/Include/BitStream.h>
#include <RakNet3.3/Include/MessageIdentifiers.h>
#include <RakNet3.3/Include/ReplicaManager2.h>
#include <RakNet3.3/Include/NetworkIDManager.h>
#include <RakNet3.3/Include/RakSleep.h>
#include <RakNet3.3/Include/FormatString.h>
#include <RakNet3.3/Include/StringCompressor.h>
#include <RakNet3.3/Include/Rand.h>

#include "Proftimer.h"
#include "TransformationHistory.h"


#pragma once

class HKObject;
class GRakNet : public HK::Singleton<GRakNet> {
	friend class HK::Singleton<GRakNet>;
public:
	GRakNet():_bIsServer(false){}
	~GRakNet(){
		RakNetworkFactory::DestroyRakPeerInterface(_rakPeer);
	}

	void AddObject(HKObject *p);

	RakNet::ReplicaManager2 *GetReplicaManager(Ogre::String str);
	bool LoadIni(char *file);
	void Init(SocketDescriptor &sd);
	bool IsServer(){return _bIsServer;}
	void IsServer(bool b) { _bIsServer = b; }

	int GetTimeDelay(){return _iTimeBetweenUpdates;}
	void SetTimeDelay(int i){ _iTimeBetweenUpdates = i;}

	
	void Update(double dt);

private:
	bool _bIsServer;
	std::vector<HKObject*> _vObjectList;

	NetworkIDManager _networkIdManager;
	RakPeerInterface *_rakPeer;
	std::map<Ogre::String,RakNet::ReplicaManager2*> _vReplicatorList;

	int _iTimeBetweenUpdates;
	char _sServerIP[256];
	unsigned short _usServerPort;
};


#endif