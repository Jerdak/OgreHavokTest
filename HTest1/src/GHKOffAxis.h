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
class GHKOffAxis : public HK::Singleton<GHKOffAxis> {
	friend class HK::Singleton<GHKOffAxis>;
protected:
	GHKOffAxis();
	~GHKOffAxis();
public:
	Ogre::SceneNode *GetUberNode(){return _uberNode;}
	bool LoadIni(char *file);
	void Start(){_bStart = true;}
	void EnableOffAxisProjection(Ogre::Camera *camera, int idx);
	void MoveCamera(Ogre::Vector3 rot, Ogre::Vector3 trans);
	void SetMatrix(int idx, Ogre::Matrix4 m){_offAxis[idx] = m;}
	void SetCamera(int idx, Ogre::Camera *c){_camera[idx] = c;}
	void Update();
private:
	double _dFOVLeft[NUMBER_OF_DISPLAYS],_dFOVRight[NUMBER_OF_DISPLAYS],_dFOVTop[NUMBER_OF_DISPLAYS],_dFOVBottom[NUMBER_OF_DISPLAYS];
	double _dCaveRoll[NUMBER_OF_DISPLAYS], _dCavePitch[NUMBER_OF_DISPLAYS], _dCaveYaw[NUMBER_OF_DISPLAYS];
	double _dCaveFOV[NUMBER_OF_DISPLAYS];

	bool _bStart;
	Ogre::Matrix4 _offAxis[NUMBER_OF_DISPLAYS];
	Ogre::Camera *_camera[NUMBER_OF_DISPLAYS];
	Ogre::SceneNode *_uberNode;
	Ogre::Camera *_playerCam;

};
#endif // __GDEBUGGER_HEADER__