#ifndef __TRANFORMATION_HISTORY_H
#define __TRANFORMATION_HISTORY_H

#include <RakNet3.3/Include/RakNetTypes.h>
#include <OgreVector3.h>
#include <OgreQuaternion.h>
#include <RakNet3.3/Include/DS_Queue.h>
#include <RakNet3.3/Include/RakMemoryOverride.h>
#include "ProfTimer.h"
struct TransformationHistoryCell 
{
	TransformationHistoryCell();
	TransformationHistoryCell(RakNetTime t, const Ogre::Vector3& pos, const Ogre::Vector3& vel, const Ogre::Quaternion& quat  );

	RakNetTime time;
	Ogre::Vector3 position;
	Ogre::Quaternion orientation;
	Ogre::Vector3 velocity;
};

class TransformationHistory 
{
public:
	void Init(RakNetTime maxWriteInterval, RakNetTime maxHistoryTime);
	void Write(const Ogre::Vector3 &position, const Ogre::Vector3 &velocity, const Ogre::Quaternion &orientation, RakNetTime curTimeMS);
	void Overwrite(const Ogre::Vector3 &position, const Ogre::Vector3 &velocity, const Ogre::Quaternion &orientation, RakNetTime when);
	// Parameters are in/out, modified to reflect the history
	void Read(Ogre::Vector3 *position, Ogre::Vector3 *velocity, Ogre::Quaternion *orientation,
		RakNetTime when, RakNetTime curTime);
	void Clear(void);
protected:
	DataStructures::Queue<TransformationHistoryCell> history;
	unsigned maxHistoryLength;
	RakNetTime writeInterval;
};

//Added a second type of transformation history that only stores the last 2 good packets and translates between em'
class TransformationHistory2 {
public:
	TransformationHistory2(){
		_timer.Start();
		_timer.Stop();
		_dLastDT1 = 0;

		_bIsValid = false;
		_dAvgTime = 0;
		_nPackets = 0;
	}
	~TransformationHistory2(){}


	double TimeSinceLastPacket();
	void Write(const Ogre::Vector3 &position);
	Ogre::Vector3 Read(double dt, Ogre::Vector3 &curPos);

protected:
	std::queue<Ogre::Vector3> _qPos;

	double _dAvgTime;
	int _nPackets;
	ProfTimer _timer;
	double _dLastDT1,_dLastDT2;
	bool _bIsValid;
};
#endif
