#include "HKCursor.h"

using namespace Ogre;
using namespace DataStructures;
using namespace RakNet;

HKCursor::HKCursor(const HKCursorDescrip &d):HKObject(d){
	Init();
}

HKCursor::~HKCursor(){

}
HKCursor *HKCursor::Create(){
	HKCursor *c = NULL;
	

	GDebugger::GetSingleton().WriteToLog("  - HKCursor:  New cursor being created....");
	//Create new character controller descriptor
	HKCursorDescrip tmpDes;
	{
		

	}
	//Create new character controller
	{
		c = new HKCursor(tmpDes);
		c->SetRenderWindow(Ogre::Root::getSingleton().getAutoCreatedWindow());
		c->SetReplicaManager(GRakNet::GetSingleton().GetReplicaManager("cross_screen_cursor")); //Find network replication controller

		GRakNet::GetSingleton().AddObject(c);
		//GHavok::GetSingleton().AddObject(c);		
	}
	
	//If this is the server be sure to automatically broadcast object creation to the client.
	if(GRakNet::GetSingleton().IsServer()){
		c->AddAutoSerializeTimer(GRakNet::GetSingleton().GetTimeDelay()  );
		// All connected systems should create this object

		
		c->BroadcastConstruction();
		// Force the first serialize to go out. We could have also just wrote the data in SerializeConstruction()
		// Without this call, no serialize would occur until something changed from the intial value

		c->BroadcastSerialize();
	}
	GDebugger::GetSingleton().WriteToLog("Complete.\n");
	
	return c;
}

void HKCursor::Step(double dt){

	
	//TODO:  We really need to make this update function it's own thread.  Otherwise we are dependent on the frame rate to keep the updates going out.
	if(GRakNet::GetSingleton().IsServer()){
		_dTimeSinceLastUpdate+=dt;
		if(_dTimeSinceLastUpdate < 0.045)return;

		_dTimeSinceLastUpdate	=   0;

		//Get the current screen position and convert it to a normalized vector	
		CEGUI::Point pt = CEGUI::MouseCursor::getSingleton().getPosition();
		_position.x = pt.d_x / _renderWindow->getWidth();
		_position.y = pt.d_y / _renderWindow->getHeight();

	} else {
		_visiblePosition		=	_position;
		//Interpolate
		//GDebugger::GetSingleton().WriteToLog("  - PositionA: %f %f\n",_visiblePosition.x,_visiblePosition.y);
		if (_bEnableInterpolation)
		{
			Ogre::Quaternion q;
			_transformationHistory.Read(&_visiblePosition, 0, &q, RakNet::GetTime()-GRakNet::GetSingleton().GetTimeDelay(),RakNet::GetTime());
			//_visiblePosition = _transformationHistory2.Read(dt,_visiblePosition);
		}
		//GDebugger::GetSingleton().WriteToLog("  - PositionB: %f %f\n",_visiblePosition.x,_visiblePosition.y);
		_position = _visiblePosition;

		//Check if position is withing the range of this client.  
		if(_position.x >= _minPositionRange.x && _position.x < _maxPositionRange.x &&\
		   _position.y >= _minPositionRange.y && _position.y < _maxPositionRange.y) {
			   Vector3 pos;
			 
			   //Apply the virtual screen position
				pos.x = _renderWindow->getWidth() * ((_position.x - _minPositionRange.x) / ( _maxPositionRange.x - _minPositionRange.x));
				pos.y = _renderWindow->getHeight() * ((_position.y - _minPositionRange.y) / ( _maxPositionRange.y - _minPositionRange.y));
			 
				//GDebugger::GetSingleton().WriteToLog("  - Pos2: %f %f\n",pos.x,pos.y);
			   CEGUI::MouseCursor::getSingleton().setPosition(CEGUI::Point(CEGUI::Vector2(pos.x,pos.y))); 
			CEGUI::MouseCursor::getSingleton().show();
		} else {
			CEGUI::MouseCursor::getSingleton().hide();
		}
	}
}
void HKCursor::Init(){
	GDebugger::GetSingleton().WriteToLog("  - HKCursor Intializing variables...");
	_minPositionRange = Vector3(0,0,0);
	_maxPositionRange = Vector3(1,1,1);
	_renderWindow = NULL;
	_bEnableInterpolation = true;


	_transformationHistory.Init(45,1000);
	_dTimeSinceLastUpdate=0;
	LoadIni("HTest1.ini");
	GDebugger::GetSingleton().WriteToLog("Complete\n");
}

bool HKCursor::LoadIni(char *file)
{
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
	ini.GetAllKeys("CrossScreenCursor", keys);
	if(keys.size() <= 0) {
		printf("CrossScreenCursor section not found.\n");
		return false;
	}
	CSimpleIniCaseA::TNamesDepend::const_iterator i; // list iterator
	for(i=keys.begin(); i!=keys.end(); ++i) {
		const char* pszValue = ini.GetValue("CrossScreenCursor",i->pItem, NULL);
			
		LOOKUP("minX",			_minPositionRange.x,DOUBLE)
		ELOOKUP("minY",			_minPositionRange.y,DOUBLE)
		ELOOKUP("minZ",			_minPositionRange.z,DOUBLE)
		ELOOKUP("maxX",			_maxPositionRange.x,DOUBLE)
		ELOOKUP("maxY",			_maxPositionRange.y,DOUBLE)
		ELOOKUP("maxZ",			_maxPositionRange.z,DOUBLE)
	}
	return true;
}
bool HKCursor::SerializeConstruction(RakNet::BitStream *bitStream, SerializationContext *serializationContext)
{
	RakAssert(GRakNet::GetSingleton().IsServer()==true);
	StringTable::Instance()->EncodeString("cross_screen_cursor", 128, bitStream);
	return true;
}

bool HKCursor::Serialize(RakNet::BitStream *bitStream, SerializationContext *serializationContext)
{
	// Autoserialize causes a network packet to go out when any of these member variables change.
	RakAssert(GRakNet::GetSingleton().IsServer()==true);
	bitStream->Write(true);
	bitStream->WriteAlignedBytes((const unsigned char*)&_position,sizeof(_position));

	
	return true;
}

void HKCursor::Deserialize(RakNet::BitStream *bitStream, SerializationType serializationType, SystemAddress sender, RakNetTime timestamp)
{
	Ogre::Vector3 pos2;
	// Doing this because we are also lagging position and orientation behind by DEFAULT_SERVER_MILLISECONDS_BETWEEN_UPDATES
	// Without it, the kernel would pop immediately but would not start moving
	bool bRead = true;
	bitStream->Read(bRead);
	bitStream->ReadAlignedBytes((unsigned char*)&pos2,sizeof(pos2));

	// Every time we get a network packet, we write it to the transformation history class.
	// This class, given a time in the past, can then return to us an interpolated position of where we should be in at that time
	_transformationHistory.Write(pos2,Vector3(0,0,0),Quaternion(0,0,0),RakNet::GetTime());
	_transformationHistory2.Write(pos2);
	//GDebugger::GetSingleton().WriteToLog("Readin data\n");
}