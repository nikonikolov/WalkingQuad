#include "State_t.h"


/* ===================================================== PUBLIC METHODS ===================================================== */

/*
const double State_t::Params[PARAM_COUNT] = { 10.95, 2.65, 17.5, 30.0, 12.0, 2.25,
										pow(10.95,2), pow(2.65,2), pow(17.5,2), pow(30.0,2), pow(12.0,2), pow(2.25,2)	};
*/

double State_t::Params[PARAM_COUNT] = { 0.0 };

// Values are valid for a left leg and correspond to values that are written to the servo
const double State_t::AngleLimits[JOINT_COUNT*2] = { 	wkq::radians(0), wkq::radians(150),					/* KNEE */ 
														wkq::radians(-(90-20)), wkq::radians(90-20),		/* HIP - JUST A GUESS */
														wkq::radians(-(90-20)), wkq::radians(90-20),		/* ARM - JUST A GUESS */
														//wkq::radians(-150), wkq::radians(150)				/* WING */
													};

double State_t::DefaultVars[VAR_COUNT] 	= { 0.0 };
double State_t::DefaultAngles[JOINT_COUNT] = { 0.0 };

bool State_t::defaults_calculated = false;


/* 	
	Tripod and Leg constructors do not write to angles, so State constrcutor is only responsible for initializing to meaningless
	 state and calculating the defaults if not already
*/
State_t::State_t(double height_in, const double robot_params[]) : ServoAngles { 0.0 }, Vars{ 0.0 } {

	// Calculate Defaults only if not calculated already - note defaults are static members for the class
	if(!defaults_calculated){
		defaults_calculated = true;

		// Initialize the parameters
		for(int i=0; i<PARAM_COUNT; i++){
			if(i<PARAM_STEP) Params[i] = robot_params[i];
			else Params[i] = pow(robot_params[i-PARAM_STEP],2);
		}

		// Find Default ServoAngles for Hip and Knee. Automatically computes the state and sets the height
		CenterAngles(height_in);

		// Save Default Variables
		for(int i=0; i<VAR_COUNT; i++){
			DefaultVars[i] = Vars[i]; 				
		}

		// Save Default Angles
		for(int i=0; i<JOINT_COUNT; i++){
			DefaultAngles[i] = ServoAngles[i];
		}

		// Initialize ServoAngles to meaningless(center) state
		for(int i=0; i<JOINT_COUNT; i++){
			ServoAngles[i]=0.0;
		}
	
		// Initialize Vars to meaningless values
		for(int i=0; i<VAR_COUNT; i++){
			Vars[i] = 0.0;
		}
	}
}

State_t::~State_t(){}

/* -------------------------------------------- GETTER AND COPY -------------------------------------------- */


double State_t::Get(int param_type, int idx) const{
	if 		(param_type==SERVO_ANGLE) 	return ServoAngles[idx];
	else if (param_type==STATE_VAR) 	return Vars[idx];
	else if (param_type==DEFAULT_VAR) 	return DefaultVars[idx];
	else if (param_type==DEFAULT_ANGLE) return DefaultAngles[idx];
	else if (param_type==PARAM) 		return Params[idx];
	else if (param_type==ANGLE_LIMIT) 	return AngleLimits[idx];
	else return 0.0;
}

void State_t::operator=(const State_t& StateIn){
	if(this!=&StateIn){
		for(int i=0; i<JOINT_COUNT; i++){
			ServoAngles[i] = StateIn.ServoAngles[i];
		}
		for(int i=0; i<VAR_COUNT; i++){
			Vars[i] = StateIn.Vars[i];
		}
	}
}


/* -------------------------------------------- STANDING POSITIONS -------------------------------------------- */

void State_t::LegDefault(){	
	// Change State Variables
	for(int i=0; i<VAR_COUNT; i++){
		Vars[i] = DefaultVars[i];
	}
	// Change State Angles
	for(int i=0; i<JOINT_COUNT; i++){
		ServoAngles[i] = DefaultAngles[i];
	}
}

void State_t::LegCenter(){
	//ServoAngles[WING] = DefaultAngles[WING];
	ServoAngles[ARM] = DefaultAngles[ARM];
	CenterAngles();
}

void State_t::LegStand(){
	SetAngles((wkq::PI)/2, 0.0, 0.0);		// Automatically calls ComputeVars[] and computes new state
}

void State_t::LegFlatten(){	
	ServoAngles[KNEE]=0.0;
	ServoAngles[HIP]=0.0;
	// ARM and WING are either set to 0.0 by default or are not to be changed
	Clear();
}


/* ================================================= MAINTAINING LEG STATE ================================================= */


void State_t::UpdateVar(int idx, double value, const bool& update_state /*=true*/){
	
	Vars[idx] = value;

	double base_idx = idx;
	// Provided argument is square
	if(idx>=VAR_STEP){
		Vars[idx-VAR_STEP] = sqrt(Vars[idx]);
		base_idx = idx-VAR_STEP;
	} 	

	// Provided argument is base number
	else Vars[idx+VAR_STEP] = pow(Vars[idx], 2);

	if(update_state) Update(base_idx);	
}

void State_t::UpdateVar(int idx, double value, double valueSQ){
	Vars[idx] = value;
	Vars[idx+VAR_COUNT] = valueSQ;
	Update(idx);	
}


void State_t::Update(int idx){

	switch(idx){
		case HIPTOEND:
			// Assumed that change is in END EFFECTOR and only ARMGTOEND is affected
			UpdateVar(ARMGTOEND, sqrt(Vars[HIPTOEND_SQ]-Vars[HEIGHT_SQ]) + Params[COXA], false);
			break;
		case ARMGTOEND:
			// Only HIPTOEND is affected
			UpdateVar(HIPTOEND_SQ, pow((Vars[ARMGTOEND]-Params[COXA]),2) + Vars[HEIGHT_SQ], false);
			break;
		case HEIGHT:
			// Only EFCENTER and HIPTOEND affected. ARMGTOEND is not affected by height change
			UpdateVar(HIPTOEND_SQ, pow((Vars[ARMGTOEND]-Params[COXA]),2) + Vars[HEIGHT_SQ], false);
			UpdateVar(EFCENTER, Vars[ARMGTOEND]+Params[DISTCENTER], false);
			break;
		//default:	// Good idea is to throw exception or signal somehow
	}

	// All parameters that can be changed require the update of HIP and KNEE angles
	UpdateAngles();			
}


// Update KNEE and HIP angles basing on the current HIPTOEND and HEIGHT
void State_t::UpdateAngles(){

	// Cosine Rule to find new Knee Servo Angle
	double KneeTmp = acos( (Params[FEMUR_SQ] + Params[TIBIA_SQ] - Vars[HIPTOEND_SQ] ) / ( 2 * Params[FEMUR] * Params[TIBIA]) );
	// Convert to actual angle for the servo
	ServoAngles[KNEE] = wkq::PI - KneeTmp;													// Input valid for a LEFT LEG

	// Cosine Rule to find new Hip Servo Angle
	double HipTmp = acos( (Params[FEMUR_SQ] + Vars[HIPTOEND_SQ] - Params[TIBIA_SQ] ) / ( 2 * Params[FEMUR] * Vars[HIPTOEND]) );
	// Convert to actual angle for the servo
	ServoAngles[HIP] = (wkq::PI)/2 - HipTmp - acos(Vars[HEIGHT]/Vars[HIPTOEND]);		// Input valid for a LEFT LEG
}





void State_t::Clear(){
	for(int i=0; i<VAR_COUNT; i++){
		Vars[i]=-1.0;
	}
}

/*
void State_t::Verify(){
	return;
	// Best to call this function at the end of each manipulation function after all variables have been configured to be true
	// This way you make sure no errors that don't exist are not found and by calling from the function you know what modifications
	// you need to do in order to correct the state


	for(int i=0; i<JOINT_COUNT; i++){
		if(ServoAngles[i]<AngleLimits[2*i]) throw wkq::leg_state_t(2*i);
		if(ServoAngles[i]>AngleLimits[2*i+1]) throw wkq::leg_state_t(2*i+1);
	}

	for(int i=0; i<VAR_STEP; i++){
		if(Vars[i]<0) throw wkq::leg_state_t(/*state vars less than 0*);
	}

	// Triangle rule check
	double tri_side = Vars[ARMGTOEND] - Params[COXA];
	if( Vars[HIPTOEND] > (tri_side + Vars[HEIGHT]) )	throw wkq::leg_state_t(leg_state_vars_broken);
	if( Vars[HEIGHT] > (tri_side + Vars[HIPTOEND]) )	throw wkq::leg_state_t(leg_state_vars_broken);
	if( tri_side > (Vars[HIPTOEND] + Vars[HEIGHT]) )	throw wkq::leg_state_t(leg_state_vars_broken);


	// angle + var check
	// armgtoend<femur+tibia
	// torque check - is knee close enough - give it 20% freedom

}
*/

/* 	height should be 0.0 when the current StateVar[HEIGHT] should be used; if this variable has not been set to a meaningful value, 
	the height that needs to be used should be passed as parameter - ComputeEFVars() will set the value in StateVar[HEIGHT]
*/
void State_t::CenterAngles(double height /*=0.0*/){
	// Center HIP
	double hip_max=asin(Params[HIPKNEEMAXHDIST]/Params[FEMUR]);
	ServoAngles[HIP] = ( hip_max + ( wkq::radians(90) - fabs(AngleLimits[HIP_MIN]) ) )/2 - wkq::radians(90);
	
	ServoAngles[HIP] = AngleLimits[HIP_MIN];

	// Center KNEE
	double height_hip;
	if(height==0.0) height_hip = Vars[HEIGHT];
	else height_hip = height;

	double height_knee = height_hip + Params[FEMUR]*sin(fabs(ServoAngles[HIP]));
	double knee_angle = wkq::PI/2 - fabs(ServoAngles[HIP]) - acos(height_knee/Params[TIBIA]);
	// if the argument of TIBIA relative to the ground is not acute but obtuse:
	if(knee_angle<0.0) knee_angle = wkq::PI/2 - fabs(ServoAngles[HIP]) + acos(height_knee/Params[TIBIA]);

	ServoAngles[KNEE] = wkq::PI - knee_angle;

	ComputeEFVars(height);			// Update Vars[] - needs to be passed the same argument
}

//void State_t::SetAngles(double knee, double hip, double arm, double wing){
void State_t::SetAngles(double knee, double hip, double arm){
	ServoAngles[KNEE] = knee;
	ServoAngles[HIP] = hip;
	ServoAngles[ARM] = arm;
//	ServoAngles[WING] = wing;
	ComputeVars();
}



// Computes valid Vars[] basing on ServoAngles[] and assumes all Vars[] can be defined
void State_t::ComputeVars(){
	double knee_height = Params[TIBIA]*cos(wkq::radians(90) - fabs(ServoAngles[HIP]) - (wkq::PI - ServoAngles[KNEE]) );
	
	double height = knee_height - Params[FEMUR]*sin(fabs(ServoAngles[HIP]));
	UpdateVar(HEIGHT, height, false);					// Only updates HEIGHT
	
	double hip_to_end_sq = pow(Params[FEMUR],2) + pow(Params[TIBIA],2) - 2*Params[FEMUR]*Params[TIBIA]*cos(wkq::PI - ServoAngles[KNEE]);
	UpdateVar(HIPTOEND_SQ, hip_to_end_sq, false); 		// Only updates HIPTOEND

	double arm_g_to_end = sqrt(Vars[HIPTOEND_SQ]-Vars[HEIGHT_SQ]) + Params[COXA];
	UpdateVar(ARMGTOEND, arm_g_to_end, false); 			// Only updates ARMGTOEND

	// Note that angles are already centered so EFCENTER will be fine
	UpdateVar(EFCENTER, Vars[ARMGTOEND]+Params[DISTCENTER], false);
}


void State_t::ComputeEFVars(double height /*=0.0*/){
	if(height!=0.0) UpdateVar(HEIGHT, height, false); 	// Only updates HEIGHT

	double hip_to_end_sq = pow(Params[FEMUR],2) + pow(Params[TIBIA],2) - 2*Params[FEMUR]*Params[TIBIA]*cos(wkq::PI - ServoAngles[KNEE]);
	UpdateVar(HIPTOEND_SQ, hip_to_end_sq, false); 		// Only updates HIPTOEND

	double arm_g_to_end = sqrt(Vars[HIPTOEND_SQ]-Vars[HEIGHT_SQ]) + Params[COXA];
	UpdateVar(ARMGTOEND, arm_g_to_end, false); 			// Only updates ARMGTOEND

	// Note that angles are already centered so EFCENTER will be fine
	if(height!=0.0) UpdateVar(EFCENTER, Vars[ARMGTOEND]+Params[DISTCENTER], false);
}
