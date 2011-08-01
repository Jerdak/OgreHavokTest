#include "HKCharacterController.h"

#include <Common/Base/hkBase.h>

#include <Physics/Collide/Filter/Group/hkpGroupFilter.h>
#include <Physics/Collide/Filter/Group/hkpGroupFilterSetup.h>
#include <Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxy.h>
#include <Physics/Utilities/CharacterControl/StateMachine/hkpDefaultCharacterStates.h>

#include <Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h>
#include <Physics/Dynamics/Phantom/hkpSimpleShapePhantom.h>
#include <Physics/Collide/Query/Collector/PointCollector/hkpClosestCdPointCollector.h>
#include <Physics/Collide/Query/Collector/PointCollector/hkpAllCdPointCollector.h>
#include <Physics/ConstraintSolver/Simplex/hkpSimplexSolver.h>
#include <Physics/Collide/Query/CastUtil/hkpLinearCastInput.h>

#include <Physics/Dynamics/World/hkpWorld.h>
#include <Physics/Dynamics/Entity/hkpRigidBody.h>

//#include <Demos/DemoCommon/Utilities/GameUtils/GameUtils.h>
//#include <Demos/DemoCommon/Utilities/Character/CharacterUtils.h>
//#include <hkdemoframework/hkDemoFramework.h>

//#include <Demos/DemoCommon/DemoFramework/hkTextDisplay.h>

#include <Graphics/Common/Input/Pad/hkgPad.h>
#include <Graphics/Common/Window/hkgWindow.h>

#include "./CharacterProxyMotion/HKState.h"
#include "./CharacterProxyMotion/HKStateMachine.h"
#include "./CharacterProxyMotion/HKStateWalk.h"
#include "./CharacterProxyMotion/HKStateStand.h"
#include "./CharacterProxyMotion/HKStateInAir.h"

using namespace DataStructures;
using namespace RakNet;



HKCharacterControllerDescrip::HKCharacterControllerDescrip():
							   HKObjectDescrip()
							  
{
	_ogreCamera				= NULL;

	_dGravitySpeed			= 1.0f/100.0f;
	_dTimeStep				= 1.0f/45.0f;
	_dInvTimeStep			= 45.0f;
	_dMaxSpeed				= 20.0f;
	_dCrouchHeight			= 0.2f;
	_dStandHeight			= 0.7f;
	_dGravity				= -16.0f;
	_dRotationSpeedX		= 0.2f;
	_dRotationSpeedY		= 0.2f;
	_iUpAxis				= 1;
	
	_up						= new hkVector4;
	_up->setZero4();
	(*_up)(_iUpAxis) = _iUpAxis;	
};

class MyCharacterListener : public hkReferencedObject, public hkpCharacterProxyListener
{
	public:
		HK_DECLARE_CLASS_ALLOCATOR(HK_MEMORY_CLASS_DEMO);

		MyCharacterListener( ) 
		: m_atLadder(false)
		{}

		// Ladder handling code goes here
		void contactPointAddedCallback(const hkpRootCdPoint& point)
		{
			hkpRigidBody* body = hkGetRigidBody(point.m_rootCollidableB);

			if ( body->hasProperty(HK_OBJECT_IS_LADDER) )
			{
				m_atLadder = true;
				m_ladderNorm = point.m_contact.getNormal();
				body->getPointVelocity(point.m_contact.getPosition(), m_ladderVelocity);
			}
		}

		void contactPointRemovedCallback(const hkpRootCdPoint& point)
		{
			hkpRigidBody* body = hkGetRigidBody(point.m_rootCollidableB);

			if ( body->hasProperty(HK_OBJECT_IS_LADDER) )
			{
				m_atLadder = false;
			}
		}

	public:

		hkBool m_atLadder;

		hkVector4 m_ladderNorm;

		hkVector4 m_ladderVelocity;

};

HKCharacterController::HKCharacterController(const HKCharacterControllerDescrip &d):HKObject(d),
																					_descrip(d){
	//TODO:  Add some sort of global class to maintain the camera and manager pointers so we never need set them twice....
	//			I know Ogre provides a global singleton for the manager class, perhaps I could just poll it for the camera.
	if(d._ogreCamera == NULL)GDebugger::GetSingleton().WriteToLog("ERROR: No camera set in Character Controller Description.\n");
	if(_sceneMgr == NULL)GDebugger::GetSingleton().WriteToLog("ERROR: No scene manager set in Character Controller Description.\n");

	Init();
}

HKCharacterController::~HKCharacterController(){

}


HKCharacterController *HKCharacterController::Create(){
	HKCharacterController *c = NULL;
	

	//Create new character controller descriptor
	HKCharacterControllerDescrip tmpDes;
	{
		//tmpDes._ogreCamera = Ogre::Root::getSingleton().getSceneManager("ExampleSMInstance")->getCamera("PlayerCam");
		//tmpDes._ogreCameraNode = Ogre::Root::getSingleton().getSceneManager("ExampleSMInstance")->getSceneNode("CameraNode");

		tmpDes._iUpAxis = 1;
		delete tmpDes._up;
		tmpDes._up = new hkVector4;

		tmpDes._up->setZero4();
		(*tmpDes._up)(tmpDes._iUpAxis) = 1;	
	}

	//Create new character controller
	{
		c = new HKCharacterController(tmpDes);
		c->SetReplicaManager(GRakNet::GetSingleton().GetReplicaManager("character_controller")); //Find network replication controller
		GHavok::GetSingleton().AddObject(c);		
	}
	
	//If this is the server be sure to automatically broadcast object creation to the client.
	if(GRakNet::GetSingleton().IsServer()){
		c->AddAutoSerializeTimer(GRakNet::GetSingleton().GetTimeDelay()  );
		// All connected systems should create this object
		c->BroadcastConstruction();
		// Force the first serialize to go out. We could have also just wrote the data in SerializeConstruction()
		// Without this call, no serialize would occur until something changed from the intial value
		c->BroadcastSerialize();
	}

	return c;
}
void HKCharacterController::Init(){
	//	Create a character proxy object
	GDebugger::GetSingleton().WriteToLog("  - Initializing Character Controller...");

	dprintf("Size of quaternion: %d\n",sizeof(Ogre::Quaternion));
	Ogre::Quaternion q;
	
	if(GRakNet::GetSingleton().IsServer()){
		GDebugger::GetSingleton().WriteToLog("... server controller..");
		{
			// Construct a shape

			hkVector4 vertexA(0, 0.4f,0.0);
			hkVector4 vertexB(0,-0.4f,0.0);		

			// Create a capsule to represent the character standing
			_standShape = new hkpCapsuleShape(vertexA, vertexB, .6f);

			// Create a capsule to represent the character crouching
			// Note that we create the smaller capsule with the base at the same position as the larger capsule.
			// This means we can simply swap the shapes without having to reposition the character proxy,
			// and if the character is standing on the ground, it will still be on the ground.
			vertexA.setZero4();
			_crouchShape = new hkpCapsuleShape(vertexA, vertexB, .6f);


			// Construct a Shape Phantom
			_phantom = new hkpSimpleShapePhantom( _standShape, hkTransform::getIdentity(), hkpGroupFilter::calcFilterInfo(0,2) );
			
			// Add the phantom to the world
			GHavok::GetSingleton().AddPhantom(_phantom);
			//m_phantom->removeReference();

			// Construct a character proxy
			hkpCharacterProxyCinfo cpci;
			cpci.m_position.set(0,15,0);//(-9.1f, 35, .4f);
			cpci.m_staticFriction = 0.0f;
			cpci.m_dynamicFriction = 1.0f;
			cpci.m_up.setNeg4( GHavok::GetSingleton().GetGravity() );
			cpci.m_up.normalize3();	
			cpci.m_userPlanes = 4;
			cpci.m_maxSlope = HK_REAL_PI / 3.f;
			cpci.m_characterStrength = 250.0f;
			cpci.m_shapePhantom = _phantom;
			_characterProxy = new hkpCharacterProxy( cpci );
			_stateCharacterContext.m_characterProxy = _characterProxy;
		}

		//
		// Add in a custom friction model
		//
		{
			hkVector4 up( 0.f, 1.f, 0.f );
			_listener = new MyCharacterListener();
			_characterProxy->addCharacterProxyListener(_listener);
		}

		//
		// Create the Character state machine and context
		//
		{
		/*	hkpCharacterState* state;
			hkpCharacterStateManager* manager = new hkpCharacterStateManager();

			state = new hkpCharacterStateOnGround();
			manager->registerState( state,	HK_CHARACTER_ON_GROUND);
			state->removeReference();

			state = new hkpCharacterStateInAir();
			manager->registerState( state,	HK_CHARACTER_IN_AIR);
			state->removeReference();

			state = new hkpCharacterStateJumping();
			manager->registerState( state,	HK_CHARACTER_JUMPING);
			state->removeReference();

			state = new hkpCharacterStateClimbing();
			manager->registerState( state,	HK_CHARACTER_CLIMBING);
			state->removeReference();

			_characterContext = new hkpCharacterContext(manager, HK_CHARACTER_ON_GROUND);
			manager->removeReference();*/

		}
		// State machine
		{
			_stateMachine = new HKStateMachine();

			// Prepare State Machine
			{
				HKState* state = new HKStateWalk();
				_stateMachine->registerState (HK_STATE_WALKING, state);
				state->removeReference();


				state = new HKStateStand();
				_stateMachine->registerState (HK_STATE_STANDING, state);
				state->removeReference();

				state = new HKStateInAir();
				_stateMachine->registerState (HK_STATE_IN_AIR, state);
				state->removeReference();

			}
		}
		//If no entity was provided create a simple sphere. (For 3rd person view testing)
		if(_descrip._ogreEntity == NULL){
			Ogre::String str("demoball2");
			//Add a demo ball to debug
			_descrip._ogreEntity = _sceneMgr->createEntity( str + "_Entity", "sphere_small.mesh" );
			_descrip._ogreEntity->setMaterialName("Ogre/Eyes");
			_descrip._ogreEntity->setNormaliseNormals(true);
			_descrip._ogreEntity->setCastShadows(true);
			if(_descrip._ogreScene == NULL){
				_descrip._ogreScene = _sceneMgr->getRootSceneNode()->createChildSceneNode( str + "_SceneNode" );
				_descrip._ogreScene->attachObject( _descrip._ogreEntity );
			}
		}
		_bCrouch = _bJump = _bFreeLook = false;
	} else {
		GDebugger::GetSingleton().WriteToLog("... client controller..");
	}
	_descrip._vPos = Ogre::Vector3::ZERO;

	GDebugger::GetSingleton().WriteToLog("Complete");
	_bEnableInterpolation = true;
	_bOffAxis = false;;
	_dCaveRoll = _dCavePitch = _dCaveYaw = 0.0f;

	_timer.Start();
	_timer.Stop();
	_dLastPacketTime = _timer.GetDurationInMSecs();

	LoadIni("HTest1.ini");
	transformationHistory.Init(30,1000);
}
void HKCharacterController::Forward(double dt){
	if(_bFreeLook)return;
	_descrip._vPos.x = -1.f;
}

void HKCharacterController::Backward(double dt){
	if(_bFreeLook)return;
	_descrip._vPos.x = 1.f;
}
void HKCharacterController::StrafeLeft(double dt){
	if(_bFreeLook)return;

	_descrip._vPos.y = -1.f;
}
void HKCharacterController::StrafeRight(double dt){
	if(_bFreeLook)return;

	_descrip._vPos.y = 1.f;
}

void HKCharacterController::TurnLeft(double dt){
	if(_bFreeLook)return;
}


void HKCharacterController::TurnRight(double dt){
	if(_bFreeLook)return;
}


void HKCharacterController::Jump(double dt){
	if(_bFreeLook)return;

	_bJump = true;
}
void HKCharacterController::Crouch(double dt){
	if(_bFreeLook)return;

	_bCrouch = true;
}
void HKCharacterController::SetFreeLook(bool b) { 
	if(_bFreeLook && !b){
		Ogre::Vector3 ogpos = _descrip._ogreCamera->getPosition();
		_characterProxy->setPosition(hkVector4(ogpos.x,ogpos.y,ogpos.z));
	}
	_bFreeLook = b;
}

void HKCharacterController::SetMovementSpeed(double d){
	_descrip._dMaxSpeed = d;
}
void HKCharacterController::Step(double dt){
	visiblePosition		=	position;
	visibleOrientation	=	orientation;
	
	
	if(GRakNet::GetSingleton().IsServer()){
		if(_bFreeLook)return;
		GHavok::GetSingleton().Lock();

		// Get input for the state machine from the keyboard
		HKStateMachineInput input;
		hkQuaternion orient;

		Quaternion q = GOffAxisCamera::GetSingleton().GetRealCameraOrientation();
		orient.setAxisAngle(*_descrip._up, q.getYaw().valueRadians());

		input.m_inputLR = _descrip._vPos.y;
		input.m_inputUD = _descrip._vPos.x;
		input.m_wantJump = false;
		input.m_atLadder = false;

		input.m_wantJump =  _bJump;
		input.m_atLadder = _listener->m_atLadder;

		input.m_up = *_descrip._up;
		input.m_forward.set(0,0,1);
		input.m_forward.setRotatedDir( orient, input.m_forward );

	
		input.m_maxSpeed = _descrip._dMaxSpeed;
		input.m_stepInfo.m_deltaTime	=_descrip._dTimeStep;
		input.m_stepInfo.m_invDeltaTime =_descrip._dInvTimeStep;
		input.m_characterGravity.set(0,_descrip._dGravity,0);
		input.m_velocity = _characterProxy->getLinearVelocity();
		input.m_position = _characterProxy->getPosition();
		input.m_maxVelocity = hkVector4(20,20,20,0);

		// Update the state machine
		HKStateMachineOutput output;
		_stateMachine->update(_stateCharacterContext, input, output);
		GHavok::GetSingleton().Unlock();

		output.m_orient = orient;
		stepWorldAndController(output);
		//Apply the player character controller	
	/*	{
			// Feed output from state machine into character proxy
			_characterProxy->setLinearVelocity(output.m_velocity);
			
			hkStepInfo si;
			si.m_deltaTime = _descrip._dMovementSpeed;
			si.m_invDeltaTime = 1/_descrip._dMovementSpeed;
			
			_characterProxy->integrate( si, GHavok::GetSingleton().GetGravity() );
		}
		*/
		// Handle crouching
		{
			hkBool wantCrouch = _bCrouch;

			hkBool isCrouching = _phantom->getCollidable()->getShape() == _crouchShape;


			// We want to stand
			if (isCrouching && !wantCrouch)
			{
				swapPhantomShape(_standShape);
			}

			// We want to crouch
			if (!isCrouching && wantCrouch)
			{
				swapPhantomShape(_crouchShape);
			
			}
		}

		double dCamHeight = _descrip._dStandHeight;

		//Drop camera height to simulate crouching.
		if(_bCrouch) dCamHeight = _descrip._dCrouchHeight;

		//Handle the camera 
		{
			hkVector4 tmp = _characterProxy->getPosition();
			hkVector4 tmp2(tmp);
			tmp2(_descrip._iUpAxis) += dCamHeight;
			
			//_descrip._ogreCamera->setPosition(tmp2(0),tmp2(1),tmp2(2));
			position = Vector3(tmp2(0),tmp2(1),tmp2(2));
			orientation = GOffAxisCamera::GetSingleton().GetOrientation();//_descrip._ogreCameraNode->getOrientation();
			GOffAxisCamera::GetSingleton().SetPosition(position);	
		}
		
		//Reset
		_descrip._vPos = Vector3(0,0,0); 
		_bJump = _bCrouch = 0;
		/*hkQuaternion orient;

		//Orient the Havok object using the ogre camera.
		Quaternion q = GOffAxisCamera::GetSingleton().GetRealCameraOrientation();//_descrip._ogreCamera->getRealOrientation();
		orient.setAxisAngle(*_descrip._up, q.getYaw().valueRadians());

		{
			GHavok::GetSingleton().Lock();
			
			hkpCharacterInput input;
			hkpCharacterOutput output;

			//Setup character inputs
			{
				input.m_inputLR = _descrip._vPos.x;
				input.m_inputUD = _descrip._vPos.y;
				input.m_wantJump = false;
				input.m_atLadder = false;

				input.m_wantJump =  _bJump;
				input.m_atLadder = _listener->m_atLadder;

				input.m_up = *_descrip._up;
				input.m_forward.set(1,0,0);
				input.m_forward.setRotatedDir( orient, input.m_forward );

			
				input.m_stepInfo.m_deltaTime	=_descrip._dMovementSpeed;
				input.m_stepInfo.m_invDeltaTime = 1/_descrip._dMovementSpeed;//_descrip._dInvMovementSpeed;
				input.m_characterGravity.set(0,_descrip._dGravity,0);
				input.m_velocity = _characterProxy->getLinearVelocity();

				GDebugger::GetSingleton().WriteToLog("Vel: %f %f %f\n",input.m_velocity(0),input.m_velocity(1),input.m_velocity(2));
				input.m_position = _characterProxy->getPosition();
				

				//Check listener for ladder collision.  If found change the input forward vec. to point up.
				if (_listener->m_atLadder)
				{
					hkVector4 right, ladderUp;
					right.setCross( *_descrip._up, _listener->m_ladderNorm );
					ladderUp.setCross( _listener->m_ladderNorm, right );
					// Calculate the up vector for the ladder
					if (ladderUp.lengthSquared3() > HK_REAL_EPSILON)
					{
						ladderUp.normalize3();
					}

					// Reorient the forward vector so it points up along the ladder
					input.m_forward.addMul4( -_listener->m_ladderNorm.dot3(input.m_forward), _listener->m_ladderNorm);
					input.m_forward.add4( ladderUp );
					input.m_forward.normalize3();

					input.m_surfaceNormal = _listener->m_ladderNorm;
					input.m_surfaceVelocity = _listener->m_ladderVelocity;
				}
				else 
				{
					hkVector4 down;	down.setNeg4(*_descrip._up);
					hkpSurfaceInfo ground;
					_characterProxy->checkSupport(down, ground);
					input.m_isSupported = ground.m_supportedState == hkpSurfaceInfo::SUPPORTED;

					input.m_surfaceNormal = ground.m_surfaceNormal;
					input.m_surfaceVelocity = ground.m_surfaceVelocity;
				}
			}
			
			// Apply the character state machine
			_characterContext->update(input, output);

			double dCamHeight = _descrip._dStandHeight;
			//Apply the player character controller	
			{
		
				// Feed output from state machine into character proxy
				_characterProxy->setLinearVelocity(output.m_velocity);
				
				hkStepInfo si;
				si.m_deltaTime = _descrip._dMovementSpeed;
				si.m_invDeltaTime = 1/_descrip._dMovementSpeed;
				
				
				_characterProxy->integrate( si, GHavok::GetSingleton().GetGravity() );

			}
		
			//
			// Handle crouching
			//
			{
				hkBool wantCrouch = _bCrouch;

				hkBool isCrouching = _phantom->getCollidable()->getShape() == _crouchShape;


				// We want to stand
				if (isCrouching && !wantCrouch)
				{
					swapPhantomShape(_standShape);
				}

				// We want to crouch
				if (!isCrouching && wantCrouch)
				{
					swapPhantomShape(_crouchShape);
				
				}
			}

			//Drop camera height to simulate crouching.
			if(_bCrouch) dCamHeight = _descrip._dCrouchHeight;

			//Handle the camera 
			{
				hkVector4 tmp = _characterProxy->getPosition();
				hkVector4 tmp2(tmp);
				tmp2(_descrip._iUpAxis) += dCamHeight;
				
				//_descrip._ogreCamera->setPosition(tmp2(0),tmp2(1),tmp2(2));
				position = Vector3(tmp2(0),tmp2(1),tmp2(2));
				orientation = GOffAxisCamera::GetSingleton().GetOrientation();//_descrip._ogreCameraNode->getOrientation();
				GOffAxisCamera::GetSingleton().SetPosition(position);	
			}
			
			//Reset
			_descrip._vPos = Vector3(0,0,0); 
			_bJump = _bCrouch = 0;
		}*/
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

		GOffAxisCamera::GetSingleton().SetPosition(visiblePosition);
		Ogre::Radian rad = visibleOrientation.getPitch();
		GOffAxisCamera::GetSingleton().SetOrientation(visibleOrientation);

		//_descrip._ogreCameraNode->setOrientation(visibleOrientation);
		//_descrip._ogreCamera->setPosition(visiblePosition);
	}
}
void HKCharacterController::stepWorldAndController(const HKStateMachineOutput& output)
{
	hkStepInfo si;

	si.m_deltaTime = _descrip._dTimeStep;
	si.m_invDeltaTime = _descrip._dInvTimeStep;

	hkVector4 m_up = *_descrip._up;

	//Apply the player character controller when the bounding boxes are correct
	const hkQsTransform& desiredMotion = output.m_desiredMotion;

	GHavok::GetSingleton().MarkForWrite();

	// Calculate the velocity we need in order to achieve the desired motion
	hkVector4 desiredVelocity;
	{
		hkVector4 desiredMotionWorld;
		desiredMotionWorld.setRotatedDir( output.m_orient, output.m_velocity/* desiredMotion.getTranslation() */);
		
		// this is the common case: add the motion velocity to the downward part of the proxy velocity (not upward though)
		//hkReal vertComponent = hkMath::min2(0.0f, static_cast<hkReal>(characterLinearVelocity.dot3( m_characterProxy->getUpLocal() )));
	//	desiredVelocityWS.addMul4(vertComponent, m_characterProxy->getUpLocal() );
		
	//	dprintf("Desired Motion: %f %f %f\n",desiredMotionWorld(0),desiredMotionWorld(1),desiredMotionWorld(2));
		// Divide motion by time
		desiredVelocity.setMul4 (si.m_invDeltaTime, desiredMotionWorld );

		//desiredVelocity.add4(output.m_velocity);
		desiredVelocity = output.m_velocity;
	}

	// Assume all desired angular motion is around the up axis
	hkReal desiredMotionAngle = desiredMotion.getRotation().getAngle();
	// Cater for inverted up axis
	if (desiredMotion.m_rotation.m_vec(2)<0)
	{
		desiredMotionAngle *= -1;
	}

	// There is nothing that can stop us rotating
	_stateCharacterContext.m_currentAngle += desiredMotionAngle;

	// Feed output from state machine into character proxy
	_characterProxy->setLinearVelocity(desiredVelocity);
	
	_characterProxy->integrate( si, GHavok::GetSingleton().GetGravity() );

	GHavok::GetSingleton().UnmarkForWrite();
}
/*
Pulled directly from Havok demo src.  Swaps character standing model with the crouching model.
*/
void HKCharacterController::swapPhantomShape( hkpShape* newShape )
{
	if(_bFreeLook)return;

	// Remember the current shape
	hkpShape* currentShape = const_cast<hkpShape*>(_phantom->getCollidable()->getShape());

	// Swap to the new shape.
	// N.B. To be safe, we always remove the phantom from the world first, then change the shape,
	// then re-add, in order to refresh the cached agents in any hkCachingShapePhantoms which
	// may also be present in the world.
	// This also forces the display to be rebuilt, which is necessary for us to see the new shape!
	{
		// Note we do not have to add a reference before removing becasue we hold a hkpCharacterProxy
		// which has a reference to this phantom - hence removal from the world cannot cause this phantom to 
		// be accidentally deleted.
		GHavok::GetSingleton().RemovePhantom(_phantom);
		_phantom->setShape(newShape);
		GHavok::GetSingleton().AddPhantom(_phantom);
	}

	//
	// We use getClosestPoints to check for penetration
	//
	hkpClosestCdPointCollector collector;
	_phantom->getClosestPoints( collector );

	// Allow a very slight tolerance (approx 1cm)

	if (collector.hasHit() && collector.getHit().m_contact.getDistance() < .01f)
	{
		// Switch the phantom back to our current shape.
		// N.B. To be safe, we always remove the phantom from the world first, then change the shape,
		// then re-add, in order to refresh the cached agents in any hkCachingShapePhantoms which
		// may also be present in the world.
		// This also forces the display to be rebuilt, which is necessary for us to see the new shape!	
		{
			// Note we do not have to add a reference before removing becasue we hold a hkpCharacterProxy
			// which has a reference to this phantom - hence removal from the world cannot cause this phantom to 
			// be accidentally deleted.
			GHavok::GetSingleton().RemovePhantom(_phantom);
			_phantom->setShape(currentShape);
			GHavok::GetSingleton().AddPhantom(_phantom);

		}
	}
}

bool HKCharacterController::SerializeConstruction(RakNet::BitStream *bitStream, SerializationContext *serializationContext)
{
	RakAssert(GRakNet::GetSingleton().IsServer()==true);
	StringTable::Instance()->EncodeString("character_controller", 128, bitStream);
	return true;
}
static int pk = 0;
bool HKCharacterController::Serialize(RakNet::BitStream *bitStream, SerializationContext *serializationContext)
{
	// Autoserialize causes a network packet to go out when any of these member variables change.
	RakAssert(GRakNet::GetSingleton().IsServer()==true);
	
	bitStream->Write(true);
	bitStream->WriteAlignedBytes((const unsigned char*)&pk,sizeof(pk));
	bitStream->WriteAlignedBytes((const unsigned char*)&position,sizeof(position));
	bitStream->WriteAlignedBytes((const unsigned char*)&orientation,sizeof(orientation));
	pk++;
	_timer.Stop();
	double tmpTime = _timer.GetDurationInSecs();
	GDebugger::GetSingleton().WriteToLog("[%d] Packet sent %f\n",pk,tmpTime - _dLastPacketTime);
	_dLastPacketTime = tmpTime;

	return true;
}

void HKCharacterController::Deserialize(RakNet::BitStream *bitStream, SerializationType serializationType, SystemAddress sender, RakNetTime timestamp)
{
	// Doing this because we are also lagging position and orientation behind by DEFAULT_SERVER_MILLISECONDS_BETWEEN_UPDATES
	// Without it, the kernel would pop immediately but would not start moving
	bool bRead = true;
	int senderPk;

	bitStream->Read(bRead);
	bitStream->ReadAlignedBytes((unsigned char*)&senderPk,sizeof(senderPk));
	bitStream->ReadAlignedBytes((unsigned char*)&position,sizeof(position));
	bitStream->ReadAlignedBytes((unsigned char*)&orientation,sizeof(orientation));

	_timer.Stop();
	double tmpTime = _timer.GetDurationInSecs();
	GDebugger::GetSingleton().WriteToLog("[%d] Packet received %f.  Timestamp: %f\n",senderPk,tmpTime - _dLastPacketTime,timestamp);


	_dLastPacketTime = tmpTime;

	// Scene node starts invisible until we deserialize the intial startup data
	// This data could also have been passed in SerializeConstruction()
//		sceneNode->setVisible(true,true);

	// Every time we get a network packet, we write it to the transformation history class.
	// This class, given a time in the past, can then return to us an interpolated position of where we should be in at that time
	transformationHistory.Write(position,Vector3(0,0,0),orientation,RakNet::GetTime());
	transformationHistory2.Write(position);
	//GDebugger::GetSingleton().WriteToLog("Readin data\n");
}

bool HKCharacterController::LoadIni(char *file)
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
			
		LOOKUP("EnableOffAxis",	_bOffAxis,INTEGER)
		ELOOKUP("CaveRoll",			_dCaveRoll,DOUBLE)
		ELOOKUP("CavePitch",		_dCavePitch,DOUBLE)
		ELOOKUP("CaveYaw",			_dCaveYaw,DOUBLE)
		ELOOKUP("MovementSpeed",	_descrip._dMaxSpeed,DOUBLE)
	}
	return true;
}