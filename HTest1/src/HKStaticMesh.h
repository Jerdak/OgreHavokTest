#ifndef __HK_STATIC_MESH__
#define __HK_STATIC_MESH__

#include "HKObject.h"
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

class HKStaticMeshDescrip : public HKObjectDescrip {
public:
	HKStaticMeshDescrip(){
		_sID					= "OpenSkyMesh";
		_sMeshName			= "level_opensky.mesh";
		_sMaterialName		= "Examples/10PointBlock";
		_sMoppName			= "./Media/Tk/level.tk";

		_vDir				= Ogre::Vector3(1,0,0);
		_vSize				= Ogre::Vector3(1.0f,1.0f,1.0f);

		_vScale				= Ogre::Vector3(0.32f,0.32f,0.32f);
		_dRadius			= 0.8f;
		_iPhysType			= HKObjectDescrip::OBJ_PHYS_CUBE;

		_qOrientation		= Ogre::Quaternion();
		
		_ogreEntity = NULL;
		_ogreScene = NULL;

		_bBumpMapping = false;
	};
	HKStaticMeshDescrip(const HKStaticMeshDescrip &d){Assign(d);}
	~HKStaticMeshDescrip(){};

	void operator = (const HKStaticMeshDescrip &d){Assign(d);}

	void Assign(const HKStaticMeshDescrip &d){
		_iPhysType		= d._iPhysType;
		_vScale			= d._vScale;
		_dRadius		= d._dRadius;

		_ogreEntity		= d._ogreEntity;
		_ogreScene	= d._ogreScene;

		_sID				= d._sID;
		_sMeshName			= d._sMeshName;
		_sMaterialName		= d._sMaterialName;
		_sMoppName			= d._sMoppName;

		_vPos				= d._vPos;
		_vDir				= d._vDir;
		_vSize				= d._vSize;
		_bBumpMapping		= d._bBumpMapping;
		_qOrientation		= d._qOrientation;
	}

	Ogre::String  _sMeshName;
	Ogre::String  _sMaterialName;
	Ogre::String  _sMoppName;

	Ogre::Vector3 _vDir;
	Ogre::Vector3 _vSize;
	Ogre::Vector3 _vScale;
	Ogre::Quaternion _qOrientation;

	double		  _dRadius;
	bool		  _bBumpMapping;
};

class HKStaticMesh : public HKObject {
public:
	HKStaticMesh(const HKStaticMeshDescrip &d);
	~HKStaticMesh();


	void Create();
	void Step(double dt);
protected:
	HKStaticMeshDescrip _hkDescrip;

private:

};


#endif //__HK_STATIC_MESH__