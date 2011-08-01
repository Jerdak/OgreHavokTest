#ifndef __HKOBJECT__
#define __HKOBJECT__
#pragma once


#include "HKCommon.h"

#include "ExampleApplication.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include "../res/resource.h"
#endif
#include <CEGUI.h>
#include <CEGUISystem.h>
#include <CEGUISchemeManager.h>
#include <OgreCEGUIRenderer.h>

#include <stdio.h>

#include "GDebugger.h"
#include "GHavok.h"
#include "GRakNet.h"

class hkpRigidBody;

//TODO:  ReWrite the descriptor class to match the ones in Primitive and CharacterController
//			Then those descrips will subclass from this one.
class HKObjectDescrip {
public:
	enum OBJPHYSTYPE { OBJ_PHYS_SPHERE, OBJ_PHYS_CUBE, OBJ_PHYS_MOPP, OBJ_PHYS_COMPOUND, OBJ_PHYS_END};

	HKObjectDescrip(){
		_ogreEntity = NULL;
		_ogreScene  = NULL;
		_hkRigid	= NULL;
		
		_bHidden	= false;

		_sID		= "GenericObject";
		_vPos		= Ogre::Vector3(0,0,0);
		_qOrient	= Ogre::Quaternion();
		
		_iPhysType	= OBJ_PHYS_CUBE;
	}
	HKObjectDescrip(const HKObjectDescrip &d){Assign(d);}
	virtual ~HKObjectDescrip();

	void Assign(const HKObjectDescrip &d){
		_sID		= d._sID;
		_vPos		= d._vPos;
		_qOrient	= d._qOrient;
		_iPhysType  = d._iPhysType;
		_ogreEntity = d._ogreEntity;
		_ogreScene	= d._ogreScene;
		_hkRigid	= d._hkRigid;

	}
	void operator = (const HKObjectDescrip &d){ Assign(d); }

	Ogre::String		_sID;
	Ogre::Vector3		_vPos;			//Position
	Ogre::Quaternion	_qOrient;		//Direction to face
	Ogre::Entity		*_ogreEntity;	//Ogre entity
	Ogre::SceneNode		*_ogreScene;	//Ogre scene node

	hkpRigidBody		*_hkRigid	;	//Rigid body physics wrapper
	int					_iPhysType;		//Physics Type
	bool				_bHidden;		//Hidden flag
};


class HKObject : public RakNet::Replica2 {
public:
	HKObject(const HKObjectDescrip &d): _bHidden(false),_descrip(d)
	{ 
		_sceneMgr = Ogre::Root::getSingleton ().getSceneManager ("ExampleSMInstance"); 
		if(_sceneMgr == NULL){
			GDebugger::GetSingleton().WriteToLog("  ERROR in object constructor, could not find SceneManager\n");
		} else {
			GDebugger::GetSingleton().WriteToLog("  - SceneMgr found: %s\n",_sceneMgr->getName().c_str());
		}
	};
	virtual ~HKObject(void){
		_sceneMgr = NULL;
	}


	//Get or set object description.
	HKObjectDescrip getDescrip() { return _descrip; }
	void setDescrip(const HKObjectDescrip& d){ _descrip = d; }

	//RakNet Networking classes
	virtual bool SerializeConstruction(RakNet::BitStream *bitStream, RakNet::SerializationContext *serializationContext){return true;}
	virtual bool Serialize(RakNet::BitStream *bitStream, RakNet::SerializationContext *serializationContext){return true;}
	virtual void Deserialize(RakNet::BitStream *bitStream, RakNet::SerializationType serializationType, SystemAddress sender, RakNetTime timestamp){}

	//Not all objects require step but most do.
	virtual void Step(double dt){}
	virtual void Spawn(){}

	SceneManager		*_sceneMgr;		//Ogre scene mgr.
	char *test;
	int x;
protected:
	bool				_bHidden;
private:
	HKObjectDescrip		_descrip;		//Object description

};

class HKActorDescrip : public HKObjectDescrip{
public:
	HKActorDescrip():HKObjectDescrip(){
		_sID		= "GenericActor";
	}
	HKActorDescrip(const HKActorDescrip &d){Assign(d);}
	virtual ~HKActorDescrip(){}

	void operator = (const HKActorDescrip &d){Assign(d);}

	void Assign(const HKActorDescrip &d){
		_sID		= d._sID;
		_vPos		= d._vPos;
		_qOrient	= d._qOrient;
		_iPhysType  = d._iPhysType;
		_ogreEntity = d._ogreEntity;
		_ogreScene	= d._ogreScene;
		_hkRigid	= d._hkRigid;
	}
};

//Actor is a type of Object that can be grabbed and moved.
class HKActor : public HKObject {
public:
	HKActor(const HKActorDescrip &d) : _bGrab(false),
									   _descrip(d),
									   HKObject(d)
	{ 
	};
	virtual ~HKActor(void){};
	
	//Get or set object description.
	HKActorDescrip getDescrip() { return _descrip; }
	void setDescrip(const HKActorDescrip& d){ _descrip = d; }

	virtual void SetGrab(bool b){ _bGrab = b;}
	virtual void Step(double dt){};

	virtual void Spawn(){}
protected:
	bool				_bGrab;			//Actor has been grabbed
private:
	HKActorDescrip		_descrip;
};
#endif