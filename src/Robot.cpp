#include "Robot.h"

const double Robot::wait_time_ = 0.1;
//const double Robot::wait_time_ = 1;
const double Robot::ef_raise_ = State_t::ef_raise;

Robot::Robot(Master* pixhawk_in, unordered_map<int, DnxHAL*>& servo_map, double height_in, const BodyParams& robot_params, wkq::RobotState_t state_in /*= wkq::RS_DEFAULT*/) :
	Tripods{
		Tripod(wkq::KNEE_LEFT_FRONT, wkq::KNEE_RIGHT_MIDDLE, wkq::KNEE_LEFT_BACK, servo_map, height_in, robot_params),
		Tripod(wkq::KNEE_RIGHT_FRONT, wkq::KNEE_LEFT_MIDDLE, wkq::KNEE_RIGHT_BACK, servo_map, height_in, robot_params)
	}, 
	pixhawk(pixhawk_in), state(state_in){
	
	if(debug_) printf("ROBOT start\n\r");
	
	// Calculate max size for movements
	max_step_size = 		State_t::max_step_size;
	max_rotation_angle = 	State_t::max_rotation_angle;

	if(debug_) printf("ROBOT calculating state\n\r");

	// init to meaningless state
	state = wkq::RS_FLAT_QUAD;
	setState(state_in);

	if(debug_) printf("ROBOT done\n\r");
}

Robot::~Robot(){}


/* ================================================= STATIC POSITIONS ================================================= */

void Robot::setState(wkq::RobotState_t state_in, bool wait_call/*=false*/){
	for(int i=0; i<TRIPOD_COUNT; i++){
		Tripods[i].setPosition(state_in);
		if(wait_call) wait(wait_time_);
	}
	state = state_in;
}

/* ================================================= WALK RELATED FUNCTIONALITY ================================================= */

void Robot::makeMovement(RobotMovement_t movement, double coeff){
	double move_arg;
	bool continue_movement;
	int tripod_up, tripod_down;

	void (Tripod::*move_body)(double);
	void (Tripod::*make_step)(double);
	void (Tripod::*finish_step)();

	if(coeff < -1.0) coeff = -1.0;
	if(coeff > 1.0)  coeff = 1.0;

	continue_movement 	= true;
	//tripod_up 			= TRIPOD_RIGHT;
	//tripod_down 		= TRIPOD_LEFT;
	tripod_up 			= TRIPOD_LEFT;
	tripod_down 		= TRIPOD_RIGHT;

	switch(movement){
		case wkq::RM_HEXAPOD_GAIT:
			move_body 		= &Tripod::bodyForward;
			make_step 		= &Tripod::stepForward;
			finish_step 	= &Tripod::finishStep;
			move_arg  		= coeff*max_step_size;
			break;
		case wkq::RM_RECTANGULAR_GAIT:
			move_body 		= &Tripod::bodyForwardRectangularGait;
			make_step 		= &Tripod::stepForwardRectangularGait;
			finish_step 	= &Tripod::finishStepRectangularGait;
			move_arg 		= coeff*max_step_size;
			break;
		case wkq::RM_ROTATION_HEXAPOD:
			move_body	 	= &Tripod::bodyRotate;
			make_step 		= &Tripod::stepRotate;
			finish_step 	= &Tripod::finishStep;
			move_arg 		= coeff*max_rotation_angle;
			break;
		case wkq::RM_ROTATION_RECTANGULAR:
			break;
		default:
			printf("ERROR - Robot::makeMovement - movement not implemented\n\r");
			return;
	}

	bool first = true;
	int i=1;
	// repeat until walkForward signal stops
	while(continue_movement){
		// Read input and find out whether movement should go on
		if(!first) continue_movement = pixhawk->inputWalkForward();

		Tripods[tripod_up].liftUp(ef_raise_);
		wait(wait_time_);

		if(first || !continue_movement){
		//if(first){
			first = false;
			(Tripods[tripod_down].*move_body)(move_arg);
		} 	
		else{
			(Tripods[tripod_down].*move_body)(2*move_arg);
		} 
		//wait(wait_time_);

		// Movement goes on
		if(continue_movement){
			/*
			if(first){
				(Tripods[tripod_up].*make_step)(move_arg);
				first = false;		
			}
			else{
				(Tripods[tripod_up].*make_step)(2*move_arg);
			}	
			*/
			(Tripods[tripod_up].*make_step)(2*move_arg);
			std::swap(tripod_up, tripod_down);			// swap the roles of the Tripods
			wait(wait_time_);
		}
		
		// Movement stops
		else{
			(Tripods[tripod_up].*finish_step)();
			wait(wait_time_);
			Tripods[tripod_down].liftUp(ef_raise_);
			wait(wait_time_);
			(Tripods[tripod_down].*finish_step)();
		}
		i++;
	}
}


void Robot::raiseBody(double hraise){
	if(wkq::compare_doubles(0.0, hraise)) return;
	Tripods[TRIPOD_LEFT].raiseBody(hraise);
	Tripods[TRIPOD_RIGHT].copyState(Tripods[TRIPOD_LEFT]);
}



/*
void Robot::writeAngles(){
	for(int i=0; i<TRIPOD_COUNT; i++){
		Tripods[i].writeAngles(distance);
	}
}
*/

/* ================================================= TESTING METHODS ================================================= */


void Robot::test(){
	//Tripods[TRIPOD_LEFT].liftUp(ef_raise_);
}

void Robot::testSingleTripodStand(){
	Tripods[TRIPOD_LEFT].liftUp(ef_raise_);
}

void Robot::singleStepForwardTest(double coeff){
	if(coeff<0.0) coeff=0.1;
	if(coeff>1.0) coeff=1.0;

	double step_size = coeff*max_step_size;

	bool continue_movement = true;
	int tripod_up = TRIPOD_LEFT, tripod_down = TRIPOD_RIGHT;
		
	wait(wait_time_);
	Tripods[tripod_up].liftUp(ef_raise_);
	wait(wait_time_);
	if(debug_) printf("First tripod lifted\n\r");
	Tripods[tripod_down].bodyForward(step_size);
	wait(wait_time_);
	if(debug_) printf("Second tripod moved body forward\n\r");

	Tripods[tripod_up].stepForward(step_size);
	if(debug_) printf("First tripod put down legs for another step forward\n\r");
}


/* ================================================= ROS COMMUNICATION ================================================= */

/*
void Robot::decodeInstruction(wkq_msgs::RPC::Request &req, wkq_msgs::RPC::Request &res){

	switch(RPC_Fn_t(req.fn)){
		case RPC_DEFAULT_POS:
			break;
		case RPC_CENTER:
			break;
		case RPC_STAND:
			break;
		case RPC_STAND_QUAD:
			break;
		case RPC_STRAIGHT_QUAD:	
			break;
		default:
	}

}
*/

/* ================================================= PRIVATE METHODS ================================================= */

bool Robot::noState(){
	if(state==wkq::RS_FLAT_QUAD) return true;
	if(state==wkq::RS_QUAD_SETUP) return true;
	return false;
}

