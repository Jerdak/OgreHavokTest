#ifndef __HKCURSOR__
#define __HKCURSOR__

#pragma once

#include "HKCommon.h"
#include "HKObject.h"
#include "GRakNet.h"

#include "ExampleApplication.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include "../res/resource.h"
#endif
#include <CEGUI.h>
#include <CEGUISystem.h>
#include <CEGUISchemeManager.h>
#include <OgreCEGUIRenderer.h>


class HKCursorDescrip: public HKObjectDescrip {
public:
	HKCursorDescrip():HKObjectDescrip(){
	};
	~HKCursorDescrip(){
	};
};


class HKCursor : public HKObject {
public:
	HKCursor(const HKCursorDescrip &d);
	~HKCursor();
	
	virtual HKCursorDescrip GetDescription(){return _descrip;}
	virtual void SetDescription(const HKCursorDescrip &d){_descrip = d;}

	static HKCursor *Create();

	void Show(){CEGUI::MouseCursor::getSingleton().show( );}
	void Hide(){CEGUI::MouseCursor::getSingleton().hide( );}

	//Object replication
	virtual bool SerializeConstruction(RakNet::BitStream *bitStream, RakNet::SerializationContext *serializationContext);
	virtual bool Serialize(RakNet::BitStream *bitStream, RakNet::SerializationContext *serializationContext);
	virtual void Deserialize(RakNet::BitStream *bitStream, RakNet::SerializationType serializationType, SystemAddress sender, RakNetTime timestamp);

	//Set position with exact coordinates.
	void SetRenderWindow(Ogre::RenderWindow *r){_renderWindow = r;}
	void SetPosition(const Ogre::Vector3 &v){
		_visiblePosition.x = _renderWindow->getWidth() / v.x;
		_visiblePosition.y = _renderWindow->getHeight() / v.y;
		CEGUI::MouseCursor::getSingleton().setPosition(CEGUI::Point(v.x,v.y));
	}

	virtual void Init();
	bool LoadIni(char *file);

	void Step(double dt);

private:
	HKCursorDescrip _descrip;
	Ogre::RenderWindow *_renderWindow;

	Ogre::Vector3 _position;
	Ogre::Vector3 _visiblePosition;
	Ogre::Vector3 _virtualPosition;

	Ogre::Vector3 _minPositionRange;
	Ogre::Vector3 _maxPositionRange;

	TransformationHistory _transformationHistory;
	TransformationHistory2 _transformationHistory2;

	bool _bEnableInterpolation;
	double _dTimeSinceLastUpdate;
};


#endif //__HKCURSOR__