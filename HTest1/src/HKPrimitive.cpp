#include "HKPrimitive.h"

// Math and base include
#include <Common/Base/hkBase.h>
#include <Common/Base/System/hkBaseSystem.h>
#include <Common/Base/Memory/hkThreadMemory.h>
#include <Common/Base/Memory/Memory/Pool/hkPoolMemory.h>
#include <Common/Base/System/Error/hkDefaultError.h>
#include <Common/Base/Monitor/hkMonitorStream.h>

// Dynamics includes
#include <Physics/Collide/hkpCollide.h>
#include <Physics/Collide/Agent/ConvexAgent/SphereBox/hkpSphereBoxAgent.h>
#include <Physics/Collide/Shape/Convex/Box/hkpBoxShape.h>
#include <Physics/Collide/Shape/Convex/Sphere/hkpSphereShape.h>
#include <Physics/Collide/Dispatch/hkpAgentRegisterUtil.h>


#include <Physics/Dynamics/World/hkpWorld.h>
#include <Physics/Dynamics/Entity/hkpRigidBody.h>
#include <Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h>

#include <Physics/Utilities/Thread/Multithreading/hkpMultithreadingUtil.h>

using namespace RakNet;

HKPrimitive::HKPrimitive(const HKPrimitiveDescrip &d): HKActor(d)
{
	_descrip = d;
}

HKPrimitive::~HKPrimitive(){
}

static int _gid = 0;
HKPrimitive *HKPrimitive::Create(){
	GDebugger::GetSingleton().WriteToLog("  - Creating Primitive --\n");

	HKPrimitive *c = NULL;
	
	char buffer[256];
	sprintf(buffer,"primitive_%d",_gid++);
	

	//Create new character controller descriptor
	HKPrimitiveDescrip tmpDes;
	{
		try {
			tmpDes._ogreCamera = Ogre::Root::getSingleton().getSceneManager("ExampleSMInstance")->getCamera("PlayerCam");
			tmpDes._sID = "primitive_" + _gid;

			Vector3 spawnPos  = tmpDes._ogreCamera->getPosition();

			tmpDes._hkMass = 20.0f;
			tmpDes._vPos = Vector3(spawnPos.x,spawnPos.y+10,spawnPos.z);
			tmpDes._vScale = Vector3(0.1f,0.1f,0.1f);
			tmpDes._sMeshName = "crate.mesh";
			tmpDes._sMaterialName = "Material/SOLID/Crate";
			tmpDes._iPhysType			= HKObjectDescrip::OBJ_PHYS_CUBE;
			tmpDes._iPrimitiveType		= HKPrimitiveDescrip::PRIMITIVE_CUBE;
		} catch (Exception *ep){
			GDebugger::GetSingleton().WriteToLog("  - ERROR:  Crash during primitive spawn: %s\n",ep->getDescription().c_str());
		}
	}
	GDebugger::GetSingleton().WriteToLog("     - Descriptor filled\n");
	
	//Create new character controller
	{
		try {
			c = new HKPrimitive(tmpDes);
			c->Spawn();
			c->SetReplicaManager(GRakNet::GetSingleton().GetReplicaManager("primitive")); //Find network replication controller

			//Originally I had RakNet update this class but Havok uses locks to make the physics work properly, without
			//these the physics don't work right.  GHavok Step should only be called once so I added the update tick to it.

			//GRakNet::GetSingleton().AddObject(c);
			GHavok::GetSingleton().AddObject(c);	
		} catch (Exception *ep){
			GDebugger::GetSingleton().WriteToLog("  - ERROR:  Crash during primitive spawn: %s\n",ep->getDescription().c_str());
		}
		GDebugger::GetSingleton().WriteToLog("     - Object Created\n");
	}
	
	//If this is the server be sure to automatically broadcast object creation to the client.
	if(GRakNet::GetSingleton().IsServer()){
		
		GDebugger::GetSingleton().WriteToLog("     - Object Spawned\n");
		c->AddAutoSerializeTimer(GRakNet::GetSingleton().GetTimeDelay()  );
		// All connected systems should create this object
	
		
		c->BroadcastConstruction();
		// Force the first serialize to go out. We could have also just wrote the data in SerializeConstruction()
		// Without this call, no serialize would occur until something changed from the intial value

		c->BroadcastSerialize();
	}

	return c;
}

void HKPrimitive::Spawn(){
	if(GRakNet::GetSingleton().IsServer()){
		//Assign new descriptor
		
		if(_sceneMgr==NULL){
			GDebugger::GetSingleton().WriteToLog("  - ERROR:  Primitive trying to spawn with no scene manager\n");
		}

		// manually create entity.  TODO:  Entities should only be loaded once.  Afterwards we can apply them to the scene.  This will be done
		// in some sort of resource manager, probably just the Ogre::ResourceManager class.
		if(_descrip._ogreEntity == NULL){
			_descrip._ogreEntity = _sceneMgr->createEntity( _descrip._sID + "_Entity", _descrip._sMeshName );
			_descrip._ogreEntity->setMaterialName(_descrip._sMaterialName);
			_descrip._ogreEntity->setNormaliseNormals(true);
			_descrip._ogreEntity->setCastShadows(true);

			_descrip._ogreEntity->setUserAny(Ogre::Any((HKActor*)this));
		}
		_descrip._ogreScene = _sceneMgr->getRootSceneNode()->createChildSceneNode( _descrip._sID + "_SceneNode" );
		_descrip._ogreScene->attachObject( _descrip._ogreEntity );
		_descrip._ogreScene->setPosition(_descrip._vPos);
		_descrip._ogreScene->setScale(_descrip._vScale.x,_descrip._vScale.y,_descrip._vScale.z);

		GHavok::GetSingleton().MarkForWrite();

		//Determine physics shape type.
		if(_descrip._iPhysType == HKObjectDescrip::OBJ_PHYS_SPHERE){
			hkReal sphereRadius = _descrip._dRadius;
		
			hkpConvexShape* sphereShape = new hkpSphereShape(sphereRadius);
			{
				hkpRigidBodyCinfo si;
				si.m_shape = sphereShape;
				si.m_position.set(_descrip._vPos.x,_descrip._vPos.y,_descrip._vPos.z);
				si.m_motionType = hkpMotion::MOTION_SPHERE_INERTIA;
				si.m_allowedPenetrationDepth = _descrip._hkPenetrationDepth;

				hkpInertiaTensorComputer::setShapeVolumeMassProperties(sphereShape, 1.0f, si);
				
				// Compute mass properties
				hkReal sphereMass = _descrip._hkMass;
				hkpMassProperties sphereMassProperties;
				hkpInertiaTensorComputer::computeSphereVolumeMassProperties(sphereRadius, sphereMass, sphereMassProperties);
				si.m_inertiaTensor = sphereMassProperties.m_inertiaTensor;
				si.m_centerOfMass = sphereMassProperties.m_centerOfMass;
				si.m_mass = sphereMassProperties.m_mass;
				

				// Create sphere RigidBody
				_descrip._hkRigid = new hkpRigidBody(si);
				_descrip._hkRigid->setQualityType(HK_COLLIDABLE_QUALITY_MOVING);
				GHavok::GetSingleton().AddEntity(_descrip._hkRigid);
			}
			sphereShape->removeReference();
		} else if(_descrip._iPhysType == HKObjectDescrip::OBJ_PHYS_CUBE){
			hkVector4 halfSize(_descrip._vSize.x/2.0f, _descrip._vSize.y/2.0f, _descrip._vSize.z/2.0f);
		
			hkpConvexShape* boxShape = new hkpBoxShape(halfSize,0);
			{
				hkpRigidBodyCinfo si;
				si.m_shape = boxShape;
				si.m_position.set(_descrip._vPos.x,_descrip._vPos.y,_descrip._vPos.z);
				si.m_motionType = hkpMotion::MOTION_BOX_INERTIA;
				si.m_allowedPenetrationDepth = _descrip._hkPenetrationDepth;

				hkpInertiaTensorComputer::setShapeVolumeMassProperties(boxShape, 1.0f, si);
				
				// Compute mass properties
				hkReal boxMass = _descrip._hkMass;
				hkpMassProperties boxMassProperties;
				hkpInertiaTensorComputer::computeBoxVolumeMassProperties(halfSize,boxMass,boxMassProperties);
				si.m_inertiaTensor = boxMassProperties.m_inertiaTensor;
				si.m_centerOfMass = boxMassProperties.m_centerOfMass;
				si.m_mass = boxMassProperties.m_mass;
				

				// Create box RigidBody
				_descrip._hkRigid = new hkpRigidBody(si);
				_descrip._hkRigid->setQualityType(HK_COLLIDABLE_QUALITY_MOVING);
				GHavok::GetSingleton().AddEntity(_descrip._hkRigid);
			}
			boxShape->removeReference();
		}
		GHavok::GetSingleton().UnmarkForWrite();
	} else {

		_descrip._ogreEntity = _sceneMgr->createEntity( _descrip._sID + "_Entity", _descrip._sMeshName );
		_descrip._ogreEntity->setMaterialName(_descrip._sMaterialName);
		_descrip._ogreEntity->setNormaliseNormals(true);
		_descrip._ogreEntity->setCastShadows(true);

		_descrip._ogreEntity->setUserAny(Ogre::Any((HKActor*)this));
		
		_descrip._ogreScene = _sceneMgr->getRootSceneNode()->createChildSceneNode( _descrip._sID + "_SceneNode" );
		_descrip._ogreScene->attachObject( _descrip._ogreEntity );
		_descrip._ogreScene->setPosition(_descrip._vPos);
		_descrip._ogreScene->setScale(_descrip._vScale.x,_descrip._vScale.y,_descrip._vScale.z);
	}

	_bEnableInterpolation = true;
	transformationHistory.Init(30,1000);
}
void HKPrimitive::Step(double dt){
	visiblePosition		=	position;
	visibleOrientation	=	orientation;
	
	
	if(GRakNet::GetSingleton().IsServer()){
		//Check for grabbed object
		if(!_bGrab){
			hkVector4 pos = _descrip._hkRigid->getPosition();
			hkQuaternion q = _descrip._hkRigid->getRotation();
			Ogre::Quaternion qOgre;

			qOgre.x = q.m_vec(0);
			qOgre.y = q.m_vec(1);
			qOgre.z = q.m_vec(2);
			qOgre.w = q.m_vec(3);
			
				
				//Apply havok object position
			_descrip._ogreScene->setPosition(pos(0),pos(1),pos(2));
			_descrip._ogreScene->setOrientation(qOgre);

			position	= _descrip._ogreScene->getPosition();
			orientation = _descrip._ogreScene->getOrientation();
		} else {
		
			hkVector4 hkpos = _descrip._hkRigid->getPosition();
			hkQuaternion q = _descrip._hkRigid->getRotation();
			
			Ogre::Quaternion qOgre;
			Ogre::Vector3 ogpos = _descrip._ogreScene->getPosition();

			_descrip._hkRigid->setPosition(hkVector4(ogpos.x,ogpos.y,ogpos.z));
			position	= _descrip._ogreScene->getPosition();
		}
	} else {
		// interpolate visible position, lagging behind by a small amount so where know where to update to
		if (_bEnableInterpolation)
		{
			//GDebugger::GetSingleton().WriteToLog("   - Interpolate1: %f %f %f\n",visiblePosition.x,visiblePosition.y,visiblePosition.z);
			// Important: the first 3 parameters are in/out parameters, so set their values to the known current values before calling Read()
			// We are subtracting DEFAULT_SERVER_MILLISECONDS_BETWEEN_UPDATES from the current time to get an interpolated position in the past
			// Without this we wouldn't have a node to interpolate to, and wouldn't know where to go
			//transformationHistory.Read(&visiblePosition, 0, &visibleOrientation, RakNet::GetTime()-GRakNet::GetSingleton().GetTimeDelay(),RakNet::GetTime());
			visiblePosition = transformationHistory2.Read(dt,visiblePosition);
			//GDebugger::GetSingleton().WriteToLog("   - Interpolate2: %f %f %f\n",visiblePosition.x,visiblePosition.y,visiblePosition.z);
		}
		position = visiblePosition;


		_descrip._ogreScene->setPosition(visiblePosition);
		_descrip._ogreScene->setOrientation(visibleOrientation);
	}
}

bool HKPrimitive::SerializeConstruction(RakNet::BitStream *bitStream, SerializationContext *serializationContext)
{
	RakAssert(GRakNet::GetSingleton().IsServer()==true);
	StringTable::Instance()->EncodeString("primitive", 128, bitStream);
	return true;
}

bool HKPrimitive::Serialize(RakNet::BitStream *bitStream, SerializationContext *serializationContext)
{
	// Autoserialize causes a network packet to go out when any of these member variables change.
	RakAssert(GRakNet::GetSingleton().IsServer()==true);
	bitStream->Write(true);
	bitStream->WriteAlignedBytes((const unsigned char*)&position,sizeof(position));
	bitStream->WriteAlignedBytes((const unsigned char*)&orientation,sizeof(orientation));
	//GDebugger::GetSingleton().WriteToLog("Writing data\n");
	return true;
}

void HKPrimitive::Deserialize(RakNet::BitStream *bitStream, SerializationType serializationType, SystemAddress sender, RakNetTime timestamp)
{
	// Doing this because we are also lagging position and orientation behind by DEFAULT_SERVER_MILLISECONDS_BETWEEN_UPDATES
	// Without it, the kernel would pop immediately but would not start moving
	bool bRead = true;
	bitStream->Read(bRead);
	bitStream->ReadAlignedBytes((unsigned char*)&position,sizeof(position));
	bitStream->ReadAlignedBytes((unsigned char*)&orientation,sizeof(orientation));

	// Scene node starts invisible until we deserialize the intial startup data
	// This data could also have been passed in SerializeConstruction()
//		sceneNode->setVisible(true,true);

	// Every time we get a network packet, we write it to the transformation history class.
	// This class, given a time in the past, can then return to us an interpolated position of where we should be in at that time
	transformationHistory.Write(position,Vector3(0,0,0),orientation,RakNet::GetTime());
	transformationHistory2.Write(position);
	//GDebugger::GetSingleton().WriteToLog("Readin data\n");
}