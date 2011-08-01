#pragma once

#include "GDebugger.h"
#include <Ogre.h>

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

#include <string>

#include "ProfTimer.h"
#include "Singleton.h"

enum {
	ID_USER_SERVERTRACKER_CREATED = ID_USER_PACKET_ENUM+1
};
class HKCursor;
class HKCharacterController;
class HKTracker : public NetworkIDObject
{
public:
	HKTracker(void);
	virtual ~HKTracker(void);

	void AttachCursor(HKCursor *p){_hkCursor = p;}
	void AttachCharacterController(HKCharacterController *p){_hkCharCont = p;}
	void AttachRenderWindow(Ogre::RenderWindow *r){_renderWindow = r;}

	void ConnectServerToClient();		//Connect server to client;
	void ConnectClientToServer();		//Connect client to server
	void Init();				//Initialize variables
	bool LoadIni(char *file);	//Load ini file

	//Get-Set Server
	bool IsServer(){ return _bIsServer;}
	void IsServer(const bool &b){_bIsServer = b;}

	//Get-Set Destination IP
	std::string GetIp(){return _sIP;}
	void SetIp(const std::string &s){ _sIP = s; }

	//Get-Set Destination Port
	int GetPort(){return _iPort;}
	void SetPort(const int &i){_iPort = i;}

	//Get-Set Miles Per Hour
	double GetMph(){return _dMph;}
	void SetMph(const double &d){_dMph = d;}

	//Get-Set Screen Coordinates
	Ogre::Vector3 GetScreenCoords(){return _vScreenCoords;}
	void SetScreenCoords(const Ogre::Vector3 &v){_vScreenCoords = v;}

	void SetSendDelay(const double &d){_dSendDelay = d;}
	double GetSendDelay(){return _dSendDelay;}

	//Register functions on server
	void RegisterFunctions(RakPeerInterface*);
	
	void SetAcceptInput(bool b){_bAcceptInput = b;}
	bool GetAcceptInput(){return _bAcceptInput;}
	//Update and spool through packet queue.
	void Update(double dt);

	virtual void __cdecl clientRPC(RPCParameters *rpcParameters);
private:

	/*Packet Variables*/
	Ogre::RenderWindow *_renderWindow;
	Ogre::Vector3 _vScreenCoords;
	double _dMph;
	double _dTimeSinceLastPacket;
	double _dSendDelay;

	/*Class Variables*/
	bool _bIsServer;
	bool _bConnected;
	bool _bTrackerConnected;
	bool _bAcceptInput;

	std::string _sIP;
	int	_iPort;
	RakPeerInterface *_rakPeer;				//
	
	HKCharacterController *_hkCharCont;			//Pointer to character controller
	HKCursor *_hkCursor;						//Pointer to HKCursor
	ProfTimer _timer;							//Hires timer

	NetworkIDManager _networkIDManager;			//Network ID manager
	NetworkID _netID;							//Network ID, assigned by server
};