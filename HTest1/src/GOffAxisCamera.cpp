#include "GOffAxisCamera.h"
#include "SimpleIni.h"
#include "GDebugger.h"

GOffAxisCamera::GOffAxisCamera(){
	_bStart=false;

	try {
		Ogre::SceneManager *mgr  = Ogre::Root::getSingleton ().getSceneManager ("ExampleSMInstance"); 
		if(mgr == NULL){
			GDebugger::GetSingleton().WriteToLog("  ERROR in object constructor, could not find SceneManager\n");
		} else {
			GDebugger::GetSingleton().WriteToLog("  - SceneMgr found: %s\n",mgr->getName().c_str());
		}

		_uberNode = mgr->createSceneNode("UberNode");
		_uberNode->setPosition(Vector3(-10,15,0));

		_dummyCam = mgr->createCamera("DummyCamera");
		_dummyCam->lookAt(0,0,300);

		_dFOVLeft  = -45;
		_dFOVRight = 45;
		_dFOVTop = 33.69;
		_dFOVBottom = -33.69;
		_dCaveFOV = 90;
		_dCaveRoll = 0;
		_dCavePitch = 0;
		_dCaveYaw = 0;

	} catch (Ogre::Exception *ex) {
		GDebugger::GetSingleton().WriteToLog("  ERROR - Problem creating off axis singleton: %s\n",ex->getDescription().c_str());
	}
}

GOffAxisCamera::~GOffAxisCamera(){

}

//TODO:  Verify this code does nothing and remove.  Originally it was thought that the projection matrix needed to be updated per frame tick
//			but it seems this isn't true.
void GOffAxisCamera::Update(){
	return;
	if(!_bStart)return;
	_camera->setCustomProjectionMatrix(true, (/*leftDefault * */ _offAxis));
}
bool GOffAxisCamera::LoadIni(char *file)
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
			
		LOOKUP("FOV",				_dCaveFOV,DOUBLE)
		ELOOKUP("FOVLeft",			_dFOVLeft,DOUBLE)
		ELOOKUP("FOVRight",			_dFOVRight,DOUBLE)
		ELOOKUP("FOVTop",			_dFOVTop,DOUBLE)
		ELOOKUP("FOVBottom",		_dFOVBottom,DOUBLE)
		ELOOKUP("CaveRoll",			_dCaveRoll,DOUBLE)
		ELOOKUP("CavePitch",		_dCavePitch,DOUBLE)
		ELOOKUP("CaveYaw",			_dCaveYaw,DOUBLE)
	}
	return true;
}
/*
	EnableOffAxisProjection - Setup and enable off axis projection.  Original off axis matrix courtesy of Willem De Jonge.  
								This function will take the input camera and attach it to this global camera class.
*/
void GOffAxisCamera::EnableOffAxisProjection(Ogre::Camera *camera){
	try {
		double pi = 3.14159;
		double dCaveFOVy = 2*atan( tan(_dCaveFOV/2 * pi/180) / camera->getAspectRatio() )*180/pi;
		camera->setFOVy(Degree(dCaveFOVy));
		
		camera->setCustomProjectionMatrix(false);
		double n = camera->getNearClipDistance();
		double f = camera->getFarClipDistance();
		double l, r, b, t;


		l = n*tan(_dFOVLeft*pi/180);
		r = n*tan(_dFOVRight*pi/180);
		b = n*tan(_dFOVBottom*pi/180);
		t = n*tan(_dFOVTop*pi/180);

		//Small modification from original off-axis algorithm.  Signs of certain vars were changed so the Y rotations aren't flipped.
		//This also fixes the problem of translations moving in the incorrect direction.
		_offAxis = Ogre::Matrix4(		(2.0f*n)/(r-l), 0,				((r+l)/(r-l)),	0,
										0,				((2.0f*n)/(t-b)), ((t+b)/(t-b)),	0,
										0,				0,				-(f+n)/(f-n),	-(2.0f*f*n)/(f-n),
										0,				0,				-1,				0);
		_offAxis.transpose();
		camera->setCustomProjectionMatrix(true, _offAxis);
		
		//TODO:  Support pitch and roll at some point, for now yaw is the only important value.
		camera->yaw(Degree(_dCaveYaw));

		_camera = camera;
		_uberNode->attachObject(camera);
	} catch (Ogre::Exception *ex){
		GDebugger::GetSingleton().WriteToLog("  ERROR - Trouble creating off axis projection: %s\n",ex->getDescription().c_str());
	}
}

//TODO:  Alternatively I think we should just attach the dummy camera to the uber node.  Then we can skip the rotation update of the dummy camera all together.
void GOffAxisCamera::MoveCamera(Ogre::Vector3 rot, Ogre::Vector3 trans){
	if(!_bStart)return;
	try {
		//Rotate the uber camera node which in turn rotates all sub cameras
		_uberNode->pitch(Ogre::Radian(rot.y) * -1, Node::TS_LOCAL);
		_uberNode->yaw(Ogre::Radian(rot.z), Node::TS_WORLD);
	
		//Rotate dummy camera.  The dummy camera does not display anything on the screen.  What it does is take the same rotations that would be
		//applied to the uber node and it points along the same direction.  We can then use 'MoveRelative' of the dummy camera to get movement along the uber node's
		//line of sight.
		_dummyCam->roll(Ogre::Radian(rot.x));
		_dummyCam->pitch(Ogre::Radian(rot.y));
		_dummyCam->yaw(Ogre::Radian(rot.z));
		_dummyCam->moveRelative(trans);

		//Adjust the uber node accordingly.
		_uberNode->setPosition(_dummyCam->getPosition());
	} catch (Ogre::Exception *ex){
		GDebugger::GetSingleton().WriteToLog("  ERROR - Trouble moving off axis projection: %s\n",ex->getDescription().c_str());
	}
}