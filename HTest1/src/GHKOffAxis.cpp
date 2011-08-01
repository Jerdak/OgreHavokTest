#include "GHKOffAxis.h"
#include "SimpleIni.h"
#include "GDebugger.h"

GHKOffAxis::GHKOffAxis(){
	_bStart=false;

	try {
		Ogre::SceneManager *mgr  = Ogre::Root::getSingleton ().getSceneManager ("ExampleSMInstance"); 
		if(mgr == NULL){
			GDebugger::GetSingleton().WriteToLog("  ERROR in object constructor, could not find SceneManager\n");
		} else {
			GDebugger::GetSingleton().WriteToLog("  - SceneMgr found: %s\n",mgr->getName().c_str());
		}

		_uberNode = mgr->createSceneNode("UberNode");
		_uberNode->setPosition(Vector3(-10,0,0));
		_playerCam = mgr->createCamera("MainPlayerCamera");
		_playerCam->lookAt(0,0,300);
		for(int i = 0; i < NUMBER_OF_DISPLAYS; i++){
			_dFOVLeft[i]  = 0;
			_dFOVRight[i] = 0;
			_dFOVTop[i] = 0;
			_dFOVBottom[i] = 0;
			_dCaveFOV[i] = 0;
			_dCaveRoll[i] = 0;
			_dCavePitch[i] = 0;
			_dCaveYaw[i] = 0;
		}
	} catch (Ogre::Exception *ex) {
		GDebugger::GetSingleton().WriteToLog("  ERROR - Problem creating off axis singleton: %s\n",ex->getDescription().c_str());
	}
}

GHKOffAxis::~GHKOffAxis(){

}

void GHKOffAxis::Update(){
	return;
	if(!_bStart)return;
//	_camera[0]->setCustomProjectionMatrix(false);   // Let OGRE recalculate the projection matrix for the leftCamera
//	_camera[1]->setCustomProjectionMatrix(false);  // Let OGRE recalculate the projection matrix for the rightCamera

//	Matrix4 leftDefault = _camera[0]->getProjectionMatrix();        // Get OGRE's default projection matrix for the leftCamera
//	Matrix4 rightDefault = _camera[1]->getProjectionMatrix();      // Get OGRE's default projection matrix for the rightCamera

	for(int i = 0; i < NUMBER_OF_DISPLAYS; i++){
		_camera[i]->setCustomProjectionMatrix(true, (/*leftDefault * */ _offAxis[i]));
	}

}
bool GHKOffAxis::LoadIni(char *file)
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
	ini.GetAllKeys("Viewport", keys);
	if(keys.size() <= 0) {
		printf("Viewport section not found.\n");
		return false;
	}
	CSimpleIniCaseA::TNamesDepend::const_iterator i; // list iterator
	for(i=keys.begin(); i!=keys.end(); ++i) {
		const char* pszValue = ini.GetValue("Viewport",i->pItem, NULL);
			
		LOOKUP("FOV",				_dCaveFOV[0],DOUBLE)
		ELOOKUP("FOV2",				_dCaveFOV[1],DOUBLE)
		ELOOKUP("FOV3",				_dCaveFOV[2],DOUBLE)
		ELOOKUP("FOV4",				_dCaveFOV[3],DOUBLE)
		ELOOKUP("FOVLeft",			_dFOVLeft[0],DOUBLE)
		ELOOKUP("FOVRight",			_dFOVRight[0],DOUBLE)
		ELOOKUP("FOVTop",			_dFOVTop[0],DOUBLE)
		ELOOKUP("FOVBottom",		_dFOVBottom[0],DOUBLE)
		ELOOKUP("CaveRoll",			_dCaveRoll[0],DOUBLE)
		ELOOKUP("CavePitch",		_dCavePitch[0],DOUBLE)
		ELOOKUP("CaveYaw",			_dCaveYaw[0],DOUBLE)
		ELOOKUP("FOVLeft1",			_dFOVLeft[1],DOUBLE)
		ELOOKUP("FOVRight1",		_dFOVRight[1],DOUBLE)
		ELOOKUP("FOVTop1",			_dFOVTop[1],DOUBLE)
		ELOOKUP("FOVBottom1",		_dFOVBottom[1],DOUBLE)
		ELOOKUP("CaveRoll1",		_dCaveRoll[1],DOUBLE)
		ELOOKUP("CavePitch1",		_dCavePitch[1],DOUBLE)
		ELOOKUP("CaveYaw1",			_dCaveYaw[1],DOUBLE)
		ELOOKUP("FOVLeft2",			_dFOVLeft[2],DOUBLE)
		ELOOKUP("FOVRight2",		_dFOVRight[2],DOUBLE)
		ELOOKUP("FOVTop2",			_dFOVTop[2],DOUBLE)
		ELOOKUP("FOVBottom2",		_dFOVBottom[2],DOUBLE)
		ELOOKUP("CaveRoll2",		_dCaveRoll[2],DOUBLE)
		ELOOKUP("CavePitch2",		_dCavePitch[2],DOUBLE)
		ELOOKUP("CaveYaw2",			_dCaveYaw[2],DOUBLE)
		ELOOKUP("FOVLeft3",			_dFOVLeft[3],DOUBLE)
		ELOOKUP("FOVRight3",		_dFOVRight[3],DOUBLE)
		ELOOKUP("FOVTop3",			_dFOVTop[3],DOUBLE)
		ELOOKUP("FOVBottom3",		_dFOVBottom[3],DOUBLE)
		ELOOKUP("CaveRoll3",		_dCaveRoll[3],DOUBLE)
		ELOOKUP("CavePitch3",		_dCavePitch[3],DOUBLE)
		ELOOKUP("CaveYaw3",			_dCaveYaw[3],DOUBLE)
	}
	return true;
}

void GHKOffAxis::EnableOffAxisProjection(Ogre::Camera *camera, int idx){
	try {
		double pi = 3.14159;
		double dCaveFOVy = 2*atan( tan(_dCaveFOV[idx]/2 * pi/180) / camera->getAspectRatio() )*180/pi;
		camera->setFOVy(Degree(dCaveFOVy));
		
		camera->setCustomProjectionMatrix(false);
		double n = camera->getNearClipDistance();
		double f = camera->getFarClipDistance();
		double l, r, b, t;


		l = n*tan(_dFOVLeft[idx]*pi/180);
		r = n*tan(_dFOVRight[idx]*pi/180);
		b = n*tan(_dFOVBottom[idx]*pi/180);
		t = n*tan(_dFOVTop[idx]*pi/180);

	
		int sign = 1;
		bool bSwapZ = false;

		if(bSwapZ)sign = -1;
		else sign = 1;

		//Small modification from original off-axis algorithm.  Signs of certain vars were changed so the Y rotations aren't flipped.
		//This also fixes the problem of translations moving in the incorrect direction.
		_offAxis[idx] = Ogre::Matrix4(	(2.0f*n)/(r-l), 0,					((r+l)/(r-l)) * sign,	0,
										0,				((2.0f*n)/(t-b)),	((t+b)/(t-b)) * sign,	0,
										0,				0,					-(f+n)/(f-n) * sign,	-(2.0f*f*n)/(f-n),
										0,				0,					-1* sign,				0);
		//_offAxis[idx].transpose();
		camera->setCustomProjectionMatrix(true, _offAxis[idx]);
		camera->yaw(Degree(_dCaveYaw[idx]));

		_camera[idx] = camera;
		_uberNode->attachObject(camera);
	} catch (Ogre::Exception *ex){
		GDebugger::GetSingleton().WriteToLog("  ERROR - Trouble creating off axis projection: %s\n",ex->getDescription().c_str());
	}
	//cameranode->yaw(Degree(_dCaveYaw[idx]), Node::TS_WORLD);
	//cameranode->pitch(Degree(_dCavePitch[idx]), Node::TS_WORLD);
	//cameranode->roll(Degree(_dCaveRoll[idx]), Node::TS_WORLD);
}

void GHKOffAxis::MoveCamera(Ogre::Vector3 rot, Ogre::Vector3 trans){
	if(!_bStart)return;
	try {
		//for(int i = 0; i < NUMBER_OF_DISPLAYS; i++){
		//	_camera[i]->roll(Ogre::Radian(rot.x));
		//	_camera[i]->pitch(Ogre::Radian(rot.y));
		//	_camera[i]->yaw(Ogre::Radian(rot.z));
		//}
		//for(int i = 0; i < NUMBER_OF_DISPLAYS; i++){
			//_uberNode->roll(Ogre::Radian(rot.x), Node::TS_WORLD);
			_uberNode->pitch(Ogre::Radian(rot.y) * -1, Node::TS_LOCAL);
			_uberNode->yaw(Ogre::Radian(rot.z), Node::TS_WORLD);
			
			_playerCam->roll(Ogre::Radian(rot.x));
			_playerCam->pitch(Ogre::Radian(rot.y));
			_playerCam->yaw(Ogre::Radian(rot.z));
			
			_playerCam->moveRelative(trans);
			_uberNode->setPosition(_playerCam->getPosition());
		//}
	} catch (Ogre::Exception *ex){
		GDebugger::GetSingleton().WriteToLog("  ERROR - Trouble moving off axis projection: %s\n",ex->getDescription().c_str());
	}
}