/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2008 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#include "./HKStateStand.h"

#include <Animation/Animation/Playback/hkaAnimatedSkeleton.h>
#include <Animation/Animation/Playback/Control/Default/hkaDefaultAnimationControl.h>
#include <Animation/Animation/Mapper/hkaSkeletonMapper.h>
#include <Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxy.h>

#include "../GDebugger.h"

/// Process the input - causes state transitions and actions
/// Process the input - causes state transitions and actions
void HKStateStand::update(HKStateMachineContext& context, const HKStateMachineInput& input, HKStateMachineOutput& output )
{
	const hkVector4 up(0.0f,1.0f,0.0f);
	const hkVector4 down(0.0f,-1.0f, 0.0f);

	if (context.m_previousState!=HK_STATE_STANDING)
	{
		//context.m_idleControl->easeIn(1.0f);
	}

	// Check if we are now supported or not
	hkpSurfaceInfo ground;
	context.m_characterProxy->checkSupport(down, ground);
	if (ground.m_supportedState != hkpSurfaceInfo::SUPPORTED)
	{
		m_timeUnsupported += input.m_stepInfo.m_deltaTime;
		output.m_additionalVelocity.setMul4(context.m_characterProxy->getLinearVelocity().dot3(up), up);

			// Add in some gravity 
		output.m_additionalVelocity.addMul4( input.m_stepInfo.m_deltaTime.val(), input.m_characterGravity );
	}
	else
	{
		m_timeUnsupported = 0.0f;
	}


	if (m_timeUnsupported>0.1f)
	{
		// We are now in the air
		context.m_currentState = HK_STATE_IN_AIR;
		//context.m_idleControl->easeOut(1.0f);
	}
	else
	{
		if (input.m_inputUD != 0.0f || input.m_inputLR != 0.0f)
		{
			// Start walking
			context.m_currentState = HK_STATE_WALKING;
			//context.m_idleControl->easeOut(0.5f);
		}
		if (input.m_wantJump){
			output.m_additionalVelocity.addMul4(3,hkVector4(0,1,0));
		}
	}


	// Prepare Output
	//context.m_animatedSkeleton->getDeltaReferenceFrame(input.m_stepInfo.m_deltaTime, output.m_desiredMotion);

	// Add extra vertical rotation depending on the left-right input
	hkReal angle = -0.03f * input.m_inputLR;

	hkQuaternion extraRotation (up, angle);
	output.m_desiredMotion.m_rotation.mul(extraRotation);
	
	// Changes to context
	context.m_previousState = HK_STATE_STANDING;
	output.m_velocity = output.m_additionalVelocity;
	//context.m_animatedSkeleton->stepDeltaTime(input.m_stepInfo.m_deltaTime);

}
