#ifndef __GOFFAXKIS_HEADER__
#define __GOFFAXKIS_HEADER__

#include "Singleton.h"


#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>

#include "ExampleApplication.h"

#pragma once
#define NUMBER_OF_DISPLAYS 4
class GOffAxisCamera : public HK::Singleton<GOffAxisCamera> {
	friend class HK::Singleton<GOffAxisCamera>;
protected:
	GOffAxisCamera();
	~GOffAxisCamera();
public:
	Ogre::SceneNode *GetUberNode(){return _uberNode;}
	bool LoadIni(char *file);
	void Start(){_bStart = true;}
	void EnableOffAxisProjection(Ogre::Camera *camera);
	void MoveCamera(Ogre::Vector3 rot, Ogre::Vector3 trans);

	void SetPosition(Ogre::Vector3 pos){
		_uberNode->setPosition(pos);	//Update uber node
		_dummyCam->setPosition(pos);	//Update dummy camera
	};
	Ogre::Vector3 GetPosition(){return _uberNode->getPosition();}
	
	void SetOrientation(Ogre::Quaternion o){_uberNode->setOrientation(o);}
	Ogre::Quaternion GetOrientation(){return _uberNode->getOrientation();}

	Ogre::Quaternion GetRealCameraOrientation(){return _dummyCam->getRealOrientation();}
	void SetMatrix(Ogre::Matrix4 m){_offAxis = m;}
	void SetCamera(Ogre::Camera *c){
		_camera = c;
		_uberNode->attachObject(_camera);
	}
	void Update();
private:
	double _dFOVLeft,_dFOVRight,_dFOVTop,_dFOVBottom;
	double _dCaveRoll, _dCavePitch, _dCaveYaw;
	double _dCaveFOV;
	bool _bStart;
	
	int _nDisplays;
	Ogre::Matrix4 _offAxis;
	Ogre::Camera	*_camera;
	Ogre::SceneNode *_uberNode;
	Ogre::Camera	*_dummyCam;

};
#endif // __GDEBUGGER_HEADER__