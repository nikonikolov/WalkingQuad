/* 

Leg Abstraction Class: Corresponds to a physical leg. Constructed from 4 Servojoints. 
===========================================================================================

Stores information about the parameters of the leg such as:

DIST_CENTER 		Distance from robot's center to the center of ARM servo
COXA 			Distance from the center of ARM servo to the center of HIP servo
FEMUR 			Distance from the center of HIP servo to the center of KNEE servo
TIBIA 			Distance from the center of KNEE servo to END EFFECTOR
hip_to_end		Direct distance from hip mount point to the point where the end effector touches ground
HEIGHT 			Vertical distance from HIP mount point to ground
arm_ground_to_ef		Horizontal distance from the center of ARM servo's ground point projection to the END EFFECTOR
ANGLEOFFSET 	Angle between Y-axis(Robot orientation) and servo orientation; always a positive quantity
HIPKNEEMAXHDIST Maximum horizontal distance between center of HIP and center of KNEE so that torque on KNEE servo is in range
KNEE_TO_MOTOR_DIST 	Horizontal distance between center of KNEE and center of MOTOR - needed for calculating Quadcopter ARM angle

-------------------------------------------------------------------------------------------

FUNCTIONALITY:
	1. Can be only controlled from the Tripod Class 
	2. Responsible for automatic leg adjustment during a movement, i.e. calculating the required angle for each ServoJoint
	3. Does not write calculated values to the motors: this action is invoked from the Tripod for coordination
	4. The class does computations for LEFT LEG only. Values for RIGHT LEG are computed at the time of writing as opposite to Left Leg
	5. ServoAgnles[] stores values to be written to servo. These would usually be different from values you need for computations
	6. Although servo_angles[] is HARDLY used for computing state, be careful if you need to use it

-------------------------------------------------------------------------------------------

FRAMEWORK:
	1. Algorithms work only for LEFT LEG. Values for RIGHT LEG are computed at the time of writing as opposite to Left Leg

	2. vars[]		-	Use updateVar(). Function automatically updates square value for the variable
							and the rest of the variables by calling StateUpdate()
	3. updateVar() 		- 	When you use this to update hip_to_end it is assumed the change is in ENDEFFECTOR
							rather than in the HEIGHT of the robot. If change is in the HEIGHT, the vars.height
							will not be updates
	4. configureAngles()	-	Function automatically called when updateVar() is called. Basing on vars[]
							function computes new angles for Hip and Knee
	5. center()			-	Assumes Leg is already lifted up, otherwise robot will probably fall down
	6. configureVars() 	- 	Computes all vars[] based on the current servo_angles[] does not use Update(), but computes variables
							manually to ensure no call to configureAngles()

-------------------------------------------------------------------------------------------

*/

#ifndef LEG_H
#define LEG_H

#include <cmath>

#include "ServoJoint.h"
#include "State_t.h"
#include "wkq.h"
using wkq::RobotState_t;

class Leg{

public:

#ifndef DOF3	
	Leg(int ID_knee, int ID_arm, unordered_map<int, DnxHAL*>& servo_map, double height_in, const BodyParams& robot_params);
#else
	Leg(int ID_knee, int ID_hip, int ID_arm, unordered_map<int, DnxHAL*>& servo_map, double height_in, const BodyParams& robot_params);
#endif
	~Leg();

	/* ---------------------------------------- GETTER AND COPY ---------------------------------------- */

	double get(int param_type, int idx) const;
	void copyState(const Leg& leg_in);								// Copy the state of input Leg

	/* ---------------------------------------- STATIC POSITIONS ---------------------------------------- */

	void setPosition(wkq::RobotState_t robot_state);

	/* ---------------------------------------- RAISE AND LOWER ---------------------------------------- */

	void prepareLiftUp(){
		state.servo_angles.knee = wkq::PI;
	}

	void liftUp(double height);				// Lift End Effector in the air
	void lowerDown(double height);			// Put End Effector straight down
	void finishStep();						// Put End Effector down with ARM, HIP and KNEE centered

	/* ---------------------------------------- WALKING ALGORITHMS ---------------------------------------- */

	void bodyForward(double step_size);	// Change angles and state of Leg for a step forward
	void stepForward(double step_size);		// Put End Effector down by making a step forward. Leg must be already lifted

	void bodyForwardRectangularGait(double step_size);
	void stepForwardRectangularGait(double step_size);

	void bodyRotate(double angle);			// Change angles and state of Leg for a rotation around central axis
	void stepRotate(double angle);			// Put End Effector down by making a rotation step. Leg must be already lifted
	
	void raiseBody(double hraise);

	/* ---------------------------------------- WRITE TO SERVOS ---------------------------------------- */

	void writeAngles();							// Write servo_angles[] to physcial servos in order ARM, HIP, KNEE
	void writeJoint(int idx);			// Write only a single angle contained in servo_angles[] to physcial servo

	template<typename MemberFnPtr, typename FnArg>
	void setServosParam(MemberFnPtr fn, FnArg arg, bool extra=false);

private:
	void confRectangular();
	void confQuadArms();

	/* ============================================== MEMBER DATA ============================================== */

	State_t state;
	LegJoints joints;							// Stores servo objects corresponding to the physical servos

	double angle_offset;						// Angle between Y-axis and servo orientation; always positive
	bool leg_right;								// 1.0 - Leg is LEFT, -1.0 - Leg is RIGHT
	wkq::LegID leg_id; 							// ID of the leg

	bool debug_ = false;


	bool first_step_taken = false;
};


template<typename MemberFnPtr, typename FnArg>
void Leg::setServosParam(MemberFnPtr fn, FnArg arg, bool extra/*=false*/){
    if(extra){
#ifdef DOF3
	    (joints.arm.*fn)(arg, extra);
#endif
	    (joints.hip.*fn)(arg, extra);
	    (joints.knee.*fn)(arg, extra);
    } 	
    else{
#ifdef DOF3
	    (joints.arm.*fn)(arg);
#endif
    	(joints.hip.*fn)(arg);
	    (joints.knee.*fn)(arg);
    } 		
}


#endif


