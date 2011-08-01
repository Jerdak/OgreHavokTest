#pragma once

#include "HKObject.h"
#include "GDebugger.h"
#include "GRakNet.h"
#include "ExampleApplication.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include "../res/resource.h"
#endif
#include <CEGUI.h>
#include <CEGUISystem.h>
#include <CEGUISchemeManager.h>
#include <OgreCEGUIRenderer.h>
#include <OgreSceneNode.h>

class HKPrimitiveDescrip : public HKActorDescrip {

public:
	enum PRIMITIVE_TYPES { PRIMITIVE_SPHERE, PRIMITIVE_CUBE, PRIMITIVE_PYRAMID, PRIMITIVE_END };
	HKPrimitiveDescrip():HKActorDescrip(){
		_sID				= "GenericPrimitve";
		_sMeshName			= "earth.mesh";
		_sMaterialName		= "Material/Earth";

		_vSize				= Ogre::Vector3(1.0f,1.0f,1.0f);

		_ogreCamera			= NULL;

		_vScale				= Ogre::Vector3(1.0f,1.0f,1.0f);
		_dRadius			= 0.8f;
		_iPrimitiveType		= HKPrimitiveDescrip::PRIMITIVE_SPHERE;
		_hkPenetrationDepth = 0.005f;
		_hkMass				= 10.0f;
	}
	HKPrimitiveDescrip(const HKPrimitiveDescrip &d){
		Assign(d);
	}
	
	~HKPrimitiveDescrip(){}
	
	void operator = (const HKPrimitiveDescrip& d){
		Assign(d);
	}
	void Assign (const HKPrimitiveDescrip& d){
		_sID				= d._sID;
		_sMeshName			= d._sMeshName;
		_sMaterialName		= d._sMaterialName;
		
		_ogreCamera			= d._ogreCamera;
		_ogreEntity			= d._ogreEntity;

		_vPos				= d._vPos;
		_qOrient			= d._qOrient;
		
		_vSize				= d._vSize;
		_vScale				= d._vScale;
		_dRadius			= d._dRadius;
		
		_iPhysType			= d._iPhysType;
		_iPrimitiveType		= d._iPrimitiveType;
		_hkPenetrationDepth = d._hkPenetrationDepth;
		_hkMass				= d._hkMass;
	}

	Ogre::String  _sMeshName;
	Ogre::String  _sMaterialName;

	Ogre::Camera	*_ogreCamera;

	Ogre::Vector3	_vSize;
	Ogre::Vector3	_vScale;
	double		  _dRadius;
	int			  _iPrimitiveType;

	double		  _hkPenetrationDepth;
	double		  _hkMass;
};
class HKPrimitive : public HKActor {
public:
					  
	HKPrimitive(const HKPrimitiveDescrip &d);
	~HKPrimitive();
	
	static HKPrimitive *Create();

	void Spawn();
	void Step(double dt);

	virtual bool SerializeConstruction(RakNet::BitStream *bitStream, RakNet::SerializationContext *serializationContext);
	virtual bool Serialize(RakNet::BitStream *bitStream, RakNet::SerializationContext *serializationContext);
	virtual void Deserialize(RakNet::BitStream *bitStream, RakNet::SerializationType serializationType, SystemAddress sender, RakNetTime timestamp);

protected:
	Ogre::Vector3		position;
	Ogre::Quaternion	orientation;
	Ogre::Vector3		visiblePosition;
	Ogre::Quaternion	visibleOrientation;
	
	
	TransformationHistory transformationHistory;
	TransformationHistory2 transformationHistory2;

	bool _bEnableInterpolation;
private:
	HKPrimitiveDescrip _descrip;
};