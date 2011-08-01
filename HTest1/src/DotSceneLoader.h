#ifndef DOT_SCENELOADER_H
#define DOT_SCENELOADER_H

// Includes
#include <OgreString.h>
#include <OgreVector3.h>
#include <OgreQuaternion.h>
#include <vector>

// Forward declarations
class TiXmlElement;

// Forward declarations

class nodeProperty
{
public:
	Ogre::String nodeName;
	Ogre::String propertyNm;
	Ogre::String valueName;
	Ogre::String typeName;

	nodeProperty(const Ogre::String &node, const Ogre::String &propertyName, const Ogre::String &value, const Ogre::String &type)
		: nodeName(node), propertyNm(propertyName), valueName(value), typeName(type) {}
};

class DotSceneLoader
{
public:
	DotSceneLoader() : mSceneMgr(0) {}
	virtual ~DotSceneLoader() {}

	void parseDotScene(const Ogre::String &SceneName, const Ogre::String &groupName, Ogre::SceneManager *yourSceneMgr, Ogre::SceneNode *pAttachNode = NULL, const Ogre::String &sPrependNode = "");
	Ogre::String getProperty(const Ogre::String &ndNm, const Ogre::String &prop);

	std::vector<nodeProperty> nodeProperties;
	std::vector<Ogre::String> staticObjects;
	std::vector<Ogre::String> dynamicObjects;

protected:
	void processScene(TiXmlElement *XMLRoot);

	void processNodes(TiXmlElement *XMLNode);
	void processExternals(TiXmlElement *XMLNode);
	void processEnvironment(TiXmlElement *XMLNode);
	void processTerrain(TiXmlElement *XMLNode);
	void processUserDataReference(TiXmlElement *XMLNode, Ogre::SceneNode *pParent = 0);
	void processUserDataReference(TiXmlElement *XMLNode, Ogre::Entity *pEntity);
	void processOctree(TiXmlElement *XMLNode);
	void processLight(TiXmlElement *XMLNode, Ogre::SceneNode *pParent = 0);
	void processCamera(TiXmlElement *XMLNode, Ogre::SceneNode *pParent = 0);

	void processNode(TiXmlElement *XMLNode, Ogre::SceneNode *pParent = 0);
	void processLookTarget(TiXmlElement *XMLNode, Ogre::SceneNode *pParent);
	void processTrackTarget(TiXmlElement *XMLNode, Ogre::SceneNode *pParent);
	void processEntity(TiXmlElement *XMLNode, Ogre::SceneNode *pParent);
	void processParticleSystem(TiXmlElement *XMLNode, Ogre::SceneNode *pParent);
	void processBillboardSet(TiXmlElement *XMLNode, Ogre::SceneNode *pParent);
	void processPlane(TiXmlElement *XMLNode, Ogre::SceneNode *pParent);

	void processFog(TiXmlElement *XMLNode);
	void processSkyBox(TiXmlElement *XMLNode);
	void processSkyDome(TiXmlElement *XMLNode);
	void processSkyPlane(TiXmlElement *XMLNode);
	void processClipping(TiXmlElement *XMLNode);

	void processLightRange(TiXmlElement *XMLNode, Ogre::Light *pLight);
	void processLightAttenuation(TiXmlElement *XMLNode, Ogre::Light *pLight);

	Ogre::String getAttrib(TiXmlElement *XMLNode, const Ogre::String &parameter, const Ogre::String &defaultValue = "");
	Ogre::Real getAttribReal(TiXmlElement *XMLNode, const Ogre::String &parameter, Ogre::Real defaultValue = 0);
	bool getAttribBool(TiXmlElement *XMLNode, const Ogre::String &parameter, bool defaultValue = false);

	Ogre::Vector3 parseVector3(TiXmlElement *XMLNode);
	Ogre::Quaternion parseQuaternion(TiXmlElement *XMLNode);
	Ogre::ColourValue parseColour(TiXmlElement *XMLNode);
	

	Ogre::SceneManager *mSceneMgr;
	Ogre::SceneNode *mAttachNode;
	Ogre::String m_sGroupName;
	Ogre::String m_sPrependNode;
};


#endif // DOT_SCENELOADER_H
