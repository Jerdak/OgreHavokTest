#include "HKStaticMesh.h"
#include "GameUtils.h"

#include <Common/Base/hkBase.h>
#include <Common/Base/Monitor/hkMonitorStream.h>
#include <Common/Visualize/hkDebugDisplay.h>

#include <Demos/Physics/UseCase/CharacterControl/CharacterProxy/CharacterController/CharacterControllerDemo.h>

#include <Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxy.h>
#include <Physics/Utilities/CharacterControl/StateMachine/hkpDefaultCharacterStates.h>

#include <Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h>
#include <Physics/Dynamics/Phantom/hkpSimpleShapePhantom.h>
#include <Physics/Collide/Query/Collector/PointCollector/hkpClosestCdPointCollector.h>
#include <Physics/Collide/Query/Collector/PointCollector/hkpAllCdPointCollector.h>
#include <Physics/ConstraintSolver/Simplex/hkpSimplexSolver.h>
#include <Physics/Collide/Query/CastUtil/hkpLinearCastInput.h>

#include "GameUtils.h"
#include <Demos/DemoCommon/Utilities/Character/CharacterUtils.h>
//#include <hkdemoframework/hkDemoFramework.h>

#include <Demos/DemoCommon/DemoFramework/hkTextDisplay.h>

#include <Graphics/Common/Input/Pad/hkgPad.h>
#include <Graphics/Common/Window/hkgWindow.h>


HKStaticMesh::HKStaticMesh(const HKStaticMeshDescrip &d):HKObject(d){
	_hkDescrip = d;
}

HKStaticMesh::~HKStaticMesh(){
}

void HKStaticMesh::Create(){

	if(_sceneMgr==NULL){
		GDebugger::GetSingleton().WriteToLog("ERROR:  No scene manager set for HKStaticMesh: %s\n",_hkDescrip._sID);
	}
	Ogre::Vector3 pos = _hkDescrip._vPos;

	//TODO:  Actually input bump mapping.  :)
	if(_hkDescrip._bBumpMapping){
	/*	MeshPtr pMesh = MeshManager::getSingleton().load("level.mesh",
                ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,    
                HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY, 
				HardwareBuffer::HBU_STATIC_WRITE_ONLY, 
				true, true); //so we can still read it

        // Build tangent vectors, all our meshes use only 1 texture coordset 
		// Note we can build into VES_TANGENT now (SM2+)
        unsigned short src, dest;
        if (!pMesh->suggestTangentVectorBuildParams(VES_TANGENT, src, dest))
        {
            pMesh->buildTangentVectors(VES_TANGENT, src, dest);
        }*/
        // Create entity
		

		if(_hkDescrip._ogreScene==NULL){
			//Allow outside classes the ability to set the ogre entity.
			if(_hkDescrip._ogreEntity==NULL){
				GDebugger::GetSingleton().WriteToLog("Outputting new entity: %s\n",_hkDescrip._sID.c_str());
				_hkDescrip._ogreEntity		= _sceneMgr->createEntity(_hkDescrip._sID + "_entity",_hkDescrip._sMeshName); 
				_hkDescrip._ogreScene	= _sceneMgr->getRootSceneNode()->createChildSceneNode(_hkDescrip._sID + "_scene");
			}
			_hkDescrip._ogreEntity->setMaterialName(_hkDescrip._sMaterialName);
			// Attach to child of root node
			{	
				_hkDescrip._ogreScene->attachObject( _hkDescrip._ogreEntity );
				_hkDescrip._ogreScene->setOrientation(_hkDescrip._qOrientation);
				_hkDescrip._ogreScene->scale(_hkDescrip._vScale);
				_hkDescrip._ogreScene->setPosition(pos.x,pos.y,pos.z);
			}
		}

		
		/*
		SceneNode *sc2 = mSceneMgr->getRootSceneNode()->createChildSceneNode("Skydome_Scene");
		Entity *skydome = mSceneMgr->createEntity("Skydome_Entity","skydome.mesh");
		skydome->setMaterialName("Material/SOLID/TEX/skydome.jpg");
		skydome->setNormaliseNormals(false);
		skydome->setCastShadows(false);
		
		// Attach to child of root node
		sc2->attachObject( skydome );

		
		Ogre::Quaternion q2(Degree(-90),Vector3::UNIT_Y);
		sc2->setOrientation(q2);
		sc2->setPosition(0,0,450);
		sc2->scale(Vector3::UNIT_SCALE * 205.32);*/
	} else {
		/*Entity *platform = mSceneMgr->createEntity("Platform_Entity","room.mesh");
		//platform->setMaterialName("Examples/10PointBlock");
		platform->setNormaliseNormals(true);
		platform->setCastShadows(true);
		
		SceneNode *sc = mSceneMgr->getRootSceneNode()->createChildSceneNode("Platform_Scene");
		sc->attachObject( platform );
		sc->setPosition(0,0,0);
		Ogre::Quaternion q(Degree(-90),Vector3::UNIT_X);
		sc->setOrientation(q);
		sc->scale(Vector3::UNIT_SCALE * 0.32);*/
	}

	//Apply physics type.  Right now that's simply Havok's MOPP type. (All tri's in mesh are collidable.)
	if(_hkDescrip._iPhysType == HKObjectDescrip::OBJ_PHYS_MOPP){
		// Load the level
		{
			hkVector4 tkScaleFactor(_hkDescrip._vScale.x,_hkDescrip._vScale.y,_hkDescrip._vScale.z);
		
			hkpShape* moppShape = GameUtils::loadTK2MOPP(_hkDescrip._sMoppName.c_str(),tkScaleFactor, -1.0f);
			HK_ASSERT2(0x64232cc0, moppShape,"TK file failed to load to MOPP in GameUtils::loadTK2MOPP.");

			hkpRigidBodyCinfo ci;
			ci.m_shape = moppShape;
			ci.m_motionType = hkpMotion::MOTION_FIXED;
			ci.m_collisionFilterInfo = hkpGroupFilter::calcFilterInfo( 0, 1 );
			//ci.m_position = hkVector4(pos.x,pos.y,pos.z);		//TODO:  Add ability for physics mesh to be positioned independent of ogre mesh.

			_hkDescrip._hkRigid = new hkpRigidBody(ci);
			moppShape->removeReference();

			GHavok::GetSingleton().AddEntity(_hkDescrip._hkRigid);
			_hkDescrip._hkRigid->removeReference();
		}
	} else {
		GDebugger::GetSingleton().WriteToLog("ERROR:  StaticMesh[%s] physics type does not exist...",_hkDescrip._sID.c_str());
	}
}

void HKStaticMesh::Step(double dt){
}