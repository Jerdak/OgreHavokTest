#include "GHavok.h"
#include "HKObject.h"

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

// Visual Debugger includes
#include <Common/Visualize/hkVisualDebugger.h>
#include <Physics/Utilities/VisualDebugger/hkpPhysicsContext.h>

// Keycode
#include <Common/Base/keycode.cxx>

#define HK_CLASSES_FILE <Common/Serialize/ClassList/hkPhysicsClasses.h>
#include <Common/Serialize/Util/hkBuiltinTypeRegistry.cxx>

// Generate a custom list to trim memory requirements
#define HK_COMPAT_FILE <Common/Compat/hkCompatVersions.h>

#include <Common/Compat/hkCompat_none.cxx>

static void HK_CALL errorReport(const char* msg, void*)
{
	GHavok::GetSingleton().WriteToLog("%s",msg);
}

inline int GHavok::WriteToLog(char *fmt,...){

	char *out = "GHavokDbg.txt";
	FILE* __fStdOut = NULL;
	HANDLE __hStdOut = NULL;

	if(_bConsoleOutput)
		__hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	else
		__fStdOut = fopen(out,"a");
	
	if(__fStdOut == NULL && __hStdOut == NULL)
		return 0;

	char s[300];
	va_list argptr;
	int cnt;

	va_start(argptr, fmt);
	cnt = vsprintf(s, fmt, argptr);
	va_end(argptr);

	DWORD cCharsWritten;

	if(__hStdOut)
		WriteConsole(__hStdOut, s, strlen(s), &cCharsWritten, NULL);

	if(__fStdOut){
		fprintf(__fStdOut, s);
		fclose(__fStdOut);
	}
	return(cnt);
}


void GHavok::StepVisualDebugger()
{

	_context->m_monitorStreamBegins.setSize( _nThreads );
	_context->m_monitorStreamEnds.setSize( _nThreads );

	// Set the timer streams for passing to the visual debugger
	for (int j = 0; j < _multithreadingUtil->getNumThreads(); ++j)
	{
		_context->m_monitorStreamBegins[j] = _multithreadingUtil->m_state.m_workerThreads[j].m_monitorStreamBegin;
		_context->m_monitorStreamEnds[j] = _multithreadingUtil->m_state.m_workerThreads[j].m_monitorStreamEnd;
	}

	// Step the debugger
	_hkVis->step();
}
/*
void GHavok::Step(double dt){
	_dCurrentTime += dt;

	if(_dCurrentTime >= _dPhysicsTimeThresh){
		// This must be called single threaded here, prior to calling startStepWorld() below.
		physicsWorld->resetThreadTokens();

		// This will signal all threads which are in the wait state, to start processing stepDeltaTime() concurrently.
		multithreadingUtil->startStepWorld( _currentTime );

		// We can now do other things in this thread if we wish, while the physics runs in separate threads.
		// The call below will block until all physics processing the timestep is finished in all threads.
		multithreadingUtil->waitForStepWorldFinished();

		stepVisualDebugger( _hkVis, _multithreadingUtil, _context );

		//Get physics ball position
		hkVector4 pos = _hkSphere->getPosition();
		
		//Apply ball position to ogrenode
		_ogreScene->setPosition(pos(0),pos(1)+0.05f,pos(2));

		_dCurrentTime = 0.0f;
	}
}*/
void GHavok::DestroyHavok(){
	WriteToLog("Destroying havok environment...");

	_physicsWorld->markForWrite();
	_physicsWorld->removeReference();

	delete _multithreadingUtil;

	_hkVis->removeReference();
	
	// Contexts are not reference counted at the base class level by the VDB as
	// they are just interfaces really. So only delete the context after you have
	// finished using the VDB.
	_context->removeReference();

	// Deallocate stack area
	_threadMemory->setStackArea(0, 0);
	hkDeallocate(_stackBuffer);
	WriteToLog("Complete\n");

	//Deallocate runtime blocks
	hkMemory::getInstance().freeRuntimeBlocks();

	_bCreated = false;
	_nThreads = 0;
	_bThreaded = false;
}
hkVisualDebugger* GHavok::SetupVisualDebugger()
{
	// Setup the visual debugger
	hkArray<hkProcessContext*> contexts;
	contexts.pushBack(_context);

	hkVisualDebugger* vdb = new hkVisualDebugger(contexts);
	vdb->serve();

	return vdb;
}
//TODO:  Currently this function ONLY supports threading.  Setting it to anything other than the defaults will crash the program.
void GHavok::SetupHavokEngine(bool bUseThreading, int nThreads){

	if(_bCreated){
		WriteToLog("WARNING: SetupHavokEngine called more than once.\n");
		return;
	}
	WriteToLog("SetupHavokEngine()\n");
	WriteToLog("   - Use Threading? %d\n",bUseThreading);
	WriteToLog("   - N. Threads: %d\n",nThreads);

	_nThreads = nThreads;
	_bThreaded = bUseThreading;

	WriteToLog("   - Allocating memory manager and thread memory....");
	// Initialize the base system including our memory system
	_memoryManager = new hkPoolMemory();
	_threadMemory = new hkThreadMemory(_memoryManager, 16);

	hkBaseSystem::init( _memoryManager, _threadMemory, errorReport );
	_memoryManager->removeReference();

	WriteToLog("Complete.\n");

	WriteToLog("   - Initializing stack...");
	// We now initialize the stack area to 100k (fast temporary memory to be used by the engine).
	{
		int stackSize = 0x100000;
		_stackBuffer = hkAllocate<char>( stackSize, HK_MEMORY_CLASS_BASE);
		hkThreadMemory::getInstance().setStackArea( _stackBuffer, stackSize);
	}
	WriteToLog("Complete\n");

	{

		WriteToLog("   - Creating physics world...");
		// Create the physics world
		{
			// The world cinfo contains global simulation parameters, including gravity, solver settings etc.
			hkpWorldCinfo worldInfo;

			// Set the simulation type of the world to multi-threaded.
			worldInfo.m_simulationType = hkpWorldCinfo::SIMULATION_TYPE_MULTITHREADED;
			worldInfo.m_collisionTolerance = 0.03f;	//TODO: Make this a class variable
			_physicsWorld = new hkpWorld(worldInfo);
		}
		WriteToLog("Complete\n");

		WriteToLog("   - Preallocating havok memory blocks...");

		//
		// Pre-allocate some larger memory blocks. These are used by the physics system when in multithreaded mode
		// The amount and size of these depends on your physics usage.  Larger simulation islands will use larger memory blocks.
		//
		{
			hkMemory::getInstance().preAllocateRuntimeBlock(512000, HK_MEMORY_CLASS_BASE);
			hkMemory::getInstance().preAllocateRuntimeBlock(256000, HK_MEMORY_CLASS_BASE);
			hkMemory::getInstance().preAllocateRuntimeBlock(128000, HK_MEMORY_CLASS_BASE);
			hkMemory::getInstance().preAllocateRuntimeBlock(64000, HK_MEMORY_CLASS_BASE);
			hkMemory::getInstance().preAllocateRuntimeBlock(32000, HK_MEMORY_CLASS_BASE);
			hkMemory::getInstance().preAllocateRuntimeBlock(16000, HK_MEMORY_CLASS_BASE);
			hkMemory::getInstance().preAllocateRuntimeBlock(16000, HK_MEMORY_CLASS_BASE);
		}
		WriteToLog("Complete.\n");

		//
		// When the simulation type is SIMULATION_TYPE_MULTITHREADED, in the debug build, the sdk performs checks
		// to make sure only one thread is modifying the world at once to prevent multithreaded bugs. Each thread
		// must call markForRead / markForWrite before it modifies the world to enable these checks.
		//

		_physicsWorld->markForWrite();

		//
		// Register all collision agents, even though only box - box will be used in this particular example.
		// It's important to register collision agents before adding any entities to the world.
		//
		WriteToLog("   - Registering agents...");
		{
			hkpAgentRegisterUtil::registerAllAgents( _physicsWorld->getCollisionDispatcher() );
		}
		WriteToLog("Complete.\n");

		//
		// Create a multi-threading utility.  This utility will create a number of threads that will run the
		// physics step in parallel. The main thread calls functions in the utility which signal these threads
		// to start a new step, or wait for step completion.
		//

		WriteToLog("   - Creating multi-threaded environment...");
		hkpMultithreadingUtilCinfo info;
		info.m_world = _physicsWorld;
		info.m_numThreads = _nThreads;

		// In this example we enable timers. The multi-threading util will allocate a buffer per thread for capturing timer data.
		// If you leave this flag to false, no timers will be enabled.
		info.m_enableTimers = true;
		_multithreadingUtil = new hkpMultithreadingUtil(info);
		WriteToLog("Complete.\n");

		// Create all the physics rigid bodies
		//SetupPhysicsWorld( physicsWorld );

		//
		// Initialize the visual debugger so we can connect remotely to the simulation
		// The context must exist beyond the use of the VDB instance, and you can make
		// whatever contexts you like for your own viewer types.
		//

		WriteToLog("   - Initializing visual debugger...");
		_context = new hkpPhysicsContext();
		hkpPhysicsContext::registerAllPhysicsProcesses(); // all the physics viewers
		_context->addWorld(_physicsWorld); // add the physics world so the viewers can see it

		_hkVis = SetupVisualDebugger();

		_context->m_monitorStreamBegins.setSize(_nThreads);
		_context->m_monitorStreamEnds.setSize(_nThreads);
		WriteToLog("Complete\n");

		// Now we have finished modifying the world, release our write marker.
		_physicsWorld->unmarkForWrite();
	}	
	_bCreated = true;
	WriteToLog("\nSetupHavokEngine() succeeded.\n");
	
}
void GHavok::AddObject(HKObject *obj){
	_vObjects.push_back(obj);
}

void GHavok::Step(double dt){
	_dCurrentTime += dt;

	if(_dCurrentTime >= _dHavokTimeThresh){
		double dTimeChange = _dCurrentTime;
		ResetThreadTokens();
		StartStepWorld(_dCurrentTime);
		WaitForStepWorldFinished();
		
		StepVisualDebugger();

		Lock();

		std::vector<HKObject*>::iterator iter;
		
		for(iter = _vObjects.begin() ;iter != _vObjects.end(); iter++){
			(*iter)->Step(dTimeChange);
		}

		Unlock();

		_dCurrentTime = 0.0f;
	}
}

hkpPhantom * GHavok::AddPhantom(hkpPhantom *phan){return _physicsWorld->addPhantom(phan);}
void GHavok::RemovePhantom(hkpPhantom *phan){_physicsWorld->removePhantom(phan);}

hkpEntity *GHavok::AddEntity(hkpEntity *ent){return _physicsWorld->addEntity(ent);}
void GHavok::ResetThreadTokens(){_physicsWorld->resetThreadTokens();}

void GHavok::StartStepWorld(double dt){_multithreadingUtil->startStepWorld( dt );}
void GHavok::WaitForStepWorldFinished(){_multithreadingUtil->waitForStepWorldFinished();}

void GHavok::MarkForWrite(){_physicsWorld->markForWrite();}
void GHavok::UnmarkForWrite(){_physicsWorld->unmarkForWrite();}

void GHavok::Lock(){_physicsWorld->lock();}
void GHavok::Unlock(){_physicsWorld->unlock();}

hkVector4 GHavok::GetGravity(){return _physicsWorld->getGravity();}
void GHavok::SetGravity(const hkVector4 &grav){return _physicsWorld->setGravity(grav);}