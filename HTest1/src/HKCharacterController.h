#ifndef __HKCHARACTERCONTROLLER__
#define __HKCHARACTERCONTROLLER__

#pragma once

#include "GHavok.h"
#include "GRakNet.h"
#include "GOffAxisCamera.h"
#include "ProfTimer.h"

#include "HKCommon.h"
#include "HKObject.h"
#include "./CharacterProxyMotion/HKStateMachine.h"

#include "ExampleApplication.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include "../res/resource.h"
#endif
#include <CEGUI.h>
#include <CEGUISystem.h>
#include <CEGUISchemeManager.h>
#include <OgreCEGUIRenderer.h>

#define HK_OBJECT_IS_LADDER 1234


class HKSpawnDescrip: public HKObjectDescrip {
public:
	HKSpawnDescrip():HKObjectDescrip(){
	};
	~HKSpawnDescrip(){
	};
};

//TODO:  Remove any type of 'up' vector.  Y will always be up.
//Predefines
class MyCharacterListener;
class hkpCharacterContext;
class hkpCharacterProxy;
class hkpShapePhantom;
class hkpShape;
class hkVector4;

class HKCharacterControllerDescrip : public HKObjectDescrip{
public:
	HKCharacterControllerDescrip();
	HKCharacterControllerDescrip(const HKCharacterControllerDescrip &d){
		Assign(d);
	};
	~HKCharacterControllerDescrip(){};

	void operator=(const HKCharacterControllerDescrip &d){ Assign(d); }

	void Assign(const HKCharacterControllerDescrip &d){
		_dTimeStep			= d._dTimeStep;
		_dInvTimeStep		= d._dInvTimeStep;

		_dMaxSpeed			= d._dMaxSpeed;
		_dGravitySpeed		= d._dGravitySpeed;

		_dCrouchHeight		= d._dCrouchHeight;
		_dStandHeight		= d._dStandHeight;
		
		_dGravity			= d._dGravity;
		
		_dRotationSpeedX	= d._dRotationSpeedX;
		_dRotationSpeedY	= d._dRotationSpeedY;

		_iUpAxis			= d._iUpAxis;
		_vPos				= d._vPos;
		_qOrient			= d._qOrient;
		
		_up					= d._up;

		_ogreCamera			= d._ogreCamera;
		_ogreEntity			= d._ogreEntity;
		_ogreScene			= d._ogreScene;
		_ogreCameraNode		= d._ogreCameraNode;
	}

	Ogre::Camera	*_ogreCamera;
	Ogre::SceneNode *_ogreCameraNode;

	double _dMaxSpeed;
	double _dGravitySpeed;
	double _dTimeStep;
	double _dInvTimeStep;

	double _dCrouchHeight;
	double _dStandHeight;
	
	double _dGravity;
	
	double _dRotationSpeedX;
	double _dRotationSpeedY;

	int _iUpAxis;

	hkVector4			*_up;	
};


class HKCharacterController : public HKObject {
public:
	HKCharacterController(const HKCharacterControllerDescrip &d);

	~HKCharacterController();
	
	virtual HKCharacterControllerDescrip GetDescription(){return _descrip;}
	virtual void SetDescription(const HKCharacterControllerDescrip &d){_descrip = d;}

	void SetCamera(Ogre::Camera *p){_descrip._ogreCamera = p;}

	virtual void Init();
	bool LoadIni(char *file);
	virtual void Forward(double dt);
	virtual void Backward(double dt);
	virtual void TurnLeft(double dt);
	virtual void StrafeLeft(double dt);
	virtual void TurnRight(double dt);
	virtual void StrafeRight(double dt);
	
	virtual void Jump(double dt);
	virtual void Crouch(double dt);

	virtual bool SerializeConstruction(RakNet::BitStream *bitStream, RakNet::SerializationContext *serializationContext);
	virtual bool Serialize(RakNet::BitStream *bitStream, RakNet::SerializationContext *serializationContext);
	virtual void Deserialize(RakNet::BitStream *bitStream, RakNet::SerializationType serializationType, SystemAddress sender, RakNetTime timestamp);

	void stepWorldAndController(const HKStateMachineOutput& output);

	virtual void SetMovementSpeed(double d);
	void SetFreeLook(bool b);
	bool GetFreeLook() { return _bFreeLook;}
	virtual void Step(double dt);

	virtual void swapPhantomShape( hkpShape* newShape );

	static HKCharacterController *Create();

private:
	HKCharacterControllerDescrip _descrip;


	MyCharacterListener*	_listener;
	hkpCharacterProxy*		_characterProxy;
	hkpCharacterContext*	_characterContext;
	hkpShapePhantom*		_phantom;
	hkpShape*				_standShape;
	hkpShape*				_crouchShape;
	
	// The context for this character in the animation state machine
	HKStateMachineContext _stateCharacterContext;

	// The state machine that govers the animation of the character
	HKStateMachine* _stateMachine;

	Ogre::Vector3 position;
	Ogre::Quaternion orientation;
	Ogre::Vector3 visiblePosition;
	Ogre::Quaternion visibleOrientation;
	
	bool _bOffAxis;
	double _dCaveRoll, _dCavePitch, _dCaveYaw;
	TransformationHistory transformationHistory;
	TransformationHistory2 transformationHistory2;
	bool _bCrouch, _bJump, _bFreeLook, _bEnableInterpolation;

	ProfTimer _timer;
	double _dLastPacketTime;
};


#endif //__HKCHARACTERCONTROLLER__