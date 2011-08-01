#include "HTest1App.h"
#include "SimpleIni.h"

#include <Common/Base/hkBase.h>
#include <Physics/Dynamics/World/hkpWorld.h>
#include <Physics/Dynamics/Entity/hkpRigidBody.h>
/** This class 'wibbles' the light and billboard */
class LightWibbler : public ControllerValue<Real>
{
protected:
    Light* mLight;
    Billboard* mBillboard;
    ColourValue mColourRange;
    ColourValue mMinColour;
    Real mMinSize;
    Real mSizeRange;
    Real intensity;
public:
    LightWibbler(Light* light, Billboard* billboard, const ColourValue& minColour, 
        const ColourValue& maxColour, Real minSize, Real maxSize)
    {
        mLight = light;
        mBillboard = billboard;
        mMinColour = minColour;
        mColourRange.r = maxColour.r - minColour.r;
        mColourRange.g = maxColour.g - minColour.g;
        mColourRange.b = maxColour.b - minColour.b;
        mMinSize = minSize;
        mSizeRange = maxSize - minSize;
    }

    virtual Real  getValue (void) const
    {
        return intensity;
    }

    virtual void  setValue (Real value)
    {
        intensity = value;

        ColourValue newColour;

        // Attenuate the brightness of the light
        newColour.r = mMinColour.r + (mColourRange.r * intensity);
        newColour.g = mMinColour.g + (mColourRange.g * intensity);
        newColour.b = mMinColour.b + (mColourRange.b * intensity);

        mLight->setDiffuseColour(newColour);
        mBillboard->setColour(newColour);
        // set billboard size
        Real newSize = mMinSize + (intensity * mSizeRange);
        mBillboard->setDimensions(newSize, newSize);

    }
};

Real timeDelay = 0;
#define KEY_PRESSED(_key,_timeDelay, _macro) \
{ \
    if (mKeyboard->isKeyDown(_key) && timeDelay <= 0) \
{ \
    timeDelay = _timeDelay; \
    _macro ; \
} \
}


//---------------------------------------------------------------------------
class GaussianListener: public Ogre::CompositorInstance::Listener
{
protected:
	int mVpWidth, mVpHeight;
	// Array params - have to pack in groups of 4 since this is how Cg generates them
	// also prevents dependent texture read problems if ops don't require swizzle
	float mBloomTexWeights[15][4];
	float mBloomTexOffsetsHorz[15][4];
	float mBloomTexOffsetsVert[15][4];
public:
	GaussianListener() {}
	virtual ~GaussianListener() {}
	void notifyViewportSize(int width, int height)
	{
		mVpWidth = width;
		mVpHeight = height;
		// Calculate gaussian texture offsets & weights
		float deviation = 3.0f;
		float texelSize = 1.0f / (float)std::min(mVpWidth, mVpHeight);

		// central sample, no offset
		mBloomTexOffsetsHorz[0][0] = 0.0f;
		mBloomTexOffsetsHorz[0][1] = 0.0f;
		mBloomTexOffsetsVert[0][0] = 0.0f;
		mBloomTexOffsetsVert[0][1] = 0.0f;
		mBloomTexWeights[0][0] = mBloomTexWeights[0][1] = 
			mBloomTexWeights[0][2] = Ogre::Math::gaussianDistribution(0, 0, deviation);
		mBloomTexWeights[0][3] = 1.0f;

		// 'pre' samples
		for(int i = 1; i < 8; ++i)
		{
			mBloomTexWeights[i][0] = mBloomTexWeights[i][1] = 
				mBloomTexWeights[i][2] = Ogre::Math::gaussianDistribution(i, 0, deviation);
			mBloomTexWeights[i][3] = 1.0f;
			mBloomTexOffsetsHorz[i][0] = i * texelSize;
			mBloomTexOffsetsHorz[i][1] = 0.0f;
			mBloomTexOffsetsVert[i][0] = 0.0f;
			mBloomTexOffsetsVert[i][1] = i * texelSize;
		}
		// 'post' samples
		for(int i = 8; i < 15; ++i)
		{
			mBloomTexWeights[i][0] = mBloomTexWeights[i][1] = 
				mBloomTexWeights[i][2] = mBloomTexWeights[i - 7][0];
			mBloomTexWeights[i][3] = 1.0f;

			mBloomTexOffsetsHorz[i][0] = -mBloomTexOffsetsHorz[i - 7][0];
			mBloomTexOffsetsHorz[i][1] = 0.0f;
			mBloomTexOffsetsVert[i][0] = 0.0f;
			mBloomTexOffsetsVert[i][1] = -mBloomTexOffsetsVert[i - 7][1];
		}

	}
	virtual void notifyMaterialSetup(Ogre::uint32 pass_id, Ogre::MaterialPtr &mat)
	{
		// Prepare the fragment params offsets
		switch(pass_id)
		{
		case 701: // blur horz
			{
				// horizontal bloom
				mat->load();
				Ogre::GpuProgramParametersSharedPtr fparams = 
					mat->getBestTechnique()->getPass(0)->getFragmentProgramParameters();
				const Ogre::String& progName = mat->getBestTechnique()->getPass(0)->getFragmentProgramName();
				// A bit hacky - Cg & HLSL index arrays via [0], GLSL does not
				fparams->setNamedConstant("sampleOffsets", mBloomTexOffsetsHorz[0], 15);
				fparams->setNamedConstant("sampleWeights", mBloomTexWeights[0], 15);

				break;
			}
		case 700: // blur vert
			{
				// vertical bloom 
				mat->load();
				Ogre::GpuProgramParametersSharedPtr fparams = 
					mat->getTechnique(0)->getPass(0)->getFragmentProgramParameters();
				const Ogre::String& progName = mat->getBestTechnique()->getPass(0)->getFragmentProgramName();
				fparams->setNamedConstant("sampleOffsets", mBloomTexOffsetsVert[0], 15);
				fparams->setNamedConstant("sampleWeights", mBloomTexWeights[0], 15);

				break;
			}
		}

	}
	virtual void notifyMaterialRender(Ogre::uint32 pass_id, Ogre::MaterialPtr &mat)
	{

	}
};
GaussianListener gaussianListener;

bool HTest1App::setup(void)
{

	GDebugger::GetSingleton().WriteToLog("  - HTest1 Setup\n");

	String pluginsPath;
	// only use plugins.cfg if not static
#ifndef OGRE_STATIC_LIB
	pluginsPath = mResourcePath + "plugins.cfg";
#endif
	
	GDebugger::GetSingleton().WriteToLog("      - Root...");
    mRoot = new Root(pluginsPath, 
        mResourcePath + "ogre.cfg", mResourcePath + "Ogre.log");
	GDebugger::GetSingleton().WriteToLog("Set.\n");
	
	GDebugger::GetSingleton().WriteToLog("      - Resources...");
    setupResources();
	GDebugger::GetSingleton().WriteToLog("Set.\n");


    bool carryOn = configure();
    if (!carryOn) return false;

	GDebugger::GetSingleton().WriteToLog("      - Load ini...");
	LoadIni("HTest1.ini");
	GDebugger::GetSingleton().WriteToLog("Set.\n");

	GDebugger::GetSingleton().WriteToLog("      - Set up camera and view....");
	chooseSceneManager();
    createCamera();
    createViewports();
	GDebugger::GetSingleton().WriteToLog("Set\n");

	GDebugger::GetSingleton().WriteToLog("      - Load textures and resources....");
    // Set default mipmap level (NB some APIs ignore this)
    TextureManager::getSingleton().setDefaultNumMipmaps(5);

	// Create any resource listeners (for loading screens)
	createResourceListener();
	// Load resources
	loadResources();

	GDebugger::GetSingleton().WriteToLog("Set\n");

	GDebugger::GetSingleton().WriteToLog("      - Create scene...");
	// Create the scene
    createScene();
	GDebugger::GetSingleton().WriteToLog("Set\n");

	GDebugger::GetSingleton().WriteToLog("      - Create frame listener...");
    createFrameListener();
	GDebugger::GetSingleton().WriteToLog("Set\n");

    return true;

}
void HTest1App::createCamera(void)
{
	//Set up global off axis camera
	GOffAxisCamera::GetSingleton().LoadIni("HTest1.ini");
	

	// Create the camera
	mCamera = mSceneMgr->createCamera("PlayerCam");
	
//	mCameraNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("CameraNode");		
//	mCameraNode->attachObject(mCamera);
//	mCameraNode->setPosition(Vector3(0,0,0));
	
	// Position it at 500 in Z direction
	//mCamera->setPosition(Vector3(0,0,80));
	
	// Look back along -Z
	mCamera->lookAt(Vector3(0,0,300));
	mCamera->setNearClipDistance(0.5);
	mCamera->setFarClipDistance(100000);
	
	if(!GRakNet::GetSingleton().IsServer() &&_bOffAxis ){
		//EnableOffAxisProjection();
		
		//Enable off axis camera and let other classes know it's ready.
		GOffAxisCamera::GetSingleton().EnableOffAxisProjection(mCamera);
		GOffAxisCamera::GetSingleton().Start();
	} else {
	
		GOffAxisCamera::GetSingleton().SetCamera(mCamera);
		GOffAxisCamera::GetSingleton().Start();
	}
}
bool HTest1App::configure(void)
{
    // Show the configuration dialog and initialise the system
    // You can skip this and use root.restoreConfig() to load configuration
    // settings if you were sure there are valid ones saved in ogre.cfg
	if(mRoot->showConfigDialog())
    {
        // If returned true, user clicked OK so initialise
        // Here we choose to let the system create a default rendering window by passing 'true'
        mWindow = mRoot->initialise(true);
		// Let's add a nice window icon
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		HWND hwnd;
		mWindow->getCustomAttribute("WINDOW", (void*)&hwnd);
		LONG iconID   = (LONG)LoadIcon( GetModuleHandle(0), MAKEINTRESOURCE(IDI_APPICON) );
		SetClassLong( hwnd, GCL_HICON, iconID );
#endif
        return true;
    }
    else
    {
        return false;
    }
}


// Just override the mandatory create scene method
void HTest1App::createScene(void)
{
	try {
		GDebugger::GetSingleton().WriteToLog("  - Creating Scene....");
		// setup GUI system
		mGUIRenderer = new CEGUI::OgreCEGUIRenderer(mWindow,
		Ogre::RENDER_QUEUE_OVERLAY, false, 3000, mSceneMgr);

		mGUISystem = new CEGUI::System(mGUIRenderer);

		CEGUI::Logger::getSingleton().setLoggingLevel(CEGUI::Informative);


		// load scheme and set up defaults
		CEGUI::SchemeManager::getSingleton().loadScheme((CEGUI::utf8*)"TaharezLookSkin.scheme");
		mGUISystem->setDefaultMouseCursor((CEGUI::utf8*)"TaharezLook", (CEGUI::utf8*)"MouseArrow");
		mGUISystem->setDefaultFont((CEGUI::utf8*)"BlueHighway-12");
		
		CEGUI::MouseCursor::getSingleton().setImage("TaharezLook", "MouseTarget");
		CEGUI::MouseCursor::getSingleton().show( );
		
		setupEventHandlers();
		Ogre::SceneNode *p;

		//testScene();
		// Set ambient light
		mSceneMgr->setAmbientLight(ColourValue(0.7, 0.7, 0.7));
		
		//mSceneMgr->setAmbientLight(ColourValue(0.0,0.0,0.0));
	//	// Create a light
		Light* l = mSceneMgr->createLight("MainLight");
		l->setPosition(20,80,50);
		l->setCastShadows(true);

		Light* l2 = mSceneMgr->createLight("Sublight");
		l2->setPosition(20,80,-50);
		l2->setCastShadows(true);
		l2->setDiffuseColour(0.5,0.5,0.5);
		GHavok::GetSingleton().SetGravity(hkVector4(0,-9.82,0));
		GDebugger::GetSingleton().WriteToLog("Complete.\n");
	} catch (Ogre::Exception *ep){
		GDebugger::GetSingleton().WriteToLog("ERROR!.  %s\n",ep->getDescription().c_str());
	}
}
void HTest1App::testScene(){}

// Create new frame listener
void HTest1App::createFrameListener(void)
{
	mFrameListener= new HFrameListener(mSceneMgr, mWindow, mCamera, mGUIRenderer,mCameraNode);
	mFrameListener->showDebugOverlay(false);
	if(!GRakNet::GetSingleton().IsServer() &&_bOffAxis ){
		static_cast<HFrameListener*>(mFrameListener)->SetOffAxis(true,_mOffAxis);
	}

	mRoot->addFrameListener(mFrameListener);
}
void HTest1App::setupEventHandlers(void)
{
}

bool HTest1App::handleQuit(const CEGUI::EventArgs& e)
{
  static_cast<HFrameListener*>(mFrameListener)->requestShutdown();
  return true;
}

void HTest1App::destroyScene(){
	mSceneMgr->clearScene();
	mSceneMgr->destroyAllCameras();
}

void HTest1App::EnableOffAxisProjection(){
	GDebugger::GetSingleton().WriteToLog("   - EnableOffAxisProjection()\n");
	double pi = 3.14159;
	double dCaveFOVy = 2*atan( tan(_dCaveFOV/2 * pi/180) / mCamera->getAspectRatio() )*180/pi;
	mCamera->setFOVy(Degree(dCaveFOVy));
	
	mCamera->setCustomProjectionMatrix(false);
	double n = mCamera->getNearClipDistance();
	double f = mCamera->getFarClipDistance();
	double l, r, b, t;

	l = n*tan(_dFOVLeft*pi/180);
	r = n*tan(_dFOVRight*pi/180);
	b = n*tan(_dFOVBottom*pi/180);
	t = n*tan(_dFOVTop*pi/180);

	int sign, signOpp;
	if(l > r){
		sign = -1;
	} else {
		sign = 1;
	}
	//Small modification from original off-axis algorithm.  Signs of certain vars were changed so the Y rotations aren't flipped.
	//This also fixes the problem of translations moving in the incorrect direction.
	_mOffAxis = Ogre::Matrix4(	(2.0f*n)/(r-l),		0,					((r+l)/(r-l)),		0,
								0,					((2.0f*n)/(t-b)),	((t+b)/(t-b)),		0,
								0,					0,					-1*(f+n)/(f-n),		-1*(2.0f*f*n)/(f-n),
								0,					0,					-1,					0);
	_mOffAxis.transpose();
	mCamera->setCustomProjectionMatrix(_bOffAxis, _mOffAxis);

	GDebugger::GetSingleton().WriteToLog("  - CaveRoll: %f\n",_dCaveRoll);
	GDebugger::GetSingleton().WriteToLog("  - CavePitch: %f\n",_dCavePitch);
	GDebugger::GetSingleton().WriteToLog("  - CaveYaw: %f\n",_dCaveYaw);
	//mCameraNode->yaw(Degree(_dCaveYaw), Node::TS_WORLD);
	//mCameraNode->pitch(Degree(_dCavePitch), Node::TS_WORLD);
	//mCameraNode->roll(Degree(_dCaveRoll), Node::TS_WORLD);
	mCamera->yaw(Degree(_dCaveYaw));
	//mCamera->pitch(Degree(_dCavePitch));
	//mCamera->roll(Degree(_dCaveRoll));
}
bool HTest1App::LoadIni(char *file)
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
			
		LOOKUP("FOV",				_dCaveFOV,DOUBLE)
		ELOOKUP("EnableOffAxis",	_bOffAxis,INTEGER)
		ELOOKUP("FOVLeft",			_dFOVLeft,DOUBLE)
		ELOOKUP("FOVRight",			_dFOVRight,DOUBLE)
		ELOOKUP("FOVTop",			_dFOVTop,DOUBLE)
		ELOOKUP("FOVBottom",		_dFOVBottom,DOUBLE)
		ELOOKUP("CaveRoll",			_dCaveRoll,DOUBLE)
		ELOOKUP("CavePitch",		_dCavePitch,DOUBLE)
		ELOOKUP("CaveYaw",			_dCaveYaw,DOUBLE)
	}
	return true;
}