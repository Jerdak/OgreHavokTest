
#ifndef HK_STATE
#define HK_STATE

#include "./HKStateMachine.h"


class HKState : public hkReferencedObject
{
public:

	HK_DECLARE_CLASS_ALLOCATOR(HK_MEMORY_CLASS_DEMO);

	/// Virtual destructor
	virtual ~HKState() {}

	/// Return the state type
	virtual HKStateType getType() const = 0;

	/// Process the input - causes state transitions and actions
	virtual void update(HKStateMachineContext& context, const HKStateMachineInput& input, HKStateMachineOutput& output ) = 0;

};

#endif // HK_STATE
