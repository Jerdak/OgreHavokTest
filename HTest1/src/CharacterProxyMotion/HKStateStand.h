#ifndef HK_STAND_STATE_H
#define HK_STAND_STATE_H

#include "./HKState.h"

class HKStateStand : public HKState
{
public:

	HK_DECLARE_CLASS_ALLOCATOR(HK_MEMORY_CLASS_DEMO);

	/// Return the state type
	virtual HKStateType getType() const {return HK_STATE_STANDING;}

	/// Process the input - causes state transitions and actions
	virtual void update(HKStateMachineContext& context, const HKStateMachineInput& input, HKStateMachineOutput& output );

private:

	// We keep it as a local member since we only use this state machine with a single instance; otherwise, we would keep this
	// on an array for each instance
	hkReal m_timeUnsupported;

};

#endif // HK_STAND_STATE_H
