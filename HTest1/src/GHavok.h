#ifndef __GHAVOK_HEADER__
#define __GHAVOK_HEADER__

#include "Singleton.h"

#include "ExampleApplication.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include "../res/resource.h"
#endif
#include <CEGUI.h>
#include <CEGUISystem.h>
#include <CEGUISchemeManager.h>
#include <OgreCEGUIRenderer.h>


#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>

#pragma once

class hkVisualDebugger;
class hkpWorld;
class hkPoolMemory;
class hkThreadMemory;
class hkpMultithreadingUtil;
class hkpPhysicsContext;
class hkpEntity;
class hkVector4;
class hkpPhantom;
class HKObject;

class GHavok : public HK::Singleton<GHavok> {
	friend class HK::Singleton<GHavok>;
protected:
	GHavok(): _bThreaded(false),
			  _bCreated(false),
			  _bConsoleOutput(false),
			  _nThreads(2),
			  _dHavokTimeThresh(0.02f),
			  _dCurrentTime(0.0f){
		FILE *stream = fopen("GHavokDbg.txt","w");
		fclose(stream);
	}
	~GHavok(){
		DestroyHavok();
	}
public:

	hkpEntity * AddEntity(hkpEntity *ent);
	void AddObject(HKObject *obj);

	hkpPhantom * AddPhantom(hkpPhantom *phan);
	void RemovePhantom(hkpPhantom *phan);

	//bool AddObject(HKObject *obj);
	void DestroyHavok();

	//void SetPhysicsTimeThresh(double d){_dPhysicsTimeThresh = d;}
	void SetupHavokEngine(bool bUseThreading = true, int nThreads = 2);

	void Step(double dt);
	void StepVisualDebugger();

	hkVector4 GetGravity();
	void SetGravity(const hkVector4 &grav);

	void Lock();
	void Unlock();
	
	void MarkForWrite();
	void UnmarkForWrite();

	void ResetThreadTokens();
	void StartStepWorld(double dt);
	void WaitForStepWorldFinished();

	int WriteToLog(char *fmt,...);
private:
	int _nThreads;

	bool _bThreaded;
	bool _bCreated;
	bool _bConsoleOutput;
	
	double _dCurrentTime;
	double _dHavokTimeThresh;

	std::vector<HKObject *> _vObjects;

private:// **Ogre Variables
	Ogre::SceneManager *mSceneMgr;
	Ogre::Entity *_ogreSphere;
	Ogre::SceneNode *_ogreScene;

private:// **Havok Variables
	hkVisualDebugger *SetupVisualDebugger();
	hkVisualDebugger *_hkVis;
	hkPoolMemory* _memoryManager;
	hkThreadMemory* _threadMemory;
	hkpWorld* _physicsWorld;
	char* _stackBuffer;
	hkpMultithreadingUtil* _multithreadingUtil;
	hkpPhysicsContext* _context;
};
#endif // __GHAVOK_HEADER__