#include "HKObject.h"


// Math and base include
#include <Common/Base/hkBase.h>
#include <Physics/Dynamics/Entity/hkpRigidBody.h>

HKObjectDescrip::~HKObjectDescrip(){
	if(_ogreEntity){delete _ogreEntity;	_ogreEntity = NULL;}
	if(_ogreScene){delete _ogreScene;				_ogreScene	= NULL;}
	if(_hkRigid){_hkRigid->removeReference();	_hkRigid	= NULL;}
}