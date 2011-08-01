#pragma once

#include "HKCommon.h"
#include "HKLevel.h"
#include "HKCursor.h"
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

#include "HKObject.h"
#include "HKFactory.h"
#include "HKCharacterController.h"
#include "HKTracker.h"
#include "GOffAxisCamera.h"

class HFrameListener : public ExampleFrameListener, public OIS::KeyListener, public OIS::MouseListener, public OIS::JoyStickListener
{

public:
	HFrameListener(SceneManager *sceneMgr, RenderWindow* win, Camera* cam, CEGUI::Renderer* renderer, SceneNode *cameraNode);

	void DrawFloor();
	void DrawLevel();
	bool frameStarted(const FrameEvent& evt);

	bool processUnbufferedKeyInput(const FrameEvent& evt);
	bool processUnbufferedMouseInput(const FrameEvent& evt);
	bool processUnbufferedJoyInput(const FrameEvent& evt);
	void requestShutdown(void);

	void switchMouseMode();
	//-------------------------------------------------------------------------------------
	void switchKeyMode();

	bool frameEnded(const FrameEvent& evt);

	void moveCamera();
	//----------------------------------------------------------------//
	bool mouseMoved( const OIS::MouseEvent &arg );

	//----------------------------------------------------------------//
	bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
	//----------------------------------------------------------------//
	bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
	//----------------------------------------------------------------//
	bool keyPressed( const OIS::KeyEvent &arg );

	//----------------------------------------------------------------//
	bool keyReleased( const OIS::KeyEvent &arg );

	void updateShadowStats();

	bool povMoved(const OIS::JoyStickEvent &ev, int pov);
	bool axisMoved(const OIS::JoyStickEvent &ev, int axis);
	bool sliderMoved(const OIS::JoyStickEvent &ev, int axis);
	bool buttonPressed( const OIS::JoyStickEvent &arg, int button );
	bool buttonReleased( const OIS::JoyStickEvent &arg, int button );
	void SetOffAxis(bool b, Ogre::Matrix4 m){_bOffAxis = b;_mOffAxis = m;}
	void UpdateOffAxis();
private:
	Ogre::Overlay *_olArabDebug; 

	HKCursor *_cursor;
	String _sShadowType;
	SceneManager* mSceneMgr;
	SceneNode *mHeldObject;         // The newly created object
	SceneNode *mCameraNode;
	HKActor *mHeldEnt;

	RaySceneQuery *mRaySceneQuery;

	CEGUI::Renderer* mGUIRenderer;
	bool mShutdownRequested;
	bool mUseBufferedInputKeys, mUseBufferedInputMouse, mInputTypeSwitchingOn;
	bool _bFreeLook;
	bool _bLevelLoaded;
	HKLevel *_hkLevel;
	HKTracker *_hkTracker;
	int _iShadowSwitch;
	bool _bOffAxis;
	Ogre::Matrix4 _mOffAxis;
};