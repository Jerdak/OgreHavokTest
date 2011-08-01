#ifndef __DOTSCENELOADER__
#define __DOTSCENELOADER__

#include <OgreString.h>
#include <OgreVector3.h>
#include <OgreQuaternion.h>
#include <vector>

class TiXmlElement;




class nodeProperty {
public:
	Ogre::String nodeName;
	Ogre::String propertyNm;
	Ogre::String valueName;
	Ogre::String typeName;

	nodeProperty(const Ogre::String &node, const Ogre::String &propertyName,const Ogre::String &value, const Ogre::String &type):
			nodeName(node),propertyNm(propertyName),valueName(value),typeName(type);
};

class HKDotSceneLoader {
public:
	HKDotSceneLoader():_sceneMgr(NULL){}
	virtual ~HKDotSceneLoader(){}

	void parseDotScene(const Ogre::String &sceneName, const Ogre::String &groupName, Ogre::SceneManager *pScene, Ogre::SceneNode *pNode = NULL, const Ogre::String &sPrependNode = "");
	

protected:

private:
	Ogre::SceneManager *_sceneMgr;
};



#endif //__DOTSCENELOADER__