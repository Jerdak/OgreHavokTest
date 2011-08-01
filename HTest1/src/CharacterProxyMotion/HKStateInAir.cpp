/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2008 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#include "./HKStateInAir.h"

#include <Animation/Animation/Playback/hkaAnimatedSkeleton.h>
#include <Animation/Animation/Playback/Control/Default/hkaDefaultAnimationControl.h>
#include <Animation/Animation/Mapper/hkaSkeletonMapper.h>
#include <Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxy.h>

#include "../GDebugger.h"

/// Process the input - causes state transitions and actions
/// Process the input - causes state transitions and actions
void HKStateInAir::update(HKStateMachineContext& context, const HKStateMachineInput& input, HKStateMachineOutput& output )
{
	const hkVector4 up(0.0f,1.0f,0.0f);
	const hkVector4 down(0.0f, -1.0f, 0.0f);

	if (context.m_previousState != HK_STATE_IN_AIR)
	{

	} 


	// Check if we are now supported or not
	hkpSurfaceInfo ground;
	context.m_characterProxy->checkSupport(down, ground);

	if (ground.m_supportedState == hkpSurfaceInfo::SUPPORTED)
	{
		context.m_currentState = HK_STATE_STANDING;
	}

	output.m_additionalVelocity = context.m_characterProxy->getLinearVelocity();

	// Add in some gravity 
	output.m_additionalVelocity.addMul4( input.m_stepInfo.m_deltaTime.val(), input.m_characterGravity );
	
	//Add in locomotion velocity
	output.m_additionalVelocity.addMul4(input.m_inputUD,input.m_forward);
	
	//Rotate our forward vector another 90 degrees to get the tanget vector.  Not perfect but it works quickly enough.
	hkQuaternion tmpOrient;
	tmpOrient.setAxisAngle(input.m_up,1.57079);

	hkVector4 tmpSide;
	tmpSide.setRotatedDir(tmpOrient,input.m_forward);
	output.m_additionalVelocity.addMul4(input.m_inputLR,tmpSide);
		

	//Cap our locomotive velocities
	if(output.m_additionalVelocity(0) > 20.0f) output.m_additionalVelocity(0) = 20;
	if(output.m_additionalVelocity(1) > 20.0f) output.m_additionalVelocity(1) = 20;
	if(output.m_additionalVelocity(2) > 20.0f) output.m_additionalVelocity(2) = 20;
	

	output.m_velocity = output.m_additionalVelocity; //Semantic change, we could simly have just made 1 variable in output


	// Changes to context
	context.m_previousState = HK_STATE_IN_AIR;
	//context.m_animatedSkeleton->stepDeltaTime(input.m_stepInfo.m_deltaTime);
}
