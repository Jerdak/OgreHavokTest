#include "./HKState.h"
#include "./HKStateMachine.h"
#include <Animation/Animation/Playback/Control/Default/hkaDefaultAnimationControl.h>
#include <Animation/Animation/Playback/hkaAnimatedSkeleton.h>
#include <Animation/Animation/Mapper/hkaSkeletonMapper.h>

#include <Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxy.h>
#include <Animation/Ragdoll/Instance/hkaRagdollInstance.h>

#include "../GDebugger.h"

HKStateMachineContext::~HKStateMachineContext()
{

}



/// Initialises the array of states to return HK_NULL for all state types
HKStateMachine::HKStateMachine()
{
	for (int i=0; i < HK_STATE_TOTAL_COUNT; i++)
	{
		m_registeredStates[i] = HK_NULL;
	}

}

// Removes references to all registered states
HKStateMachine::~HKStateMachine()
{
	for (int i=0; i <HK_STATE_TOTAL_COUNT; i++)
	{
		if (m_registeredStates[i] != HK_NULL)
		{
			m_registeredStates[i]->removeReference();
			m_registeredStates[i] = HK_NULL;
		}
	}
}

/// Registers a state for a given state type. This adds a reference to the registered
/// If a state already exists for this type then the reference to the existing state
/// is removed.
void HKStateMachine::registerState(HKStateType stateType, HKState* state)
{
	state->addReference();

	HKState* oldState = m_registeredStates[stateType];

	if (oldState != HK_NULL)
	{
		oldState->removeReference();
	}

	m_registeredStates[stateType] = state; 
}

/// returns the state registered for the given type
/// If no state has been registered this returns HK_NULL
HKState* HKStateMachine::getState (HKStateType stateType) const
{
	return m_registeredStates[stateType]; 
}


/// Updates the state machine using the given input
/// The output structure in initialised before being passed to the state
void HKStateMachine::update(HKStateMachineContext& context, const HKStateMachineInput& input, HKStateMachineOutput& output)
{
	
	//GDebugger::GetSingleton().WriteToLog("Current State: %d\n",context.m_currentState);
	getState(context.m_currentState)->update(context, input, output);
}


