#include <Common/Base/hkBase.h>
#include <Common/Base/Types/Physics/hkStepInfo.h>
#include <Physics/Dynamics/Motion/hkpMotion.h>

//Animation
#include <Animation/Animation/Playback/hkaAnimatedSkeleton.h>

#ifndef HK_STATE_MACHINE
#define HK_STATE_MACHINE

enum HKStateType
{
	// default states
	HK_STATE_STANDING = 0 ,
	HK_STATE_WALKING,
	HK_STATE_IN_AIR,
	HK_STATE_RUNNING,

	HK_STATE_TOTAL_COUNT
};


// CONTEXT : Contains all persistent state information across updates on the state machine
// (contains per-instance information)
class hkaAnimatedSkeleton;
class hkaDefaultAnimationControl;
class hkpCharacterProxy;
class hkaRagdollInstance;
class hkaSkeletonMapper;
class hkaSkeleton;
class hkpWorld;
struct HKStateMachineContext
{
public:

	// Current State
	HKStateType m_currentState;

	// Previous State
	HKStateType m_previousState;

	// Animated Skeleton associated with this character
	hkaAnimatedSkeleton*	m_animatedSkeleton;

	// Different animations controls in the character
	hkaDefaultAnimationControl* m_walkControl;
	hkaDefaultAnimationControl* m_idleControl;
	hkaDefaultAnimationControl* m_protectControl;

	// Orientation (forward)
	hkReal m_currentAngle;

	// Physical world
	hkpWorld* m_world;
 
	// Physical proxy associated with this character
	hkpCharacterProxy* m_characterProxy;

	// Physical ragdoll associated with this character
	// and mappings to and from this representation
	// (Used by ragdoll demos)
	hkaRagdollInstance* m_ragdollInstance;
	hkaSkeletonMapper* m_girlToRagdollMapper;
	hkaSkeletonMapper* m_ragdollToGirlMapper;

	// An artificial force applied on dying
	hkVector4 m_launchImpulse;

	HKStateMachineContext()
	{
		m_currentState = m_previousState = HK_STATE_STANDING;
		m_animatedSkeleton = HK_NULL;
		m_walkControl =  HK_NULL;
		m_idleControl = HK_NULL;
		m_world = HK_NULL;
		m_characterProxy = HK_NULL;
		m_ragdollInstance = HK_NULL;
		m_girlToRagdollMapper = HK_NULL;
		m_ragdollToGirlMapper = HK_NULL;
		m_currentAngle = 0;
		m_launchImpulse.setZero4();

	}

	// Removes references
	~HKStateMachineContext();

};


// INPUT : Input in to the state machine (other than the current context)
struct HKStateMachineInput
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_UTILITIES, HKStateMachineInput );

	//
	// User input
	//

	/// Input X range -1 to 1 (left / right) 
	hkReal				m_inputLR;	

	/// Input Y range -1 to 1 (forward / back)
	hkReal				m_inputUD;	

	/// Set this if you want the character to try and jump
	hkBool				m_wantJump;	

	hkReal				m_maxSpeed;
	//
	// Orientation information
	//
	
	/// Up vector in world space - should generally point in the opposite direction to gravity
	hkVector4			m_up;		

	/// Forward vector in world space - point in the direction the character is facing
	hkVector4			m_forward;	

	//
	// Spatial info
	//

	/// Set this if the character is at a ladder and you want it to start to climb
	hkBool				m_atLadder;			

	/// Set this if the character is standing on a surface
	hkBool				m_isSupported;		

	/// Set this to represent the normal of the surface we're supported by
	hkVector4			m_surfaceNormal;	

	/// Set this to represent the velocity of the surface we're supported by
	hkVector4			m_surfaceVelocity;	

	/// Set this to represent the type of motion of the surface we're supported by
	hkpMotion::MotionType	m_surfaceMotionType;	


	//
	// Simulation info
	//

	/// Set this to the timestep between calls to the state machine
	hkStepInfo			m_stepInfo;

	/// Set this to the current position
	hkVector4			m_position;

	/// Set this to the current Velocity
	hkVector4			m_velocity;

	/// The gravity that is applied to the character when in the air
	hkVector4			m_characterGravity;

	/// Character speed as applied to manual input.
	hkVector4			m_maxVelocity;

	//
	// User Data
	//

	/// Tag in extra user data for new user states here
	hkUlong m_userData; // +default(0)

	HKStateMachineInput()
	{
		m_inputLR = m_inputUD = 0.0f;
		m_maxSpeed = 0.0f;
		m_wantJump = false;
		m_atLadder = false;
		m_isSupported = false;
		m_stepInfo.m_deltaTime = -1.0f; // Force the user to set this
		m_stepInfo.m_invDeltaTime = -1.0f; // Force the user to set this
		m_characterGravity.setZero4();
		m_maxVelocity.setZero4();
	}

};


// OUTPUT : Output from the state machine (other than changes to the context)
struct HKStateMachineOutput
{
	// Motion resulting during the animation
	hkQsTransform m_desiredMotion;

	// Orientation
	hkQuaternion m_orient;

	// Desired additional velocity
	hkVector4 m_additionalVelocity;

	//  Desired actual velocity.
	hkVector4 m_velocity;
	
	// Should we disable the character proxy ?
	// (Used by ragdoll demo)
	hkBool m_disableProxy;

	// Should we render the girl based on ragdoll
	hkBool m_renderRagdoll;

	// Whether the ragdoll is in a bad state (through landscape and we're trying to fix it)
	hkBool m_badRagdoll;


	HKStateMachineOutput ()
	{
		m_desiredMotion.setIdentity();
		m_orient.setIdentity();

		m_additionalVelocity.setZero4();

		m_disableProxy = false;
		m_renderRagdoll = false;
		m_badRagdoll = false;
	}

};

class HKState;


class HKStateMachine: public hkReferencedObject
{
public:

	HK_DECLARE_CLASS_ALLOCATOR(HK_MEMORY_CLASS_DEMO);

	/// Initializes the array of states to return HK_NULL for all state types
	HKStateMachine();

	// Removes references to all registered states
	~HKStateMachine();

	/// Registers a state for a given state type. This adds a reference to the registered
	/// If a state already exists for this type then the reference to the existing state
	/// is removed.
	void registerState(HKStateType stateType, HKState* state );

	/// returns the state registered for the given type
	/// If no state has been registered this returns HK_NULL
	HKState* getState (HKStateType stateType) const;

	/// Updates the state machine using the given input
	/// The output structure in initialized before being passed to the state
	void update(HKStateMachineContext& context, const HKStateMachineInput& input, HKStateMachineOutput& output);

private:

	HKState* m_registeredStates[HK_STATE_TOTAL_COUNT];

};

#endif //HK_STATE_MACHINE
