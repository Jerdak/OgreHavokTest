#pragma once
#include <vector>
#include <string>

#include "DotSceneLoader.h"

#include "HKFactory.h"
#include "HKObject.h"
#include "HKPrimitive.h"
#include "HKCharacterController.h"
#include "HKStaticMesh.h"

#include "GHavok.h"
#include "GDebugger.h"

#include "ExampleApplication.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include "../res/resource.h"
#endif
#include <CEGUI.h>
#include <CEGUISystem.h>
#include <CEGUISchemeManager.h>
#include <OgreCEGUIRenderer.h>
#include <OgreSceneNode.h>

#define OIS_DYNAMIC_LIB
#include <OIS/OIS.h>


class HKLevelDescrip : public HKObjectDescrip {
public:
	HKLevelDescrip():HKObjectDescrip(){
		_ogreCamera = NULL;
	}
	HKLevelDescrip(const HKLevelDescrip &d){Assign(d);}

	~HKLevelDescrip(){};

	void Assign (const HKLevelDescrip &d){
		_ogreCamera = d._ogreCamera;
	}
	Ogre::Camera *_ogreCamera;
};
class HKLevel :
	public HKObject
{
public:
	HKLevel(const HKLevelDescrip &d);
	~HKLevel(void);

	void Load(Ogre::String in);

	void processUnbufferedKeyInput(OIS::Keyboard* mKeyboard,const FrameEvent& evt);
	void AddPrimitive(const HKPrimitiveDescrip& hk);
	void AddLight(/*const HKLightDescrip &hk*/);
	void AddStaticMesh(const HKStaticMeshDescrip &hk);
	void AddCharacterController(const HKCharacterControllerDescrip& hk);
	void AddSpawnPoint(const HKSpawnDescrip& hk);

	HKCharacterController *GetCharacterController(){return _hkCharController;}
private:
	std::vector<HKSpawnDescrip*>	_vSpawns;
	std::vector<HKPrimitive*>		_vPrimitives;
	std::vector<HKStaticMesh*>		_vStaticMeshes;

	std::vector<Ogre::Entity*> _vEntities;

	Ogre::Camera		*_ogreCamera;

	HKFactory *_hkFactory;
	HKCharacterController *_hkCharController;

	HKLevelDescrip _descrip;

	DotSceneLoader _sceneLoader;
};
