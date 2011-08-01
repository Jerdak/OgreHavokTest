#include "TransformationHistory.h"
#include "RakNet3.3/Include/RakAssert.h"
#include "GDebugger.h"

TransformationHistoryCell::TransformationHistoryCell()
{

}
TransformationHistoryCell::TransformationHistoryCell(RakNetTime t, const Ogre::Vector3& pos, const Ogre::Vector3& vel, const Ogre::Quaternion& quat) :
time(t),
velocity(vel),
position(pos),
orientation(quat)
{
}

void TransformationHistory::Init(RakNetTime maxWriteInterval, RakNetTime maxHistoryTime)
{
	writeInterval=maxWriteInterval;
	maxHistoryLength = maxHistoryTime/maxWriteInterval+1;
	history.ClearAndForceAllocation(maxHistoryLength+1);
	RakAssert(writeInterval>0);
}
void TransformationHistory::Write(const Ogre::Vector3 &position, const Ogre::Vector3 &velocity, const Ogre::Quaternion &orientation, RakNetTime curTimeMS)
{
	if (history.Size()==0)
	{
		history.Push(TransformationHistoryCell(curTimeMS,position,velocity,orientation));
	}
	else
	{
		const TransformationHistoryCell &lastCell = history.PeekTail();
		if (curTimeMS-lastCell.time>=writeInterval)
		{
			history.Push(TransformationHistoryCell(curTimeMS,position,velocity,orientation));
			if (history.Size()>maxHistoryLength)
				history.Pop();
		}
	}	
}
void TransformationHistory::Overwrite(const Ogre::Vector3 &position, const Ogre::Vector3 &velocity, const Ogre::Quaternion &orientation, RakNetTime when)
{
	int historySize = history.Size();
	if (historySize==0)
	{
		history.Push(TransformationHistoryCell(when,position,velocity,orientation));
	}
	else
	{
		// Find the history matching this time, and change the values.
		int i;
		for (i=historySize-1; i>=0; i--)
		{
			TransformationHistoryCell &cell = history[i];
			if (when >= cell.time)
			{
				if (i==historySize-1 && when-cell.time>=writeInterval)
				{
					// Not an overwrite at all, but a new cell
					history.Push(TransformationHistoryCell(when,position,velocity,orientation));
					if (history.Size()>maxHistoryLength)
						history.Pop();
					return;
				}

				cell.time=when;
				cell.position=position;
				cell.velocity=velocity;
				cell.orientation=orientation;
				return;
			}
		}
	}	
}
void TransformationHistory::Read(Ogre::Vector3 *position, Ogre::Vector3 *velocity, Ogre::Quaternion *orientation,
								 RakNetTime when, RakNetTime curTime)
{
	int historySize = history.Size();
	if (historySize==0)
	{
		return;
	}

	int i;
	for (i=historySize-1; i>=0; i--)
	{
		const TransformationHistoryCell &cell = history[i];
		if (when >= cell.time)
		{
			if (i==historySize-1)
			{
				if (curTime<=cell.time)
					return;

				float divisor = (float)(curTime-cell.time);
				RakAssert(divisor!=0.0f);
				float lerp = (float)(when - cell.time) / divisor;
				if (position)
					*position=cell.position + (*position-cell.position) * lerp;
				if (velocity)
					*velocity=cell.velocity + (*velocity-cell.velocity) * lerp;
				if (orientation)
					*orientation = Ogre::Quaternion::Slerp(lerp, cell.orientation, *orientation,true);
			}
			else
			{
				const TransformationHistoryCell &nextCell = history[i+1];
				float divisor = (float)(nextCell.time-cell.time);
				RakAssert(divisor!=0.0f);
				float lerp = (float)(when - cell.time) / divisor;
				if (position)
					*position=cell.position + (nextCell.position-cell.position) * lerp;
				if (velocity)
					*velocity=cell.velocity + (nextCell.velocity-cell.velocity) * lerp;
				if (orientation)
					*orientation = Ogre::Quaternion::Slerp(lerp, cell.orientation, nextCell.orientation,true);
			}
			return;
		}
	}

	// Return the oldest one
	const TransformationHistoryCell &cell = history.Peek();
	if (position)
		*position=cell.position;
	if (orientation)
		*orientation=cell.orientation;
	if (velocity)
		*velocity=cell.velocity;
}
void TransformationHistory::Clear(void)
{
	history.Clear();
}

double TransformationHistory2::TimeSinceLastPacket(){
	_timer.Stop();
	_dLastDT2 = _timer.GetDurationInSecs();

	return _dLastDT2 - _dLastDT1;
}
void TransformationHistory2::Write(const Ogre::Vector3 &position){

	

	//Add new position to packet queue
	_qPos.push(position);

//	GDebugger::GetSingleton().WriteToLog("Time since last packet: %f\n",TimeSinceLastPacket());

	if(_nPackets >= 1 && _nPackets < 30000)
	_dAvgTime += TimeSinceLastPacket();

	//Get last received packet time;
	_timer.Stop();
	_dLastDT1 = _timer.GetDurationInSecs();



	//Only maintain a queue of the last 2 values.
	if(_qPos.size() > 1){
		_qPos.pop();
		_bIsValid = true;
	}
	if(_nPackets < 30000)_nPackets++;
}
Ogre::Vector3 TransformationHistory2::Read(double dt, Ogre::Vector3 &curPos){
	if(_bIsValid){
		Ogre::Vector3 pt2 = _qPos.front();
		_qPos.pop();

		double pkTime = 0.045;	//Because packets aren't sent unless something updates we can't count on an average measure.
		Ogre::Vector3 _vVelocity;

		if(pkTime <= 0) _vVelocity = Ogre::Vector3(0,0,0);
		else _vVelocity = (pt2 - curPos) / pkTime;
		Ogre::Vector3 ret = curPos + (_vVelocity * dt);
		_qPos.push(ret);
		return ret;
	}
	return curPos;
	/*
	//Don't return a new value unless you've got a full queue.
	if(_bIsValid){
		//Pop queue
		Ogre::Vector3 pt1 = _qPos.front();
		_qPos.pop();
		
		//Pop queue
		Ogre::Vector3 pt2 = _qPos.front();
		_qPos.pop();

		//Get time since last packet
		//double pkTime = _dAvgTime/(_nPackets-1);
		double pkTime = 0.03;	//Because packets aren't sent unless something updates we can't count on an average measure.
		Ogre::Vector3 _vVelocity;

		if(pkTime <= 0) _vVelocity = Ogre::Vector3(0,0,0);
		else _vVelocity = (pt2 - pt1) / pkTime;

		Ogre::Vector3 ret = pt2 + (_vVelocity * dt);

		_qPos.push(pt1);
		_qPos.push(pt2);
		GDebugger::GetSingleton().WriteToLog("Vel: %f %f  Avg: %f  nPackets: %d  Ret: %f %f %f\n",_vVelocity.x,_vVelocity.y,_dAvgTime,_nPackets,ret.x,ret.y);

		return ret;
	}*/
	return curPos;
}