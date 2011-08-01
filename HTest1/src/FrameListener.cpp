#include "FrameListener.h"
#include "HKPrimitive.h"

#include <Mmsystem.h>
#include <Common/Base/hkBase.h>
#include <Common/Base/Monitor/hkMonitorStream.h>

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

#include <Common/Visualize/hkDebugDisplay.h>

CEGUI::MouseButton convertOISMouseButtonToCegui(int buttonID)
{
    switch (buttonID)
    {
	case 0: return CEGUI::LeftButton;
	case 1: return CEGUI::RightButton;
	case 2:	return CEGUI::MiddleButton;
	case 3: return CEGUI::X1Button;
	default: return CEGUI::LeftButton;
    }
}

HFrameListener::HFrameListener(SceneManager *sceneMgr, RenderWindow* win, Camera* cam, CEGUI::Renderer* renderer, Ogre::SceneNode *cameraNode)
		: ExampleFrameListener(win, cam, false, true),
      mGUIRenderer(renderer),
	  mCameraNode(cameraNode),
      mShutdownRequested(false),
      mUseBufferedInputKeys(false),
      mUseBufferedInputMouse(true),
      mSceneMgr(sceneMgr),
	  _bLevelLoaded(false),
	  _bFreeLook(false),
	  _bOffAxis(true),
	  mHeldObject(NULL),
	  mHeldEnt(NULL)
{
	try {
		mMouse->setEventCallback(this);
		mKeyboard->setEventCallback(this);


		mInputTypeSwitchingOn = mUseBufferedInputKeys || mUseBufferedInputMouse;
		mDebugOverlay->hide();
		
		//_sShadowType = "None";
		_hkLevel = NULL;
		
		//_iShadowSwitch = 0;

		if(GRakNet::GetSingleton().IsServer()) {
			if(mJoy){
				mJoy->setEventCallback(this);
				mJoy->setBuffered(false);
			}
			_cursor = HKCursor::Create();
			_hkTracker = new HKTracker();
			_hkTracker->ConnectServerToClient();
			_hkTracker->AttachCursor(_cursor);
			_hkTracker->AttachRenderWindow(win);
		}
		mRaySceneQuery = mSceneMgr->createRayQuery(Ray());
		_olArabDebug = OverlayManager::getSingleton().getByName("Core/ShadowDebugOverlay");
		if(_olArabDebug == NULL)GDebugger::GetSingleton().WriteToLog("   - ERROR.  Could not find ShadowDebugOverlay\n"); 
		else _olArabDebug->show();
	} catch (Exception *ep){
		GDebugger::GetSingleton().WriteToLog("   - ERROR.  HFrameListener crashed: %s\n",ep->getDescription().c_str());
	}
	GDebugger::GetSingleton().WriteToLog("  - HKFrameListener initialized\n");
}
void HFrameListener::updateShadowStats(void)
{
	static String currFps = "Current FPS: ";
	static String avgFps = "Average FPS: ";
	static String bestFps = "Best FPS: ";
	static String worstFps = "Worst FPS: ";
	static String tris = "Triangle Count: ";
	static String shadows = "Shadow Type: ";

	// update stats when necessary
	try {
		OverlayElement* guiAvg = OverlayManager::getSingleton().getOverlayElement("ShadowDebug/AverageFps");
		OverlayElement* guiCurr = OverlayManager::getSingleton().getOverlayElement("ShadowDebug/CurrFps");
		OverlayElement* guiBest = OverlayManager::getSingleton().getOverlayElement("ShadowDebug/BestFps");
		OverlayElement* guiWorst = OverlayManager::getSingleton().getOverlayElement("ShadowDebug/WorstFps");

		const RenderTarget::FrameStats& stats = mWindow->getStatistics();
		guiAvg->setCaption(avgFps + StringConverter::toString(stats.avgFPS));
		guiCurr->setCaption(currFps + StringConverter::toString(stats.lastFPS));
		guiBest->setCaption(bestFps + StringConverter::toString(stats.bestFPS)
			+" "+StringConverter::toString(stats.bestFrameTime)+" ms");
		guiWorst->setCaption(worstFps + StringConverter::toString(stats.worstFPS)
			+" "+StringConverter::toString(stats.worstFrameTime)+" ms");

		OverlayElement* guiTris = OverlayManager::getSingleton().getOverlayElement("ShadowDebug/NumTris");
		guiTris->setCaption(tris + StringConverter::toString(stats.triangleCount));

		OverlayElement* guiBatches = OverlayManager::getSingleton().getOverlayElement("ShadowDebug/ShadowType");
		guiBatches->setCaption(shadows + "[" + StringConverter::toString(_iShadowSwitch) + "]" + _sShadowType);

		OverlayElement* guiDbg = OverlayManager::getSingleton().getOverlayElement("ShadowDebug/DebugText");
		guiDbg->setCaption(mDebugText);
	}
	catch(...) { /* ignore */ }
}
bool HFrameListener::frameStarted(const FrameEvent& evt)
{
	/*

	*/
	using namespace OIS;

	if(mWindow->isClosed())	return false;

	//Need to capture/update each device
	mKeyboard->capture();
	mMouse->capture();

	if( mJoy ) mJoy->capture();
	bool buffJ = (mJoy) ? mJoy->buffered() : true;

	//Check if one of the devices is not buffered
	if( !mMouse->buffered() || !mKeyboard->buffered() || !buffJ )
	{
		// one of the input modes is immediate, so setup what is needed for immediate movement
		if (mTimeUntilNextToggle >= 0)
			mTimeUntilNextToggle -= evt.timeSinceLastFrame;

		// Move about 100 units per second
		mMoveScale = mMoveSpeed * evt.timeSinceLastFrame;
		// Take about 10 seconds for full rotation
		mRotScale = mRotateSpeed * evt.timeSinceLastFrame;

		mRotX = 0;
		mRotY = 0;
		mTranslateVector = Ogre::Vector3::ZERO;
	}

	//Check to see which device is not buffered, and handle it
	if( !mKeyboard->buffered() )
		if( processUnbufferedKeyInput(evt) == false )
			return false;
	if( !mMouse->buffered() )
		if( processUnbufferedMouseInput(evt) == false )
			return false;

	if(mJoy){
		if( !mJoy->buffered() )
			if( processUnbufferedJoyInput(evt) == false )
				return false;
	}
	if( !mMouse->buffered() || !mKeyboard->buffered() || !buffJ )
		moveCamera();

	

	if(mUseBufferedInputMouse)
	{
		_cursor->Show();
	}
	//else
	//{
	//	CEGUI::MouseCursor::getSingleton().hide( );
	//}
	//Update havok engine
	GHavok::GetSingleton().Step(evt.timeSinceLastFrame);
	GRakNet::GetSingleton().Update(evt.timeSinceLastFrame);
	if(GRakNet::GetSingleton().IsServer())_hkTracker->Update(evt.timeSinceLastFrame);

	//if(_bOffAxis)UpdateOffAxis();
	RakSleep(0);
	return true;
}
void HFrameListener::moveCamera()
{
	//Offaxis camera class takes care of movement.
	GOffAxisCamera::GetSingleton().MoveCamera(Vector3(0,mRotY.valueRadians(),mRotX.valueRadians()),mTranslateVector);
}
static int nObj = 0;

bool HFrameListener::povMoved(const OIS::JoyStickEvent &ev, int pov){
	GDebugger::GetSingleton().WriteToLog("  - mJoy: Pov Moved\n");
	return true;
}
bool HFrameListener::axisMoved(const OIS::JoyStickEvent &ev, int axis){

	GDebugger::GetSingleton().WriteToLog("  - mJoy: Axis Moved: %d of %d\n",axis,mJoy->axes());

	if(axis==0)mRotX = -1;
	else mRotX = 1;
//	ev.state.buttons
	//	GDebugger::GetSingleton().WriteToLog("  - mJoy: Axis Moved: %d of %d\n",axis,mJoy->axes());
//	for( std::vector<OIS::Axis>::const_iterator i = ev.state.mAxes.begin(); i!=ev.state.mAxes.end(); ++i ){
//		GDebugger::GetSingleton().WriteToLog("  - mJoy: Axis abs value: %f\n",(*i).abs);
//		GDebugger::GetSingleton().WriteToLog("  - mJoy: Axis rel value: %f\n",(*i).rel);
//	}
		
	return true;
}
bool HFrameListener::sliderMoved(const OIS::JoyStickEvent &ev, int sliderID){
	GDebugger::GetSingleton().WriteToLog("  - mJoy: Slider Moved\n");
	return true;
}
bool HFrameListener::buttonPressed( const OIS::JoyStickEvent &arg, int button ){
	GDebugger::GetSingleton().WriteToLog("  - mJoy: Button Pressed\n");
	return true;
}
bool HFrameListener::buttonReleased( const OIS::JoyStickEvent &arg, int button ){
	GDebugger::GetSingleton().WriteToLog("  - mJoy: Button Release\n");
	return true;
}
bool HFrameListener::processUnbufferedKeyInput(const FrameEvent& evt)
{
	//bool ret = ExampleFrameListener::processUnbufferedKeyInput(evt);

	using namespace OIS;

	
	if(_bFreeLook || !_bLevelLoaded){
		if(mKeyboard->isKeyDown(KC_A))
			mTranslateVector.x = -mMoveScale;	// Move camera left

		if(mKeyboard->isKeyDown(KC_D))
			mTranslateVector.x = mMoveScale;	// Move camera RIGHT

		if(mKeyboard->isKeyDown(KC_UP) || mKeyboard->isKeyDown(KC_W) )
			mTranslateVector.z = -mMoveScale;	// Move camera forward

		if(mKeyboard->isKeyDown(KC_DOWN) || mKeyboard->isKeyDown(KC_S) )
			mTranslateVector.z = mMoveScale;	// Move camera backward
	}  else {
		_hkLevel->processUnbufferedKeyInput(mKeyboard,evt);
	}
	if(mKeyboard->isKeyDown(KC_PGUP))
		mTranslateVector.y = mMoveScale;	// Move camera up

	if(mKeyboard->isKeyDown(KC_PGDOWN))
		mTranslateVector.y = -mMoveScale;	// Move camera down

	if( mKeyboard->isKeyDown(KC_ESCAPE) || mKeyboard->isKeyDown(KC_Q) )
		return false;

	if( !_bLevelLoaded && mKeyboard->isKeyDown(KC_L) && mTimeUntilNextToggle <= 0 ){
		HKLevelDescrip tmp;
		tmp._ogreCamera = mCamera;
		_hkLevel = new HKLevel(tmp);
		_hkLevel->Load("level001.xml");
		_bLevelLoaded = true;
		
		if(GRakNet::GetSingleton().IsServer()){
			_hkTracker->AttachCharacterController(_hkLevel->GetCharacterController());
		}
	}
	if( mKeyboard->isKeyDown(KC_LEFT) && mTimeUntilNextToggle <= 0 ){
		mRotX = 0.01;
	}
	if( mKeyboard->isKeyDown(KC_RIGHT) && mTimeUntilNextToggle <= 0 ){
		mRotX = -0.01;
	}
   	if( mKeyboard->isKeyDown(KC_F) && mTimeUntilNextToggle <= 0 )
	{
		mStatsOn = !mStatsOn;
		showDebugOverlay(mStatsOn);
		mTimeUntilNextToggle = 1;

		if(!mStatsOn)_olArabDebug->hide();
		else _olArabDebug->show();
	}
	if( mKeyboard->isKeyDown(KC_N) && mTimeUntilNextToggle <= 0 )
	{	
		_bFreeLook = !_bFreeLook;
		mTimeUntilNextToggle = 1;
		// _hkCharController->SetFreeLook(_bFreeLook);
	}
	if( mKeyboard->isKeyDown(KC_T) && mTimeUntilNextToggle <= 0 )
	{
		switch(mFiltering)
		{
		case TFO_BILINEAR:
			mFiltering = TFO_TRILINEAR;
			mAniso = 1;
			break;
		case TFO_TRILINEAR:
			mFiltering = TFO_ANISOTROPIC;
			mAniso = 8;
			break;
		case TFO_ANISOTROPIC:
			mFiltering = TFO_BILINEAR;
			mAniso = 1;
			break;
		default: break;
		}
		MaterialManager::getSingleton().setDefaultTextureFiltering(mFiltering);
		MaterialManager::getSingleton().setDefaultAnisotropy(mAniso);

		showDebugOverlay(mStatsOn);
		mTimeUntilNextToggle = 1;
	}

	if(mKeyboard->isKeyDown(KC_SYSRQ) && mTimeUntilNextToggle <= 0)
	{
		std::ostringstream ss;
		ss << "screenshot_" << ++mNumScreenShots << ".png";
		mWindow->writeContentsToFile(ss.str());
		mTimeUntilNextToggle = 0.5;
		mDebugText = "Saved: " + ss.str();
	}

	if(mKeyboard->isKeyDown(KC_R) && mTimeUntilNextToggle <=0)
	{
		mSceneDetailIndex = (mSceneDetailIndex+1)%3 ;
		switch(mSceneDetailIndex) {
			case 0 : mCamera->setPolygonMode(PM_SOLID); break;
			case 1 : mCamera->setPolygonMode(PM_WIREFRAME); break;
			case 2 : mCamera->setPolygonMode(PM_POINTS); break;
		}
		mTimeUntilNextToggle = 0.5;
	}

	static bool displayCameraDetails = false;
	if(mKeyboard->isKeyDown(KC_P) && mTimeUntilNextToggle <= 0)
	{
		displayCameraDetails = !displayCameraDetails;
		mTimeUntilNextToggle = 0.5;
		if (!displayCameraDetails)
			mDebugText = "";
	}

	// Print camera details
	if(displayCameraDetails)
		mDebugText = "P: " + StringConverter::toString(mCamera->getDerivedPosition()) +
					 " " + "O: " + StringConverter::toString(mCamera->getDerivedOrientation());

	// see if switching is on, and you want to toggle
	if (mInputTypeSwitchingOn && mKeyboard->isKeyDown(OIS::KC_M) && mTimeUntilNextToggle <= 0)
	{
		switchMouseMode();
		mTimeUntilNextToggle = 1;
	}
	if (mInputTypeSwitchingOn && mKeyboard->isKeyDown(OIS::KC_G) && mTimeUntilNextToggle <= 0)
	{
		
		_iShadowSwitch++;
		switch(_iShadowSwitch){
			case 0:
				mSceneMgr->setShadowTechnique(SHADOWTYPE_NONE);
				_sShadowType = "None";
				break;
			case 1:
				mSceneMgr->setShadowTechnique(SHADOWDETAILTYPE_ADDITIVE);
				_sShadowType = "Additive";
				break;
			case 2:
				mSceneMgr->setShadowTechnique(SHADOWDETAILTYPE_MODULATIVE);
				_sShadowType = "Modulative";
				break;
			case 3:
				mSceneMgr->setShadowTechnique(SHADOWDETAILTYPE_INTEGRATED);
				_sShadowType = "Integrated";
				break;
			case 4:
				mSceneMgr->setShadowTechnique(SHADOWDETAILTYPE_STENCIL);
				_sShadowType = "Stencil";
				break;
			case 5:
				mSceneMgr->setShadowTechnique(SHADOWTYPE_STENCIL_MODULATIVE);
				_sShadowType = "Stencil_Additive";
				break;
			case 6:
				mSceneMgr->setShadowTechnique(SHADOWTYPE_STENCIL_ADDITIVE);
				_sShadowType = "Stencil_Modulative";
				break;
			case 7:
				mSceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_MODULATIVE);
				_sShadowType = "Texture_Modulative";
				break;
			case 8:
				mSceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_ADDITIVE);
				_sShadowType = "Texture_Additive";
				break;
			case 9:
				mSceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_ADDITIVE_INTEGRATED);
				_sShadowType = "Texture_Additive_Integ";
				break;
			case 10:
				mSceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_MODULATIVE_INTEGRATED);
				_sShadowType = "Texture_Mod_Integ";
				break;
			default: {
				_iShadowSwitch = 0;
				mSceneMgr->setShadowTechnique(SHADOWTYPE_NONE);
				_sShadowType = "None";
			 }
		}
		mTimeUntilNextToggle = 0.2;
	}
	if (mInputTypeSwitchingOn && mKeyboard->isKeyDown(OIS::KC_X) && mTimeUntilNextToggle <= 0)
	{
		
		//Assign new primitive descriptor
		{
			HKPrimitiveDescrip descrip;	
			
			Vector3 pos = mCamera->getPosition();
			Vector3 dir = mCamera->getDirection();
			Vector3 spawnPos = dir * 0.1 + pos; 

			char buffer[256];
			sprintf(buffer,"Sphere_%d_",nObj++);

			descrip._sID = buffer;
			descrip._iPhysType			= HKObjectDescrip::OBJ_PHYS_SPHERE;
			descrip._iPrimitiveType		= HKPrimitiveDescrip::PRIMITIVE_SPHERE;
			descrip._vPos = Vector3(spawnPos.x,spawnPos.y+10,pos.z);
			descrip._ogreCamera = mCamera;
			_hkLevel->AddPrimitive(descrip);
		}

		mTimeUntilNextToggle = 0.2;
	}
	if (mKeyboard->isKeyDown(OIS::KC_Y) && mTimeUntilNextToggle <= 0){
		mCamera->yaw(Ogre::Degree(20));//,Ogre::Node::TS_WORLD);
		mTimeUntilNextToggle = 0.2;
	}
	if (mKeyboard->isKeyDown(OIS::KC_B) && mTimeUntilNextToggle <= 0){
		char buffer[256];
		sprintf(buffer,"Ray_%d_",nObj++);
	
		Ogre::String str = buffer;
		ManualObject* myManualObject =  mSceneMgr->createManualObject(str + "object"); 
		SceneNode* myManualObjectNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(str + "_node"); 

		MaterialPtr myManualObjectMaterial = MaterialManager::getSingleton().create(str + "manual1Material","debugger"); 
		myManualObjectMaterial->setReceiveShadows(false); 
		myManualObjectMaterial->getTechnique(0)->setLightingEnabled(true); 
		myManualObjectMaterial->getTechnique(0)->getPass(0)->setDiffuse(0,0,1,0); 
		myManualObjectMaterial->getTechnique(0)->getPass(0)->setAmbient(0,0,1); 
		myManualObjectMaterial->getTechnique(0)->getPass(0)->setSelfIllumination(0,0,1); 

		myManualObject->begin(str + "manual1Material", Ogre::RenderOperation::OT_LINE_LIST); 
		Ogre::Ray ray = mCamera->getCameraToViewportRay(0.5,0.5);
		
		Vector3 dir = ray.getDirection();
		Vector3 o = ray.getOrigin();
		double t = 0.5;

		myManualObject->position(o); 
		myManualObject->position(o + dir * t); 
		// etc 
		myManualObject->end(); 

		myManualObjectNode->attachObject(myManualObject);
	}

	if (mKeyboard->isKeyDown(OIS::KC_F1) && mTimeUntilNextToggle <= 0){
		//PlaySound("./Media/sounds/m4Single.wav",NULL,SND_ASYNC);	
		mTimeUntilNextToggle = 0.2;
	}
	if (mInputTypeSwitchingOn && mKeyboard->isKeyDown(OIS::KC_V) && mTimeUntilNextToggle <= 0)
	{
		{
			HKPrimitiveDescrip descrip;	
			
			Vector3 pos = mCamera->getPosition();
			Vector3 dir = mCamera->getDirection();
			Vector3 spawnPos = dir * 0.1 + pos; 

			char buffer[256];
			sprintf(buffer,"Box_%d_",nObj++);

			descrip._sID = buffer;
			descrip._vPos = Vector3(spawnPos.x,spawnPos.y+10,pos.z);
			descrip._vScale = Vector3(0.1f,0.1f,0.1f);
			descrip._sMeshName = "crate.mesh";
			descrip._sMaterialName = "Material/SOLID/Crate";
			descrip._iPhysType			= HKObjectDescrip::OBJ_PHYS_CUBE;
			descrip._iPrimitiveType		= HKPrimitiveDescrip::PRIMITIVE_CUBE;

			descrip._ogreCamera = mCamera;
			_hkLevel->AddPrimitive(descrip);
		}
		mTimeUntilNextToggle = 0.2;
	}
	if (mInputTypeSwitchingOn && mKeyboard->isKeyDown(OIS::KC_Z) && mTimeUntilNextToggle <= 0)
	{
	
		//_hkFactory->RemoveAllObjects();
		mTimeUntilNextToggle = 1;
	}
	if (mInputTypeSwitchingOn && mKeyboard->isKeyDown(OIS::KC_K) && mTimeUntilNextToggle <= 0)
	{

		HKPrimitive::Create();
		// must be going from immediate keyboard to buffered keyboard
		//switchKeyMode();
		mTimeUntilNextToggle = 1;
	}
	if (mInputTypeSwitchingOn && mKeyboard->isKeyDown(OIS::KC_J) && mTimeUntilNextToggle <= 0)
	{
		bool accept = _hkTracker->GetAcceptInput();

		//Toggle Tracker Input
		_hkTracker->SetAcceptInput(!accept);

		//If we were accepting input lets reset the movement speed
		if(accept){
			_hkLevel->GetCharacterController()->SetMovementSpeed(20.0f);
		}
		mTimeUntilNextToggle = 1;
	}
	return true;
}
void HFrameListener::requestShutdown(void)
{
  mShutdownRequested = true;
}

void HFrameListener::switchMouseMode()
{
	mUseBufferedInputMouse = !mUseBufferedInputMouse;

	if(GRakNet::GetSingleton().IsServer()){
		if(!mUseBufferedInputMouse)_cursor->SetPosition(Vector3(mWindow->getWidth()/2,mWindow->getHeight()/2,0));
	}

	mMouse->setBuffered(mUseBufferedInputMouse);
}
//-------------------------------------------------------------------------------------
void HFrameListener::switchKeyMode()
{
	mUseBufferedInputKeys = !mUseBufferedInputKeys;
	mKeyboard->setBuffered(mUseBufferedInputKeys);
}

bool HFrameListener::frameEnded(const FrameEvent& evt)
{
  if (mShutdownRequested)
     return false;
  else {
     updateShadowStats();
	 return ExampleFrameListener::frameEnded(evt);
  }
}

//----------------------------------------------------------------//
bool HFrameListener::mouseMoved( const OIS::MouseEvent &arg )
{
	CEGUI::System::getSingleton().injectMouseMove( arg.state.X.rel, arg.state.Y.rel );
	if(mUseBufferedInputMouse && mHeldObject != NULL){
		Vector3 pos = mHeldObject->getPosition();
		pos.z += (-arg.state.Y.rel) * 0.2;
		mHeldObject->setPosition(pos);
	}
	return true;
}

//----------------------------------------------------------------//
bool HFrameListener::mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
	// Left mouse button down, point to object
    if (id == OIS::MB_Left) {     
		//GDebugger::GetSingleton().WriteToLog("Left Mouse Pressed: ");
		
		if(!mHeldObject){
			//GDebugger::GetSingleton().WriteToLog("No object held.\n");

			// Setup the ray scene query
			CEGUI::Point mousePos = CEGUI::MouseCursor::getSingleton().getPosition();
			Ray mouseRay = mCamera->getCameraToViewportRay(mousePos.d_x/float(arg.state.width), mousePos.d_y/float(arg.state.height));
			mRaySceneQuery->setRay(mouseRay);
			mRaySceneQuery->setSortByDistance(true);

			// Execute query
			RaySceneQueryResult &result = mRaySceneQuery->execute();
			RaySceneQueryResult::iterator itr;

			int ct = 0;
			for (itr = result.begin(); itr != result.end(); itr++)
			{
				
				GDebugger::GetSingleton().WriteToLog("  - Iter[%d]: ",ct++);
				if(itr->movable){
					GDebugger::GetSingleton().WriteToLog("movable. ");
					
					if(itr->movable->getParentSceneNode()->getName().substr(0,6).compare("Sphere") == 0 || 
					   itr->movable->getParentSceneNode()->getName().substr(0,3).compare("Box") == 0){
						mHeldObject = itr->movable->getParentSceneNode();
						mHeldObject->showBoundingBox(true);
						mHeldEnt = Ogre::any_cast<HKActor*>(itr->movable->getUserAny());
						mHeldEnt->SetGrab(true);
						GDebugger::GetSingleton().WriteToLog("Name: %s\n",mHeldObject->getName().substr(0,6).c_str());
						break;
					}
				} else if (itr->worldFragment){
					GDebugger::GetSingleton().WriteToLog("worldFrag.\n");
				} else {
					GDebugger::GetSingleton().WriteToLog("No type?\n");
				}
			}
		} else {
			//GDebugger::GetSingleton().WriteToLog("Object Held.  Name: %s\n",mHeldObject->getName());
		}
	} else {
		GDebugger::GetSingleton().WriteToLog("Right Mouse Pressed\n");
		CEGUI::System::getSingleton().injectMouseButtonDown(convertOISMouseButtonToCegui(id));
	}
	return true;
}

//----------------------------------------------------------------//
bool HFrameListener::mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
	if (id == OIS::MB_Left)
    { 
		if(mHeldObject)mHeldObject->showBoundingBox(false);
		mHeldObject = NULL;

		if(mHeldEnt)mHeldEnt->SetGrab(false);
		mHeldEnt = NULL;
	}
	else {
		CEGUI::System::getSingleton().injectMouseButtonUp(convertOISMouseButtonToCegui(id));
	}
	return true;
}

//----------------------------------------------------------------//
bool HFrameListener::keyPressed( const OIS::KeyEvent &arg )
{
	if( arg.key == OIS::KC_ESCAPE )
		mShutdownRequested = true;
	CEGUI::System::getSingleton().injectKeyDown( arg.key );
	CEGUI::System::getSingleton().injectChar( arg.text );
	return true;
}

//----------------------------------------------------------------//
bool HFrameListener::keyReleased( const OIS::KeyEvent &arg )
{
	if( arg.key == OIS::KC_M )
		mMouse->setBuffered( !mMouse->buffered() );
	else if( arg.key == OIS::KC_K )
		mKeyboard->setBuffered( !mKeyboard->buffered() );
	CEGUI::System::getSingleton().injectKeyUp( arg.key );
	return true;
}
bool HFrameListener::processUnbufferedMouseInput(const FrameEvent& evt)
{
	using namespace OIS;

	// Rotation factors, may not be used if the second mouse button is pressed
	// 2nd mouse button - slide, otherwise rotate
	const MouseState &ms = mMouse->getMouseState();
	if( ms.buttonDown( MB_Right ) )
	{
		mTranslateVector.x += ms.X.rel * 0.13;
		mTranslateVector.y -= ms.Y.rel * 0.13;
	}
	else
	{
		mRotX = Degree(-ms.X.rel)* 0.2;
		mRotY = Degree(-ms.Y.rel)* 0.2;
	}

	return true;
}
bool HFrameListener::processUnbufferedJoyInput(const FrameEvent& evt)
{
	using namespace OIS;

	const JoyStickState &js = mJoy->getJoyStickState();


	float minVal = -31000,maxVal = 31000;
	for(int i = 0; i < js.mAxes.size(); i++){
		if(js.mAxes[i].abs < minVal && i==1){
			//GDebugger::GetSingleton().WriteToLog("  - Left\n");
			mRotX = mRotateSpeed * evt.timeSinceLastFrame;
		} else if(js.mAxes[i].abs < minVal && i==0){
			//GDebugger::GetSingleton().WriteToLog("  - up\n");
		}else if(js.mAxes[i].abs > maxVal && i==1){
			mRotX = -mRotateSpeed * evt.timeSinceLastFrame;
			//GDebugger::GetSingleton().WriteToLog("  - right\n");
		}else if(js.mAxes[i].abs > maxVal && i==0){
			//GDebugger::GetSingleton().WriteToLog("  - down\n");
		}
		//GDebugger::GetSingleton().WriteToLog("  - Unbuffered Axis[%d]:\n",i);
		//GDebugger::GetSingleton().WriteToLog("     - Type: %d\n", js.mAxes[i].cType);
		//GDebugger::GetSingleton().WriteToLog("     - Abs: %d\n",js.mAxes[i].abs);
		//GDebugger::GetSingleton().WriteToLog("     - Rel: %d\n",js.mAxes[i].rel);
	}
	return true;
}

void HFrameListener::UpdateOffAxis(){
	mCamera->setCustomProjectionMatrix(true, _mOffAxis);
}