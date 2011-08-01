#include "HKLevel.h"

#include <Common/Base/hkBase.h>

HKLevel::HKLevel(const HKLevelDescrip &d):HKObject(d){
	HKFactoryDescrip tmp;
	_hkFactory = new HKFactory(tmp);
	GHavok::GetSingleton().AddObject(_hkFactory);
	_descrip = d;
}

HKLevel::~HKLevel(){

}

void HKLevel::Load(Ogre::String in){
	GDebugger::GetSingleton().WriteToLog("  - HKLevel: Loading level\n");
	//Create factory
	{
		GHavok::GetSingleton().AddObject(_hkFactory);
	}
	GDebugger::GetSingleton().WriteToLog("     - Object factory created\n");

	if(GRakNet::GetSingleton().IsServer()){
		//Default scene adjustments
		{
			_sceneMgr->setSkyDome(true,"Examples/CloudySky",5,8);
		}
		GDebugger::GetSingleton().WriteToLog("     - Skydome created\n");

		//Load character controller
		{
			_hkCharController = HKCharacterController::Create();
			//AddCharacterController(tmpDes);
		}
		GDebugger::GetSingleton().WriteToLog("     - Character Controller added\n");

		//Load static meshes
		{
			SceneNode *level = NULL;
			HKStaticMeshDescrip des;
			des._ogreScene = level;
			des._iPhysType = HKObjectDescrip::OBJ_PHYS_MOPP;
			des._sMoppName = "./Media/ArabLevel/ArabLevel.tk";
			des._vScale= Vector3(1,1,1);
			_sceneLoader.parseDotScene("./Media/ArabLevel/ArabLevel2.scene","General",_sceneMgr,level);
			AddStaticMesh(des);

		/*
			//Level mesh
			HKStaticMeshDescrip des;
			des._bBumpMapping = true;
			//des._sMeshName = "level_opensky.mesh";
			//des._sMaterialName = "Examples/BumpMapping/SingleLight";
			//des._sMoppName = "./Media/Tk/level_opensky.tk";

			des._sMeshName = "arab.mesh";
			des._sMaterialName = "Met_Grey_SG1/SOLID/TEX/Met_Grey.bmp";
			des._sMoppName = "./Media/Tk/arab.tk";

			des._iPhysType = HKObjectDescrip::OBJ_PHYS_MOPP;
			des._qOrientation = Ogre::Quaternion();
			//des._vScale = Vector3(27.0f,27.0f,27.0f);
			des._vScale = Vector3(2,2,2);
			AddStaticMesh(des);
	/*
			//Landscape mesh.
			des._sID = "landscapemesh";
			des._sMeshName = "landscape_small.mesh";
			des._sMaterialName = "Material.001/SOLID/TEX/heightmaps.jpg";
			des._sMoppName = "./Media/Tk/landscape.tk";
			des._vPos.y -= 30;
			AddStaticMesh(des);*/
		}
		GDebugger::GetSingleton().WriteToLog("     - Static Meshes loaded.\n");

		//Load spawn points
		{
			HKSpawnDescrip des;
			AddSpawnPoint(des);
		}
		GDebugger::GetSingleton().WriteToLog("     - Spawn points added.\n");
		//Load primitives
		{
		
		}
	} else {
		//Default scene adjustments
		{
			_sceneMgr->setSkyDome(true,"Examples/CloudySky",5,8);
		}
		GDebugger::GetSingleton().WriteToLog("     - Sky dome created\n");
		//Load character controller
		{
		}

		//Load static meshes
		{
			SceneNode *level = NULL;
			_sceneLoader.parseDotScene("./Media/ArabLevel/ArabLevel2.scene","General",_sceneMgr,level);
		}
		GDebugger::GetSingleton().WriteToLog("     - Scene loaded.\n");

		//Load spawn points
		{
			HKSpawnDescrip des;
			AddSpawnPoint(des);
		}
		GDebugger::GetSingleton().WriteToLog("     - Spawn points added\n");

		//Load primitives
		{
		
		}
	}
}

void HKLevel::AddCharacterController(const HKCharacterControllerDescrip& d){
	_hkCharController = HKCharacterController::Create();
	GHavok::GetSingleton().AddObject(_hkCharController);
}
void HKLevel::AddStaticMesh(const HKStaticMeshDescrip& d){
	HKStaticMesh *tmp = new HKStaticMesh(d);
	tmp->Create();

	_hkFactory->AddObject(tmp);
	_vStaticMeshes.push_back(tmp);
}

void HKLevel::AddSpawnPoint(const HKSpawnDescrip& d){


	HKSpawnDescrip *tmp = new HKSpawnDescrip();
	tmp->_qOrient = d._qOrient;
	tmp->_vPos = d._vPos;

	_vSpawns.push_back(tmp);
}

void HKLevel::AddPrimitive(const HKPrimitiveDescrip &d){
	GDebugger::GetSingleton().WriteToLog("Debugging Primitive Spawn\n--------------------\n");
	GDebugger::GetSingleton().WriteToLog("  - Descriptor created: %s\n",d._sID.c_str());

	HKPrimitive *tmp = new HKPrimitive(d);
	GDebugger::GetSingleton().WriteToLog("  - Spawning primitive...");
	tmp->Spawn();
	GDebugger::GetSingleton().WriteToLog("Complete.\n");

	_hkFactory->AddObject(tmp);
	_vPrimitives.push_back(tmp);
}
void HKLevel::processUnbufferedKeyInput(OIS::Keyboard* mKeyboard,const FrameEvent& evt){
	if(GRakNet::GetSingleton().IsServer()){
		if(mKeyboard->isKeyDown(OIS::KC_W) )		_hkCharController->Forward(0);
		if(mKeyboard->isKeyDown(OIS::KC_S) )		_hkCharController->Backward(0);
		if(mKeyboard->isKeyDown(OIS::KC_D) )		_hkCharController->StrafeRight(0);
		if(mKeyboard->isKeyDown(OIS::KC_A) )		_hkCharController->StrafeLeft(0);
		if(mKeyboard->isKeyDown(OIS::KC_SPACE) )	_hkCharController->Jump(0);
		if(mKeyboard->isKeyDown(OIS::KC_C) )		_hkCharController->Crouch(0);
		if(mKeyboard->isKeyDown(OIS::KC_N) )		_hkCharController->SetFreeLook(!_hkCharController->GetFreeLook());
	}
}