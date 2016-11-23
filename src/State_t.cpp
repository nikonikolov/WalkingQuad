#include "State_t.h"

/*
// @to do: Fix the values for the limits - nothing confirmed

// @Note: Values are valid for a left leg and correspond to values that are written to the servo
LegAngles State_t::angle_limits_max(wkq::radians(0), wkq::radians(90-20), wkq::radians(90-20));
LegAngles State_t::angle_limits_min(wkq::radians(150), wkq::radians(-(90-20)), wkq::radians(-(90-20)));
*/

// Initialize static varialbes
LegAngles State_t::default_pos_angles;
DynamicVars State_t::default_pos_vars;
bool State_t::default_pos_calculated = false;


/*  @ Notes:
    Tripod and Leg constructors do not write to servo_angles, so State constrcutor is only responsible for initializing to meaningless
    state and calculating the defaultPos if not calculated already
*/
State_t::State_t(double height_in, const BodyParams& robot_params) : /*servo_angles { 0.0 }, vars{ 0.0 },*/ params(robot_params) {

    // Calculate defaultPoss only if not calculated already - note defaultPoss are static members for the class
    if(!default_pos_calculated){
        default_pos_calculated = true;

        // Find defaultPos servo_angles for Hip and Knee. Automatically computes the state and sets the height
        centerAngles(height_in);

        // Save the default variables and servo_angles
        default_pos_vars = vars;                
        default_pos_angles = servo_angles;              

        // Initialize servo_angles and vars to meaningless state
        servo_angles = 0.0;
        vars = 0.0;
    }
}

State_t::~State_t(){}


void State_t::operator=(const State_t& obj_in){
    if(this!=&obj_in){
        this->vars = obj_in.vars;               
        this->servo_angles = obj_in.servo_angles;               
    }
}


/* -------------------------------------------- STATIC POSITIONS -------------------------------------------- */

void State_t::legDefaultPos(){  
    vars = default_pos_vars;                
    servo_angles = default_pos_angles;              
}

void State_t::legCenter(){
    //servo_angles.wing = default_pos_angles.wing;
    servo_angles.arm = default_pos_angles.arm;
    centerAngles();
}

void State_t::legStand(){
    //setAngles((wkq::PI)/2, 0.0, 0.0);       // Automatically calls computevars[] and computes new state
}

void State_t::legFlatten(){ 
/*    servo_angles.knee=0.0;
    servo_angles.hip=0.0;
    // ARM and WING are either set to 0.0 by defaultPos or are not to be changed
    clear();
*/
}


/* ================================================= MAINTAINING LEG STATE ================================================= */

void State_t::updateVar(double* address, double value, bool update_state /*=true*/){
    
    *address = value;

    if     (address == &(vars.hip_to_end))           vars.hip_to_end_sq = pow(value, 2);
    else if(address == &(vars.height))               vars.height_sq = pow(value, 2);
    else if(address == &(vars.arm_ground_to_ef))     vars.arm_ground_to_ef_sq = pow(value, 2);
    else if(address == &(vars.ef_center))            vars.ef_center_sq = pow(value, 2);
    else if(address == &(vars.hip_to_end_sq))        vars.hip_to_end = sqrt(value);
    else if(address == &(vars.height_sq))            vars.height = sqrt(value);
    else if(address == &(vars.arm_ground_to_ef_sq))  vars.arm_ground_to_ef = sqrt(value);
    else if(address == &(vars.ef_center_sq))         vars.ef_center = sqrt(value);
    else                                             printf("ERROR in State_t::updateVar\n\r");

    if(update_state) update(address);  
}


void State_t::updateVar(double* address, double value, double* address_sq, double value_sq){
    *address = value;
    *address_sq = value_sq;
    update(address);    
}


void State_t::update(double* address){
/*    if      (address == &(vars.hip_to_end))
        // Assumed that change is in END EFFECTOR and only arm_ground_to_ef is affected
        updateVar(&vars.arm_ground_to_ef, sqrt(vars.hip_to_end_sq - vars.height_sq) + params.COXA, false);
    
    else if(address == &(vars.arm_ground_to_ef))
        // Only hip_to_end is affected
        updateVar(&vars.hip_to_end_sq, pow((vars.arm_ground_to_ef - params.COXA),2) + vars.height_sq, false);
    
    else if(address == &(vars.height)){
        // Only ef_center and hip_to_end affected. arm_ground_to_ef is not affected by height change
        vars.hip_to_end_sq = pow((vars.arm_ground_to_ef - params.COXA),2) + vars.height_sq;
        vars.hip_to_end = sqrt(vars.hip_to_end_sq); 
        
        vars.ef_center = vars.arm_ground_to_ef + params.DIST_CENTER;
        vars.ef_center_sq = pow(vars.ef_center, 2); 

    }
    
    else printf("ERROR in State_t::update\n\r");

    // All parameters that can be changed require the update of HIP and KNEE servo_angles
    updateAngles();         
*/
}


// Update KNEE and HIP servo_angles basing on the current hip_to_end and HEIGHT
void State_t::updateAngles(){
/*
    // Cosine Rule to find new Knee Servo Angle
    double knee_tmp = acos( (params.FEMUR_SQ + params.TIBIA_SQ - vars.hip_to_end_sq ) / ( 2 * params.FEMUR * params.TIBIA) );
    // Convert to actual angle for the servo
    servo_angles.knee = wkq::PI - knee_tmp;                                                 // Input valid for a LEFT LEG

    // Cosine Rule to find new Hip Servo Angle
    double hip_tmp = acos( (params.FEMUR_SQ + vars.hip_to_end_sq - params.TIBIA_SQ ) / ( 2 * params.FEMUR * vars.hip_to_end) );
    // Convert to actual angle for the servo
    servo_angles.hip = (wkq::PI)/2 - hip_tmp - acos(vars.height/vars.hip_to_end);       // Input valid for a LEFT LEG
*/
}


void State_t::clear(){
    vars =  -1.0;
}


/*  @ Notes:
    height should be 0.0 when the current vars.height should be used; if this variable has not been set to a meaningful value, 
    the height that needs to be used should be passed as parameter - computeEFVars() will set the value in StateVar[HEIGHT]
*/
void State_t::centerAngles(double height /*=0.0*/){
    servo_angles.arm = 0.0;

#ifndef DOF3

    if(height == 0.0) height = vars.height;

    if      (height < params.MIN_HEIGHT) height = params.MIN_HEIGHT;
    else if (height > params.MAX_HEIGHT) height = params.MAX_HEIGHT;

    servo_angles.knee = wkq::PI/2 - acos( height / params.TIBIA);

    computeEFVars(height);          // Update vars - needs to be passed the same argument

#else

    if(height == 0.0) height = vars.height;

    if      (height < params.MIN_HEIGHT) height = params.MIN_HEIGHT;
    else if (height > params.MAX_HEIGHT) height = params.MAX_HEIGHT;

    servo_angles.hip = asin( (height - params.TIBIA) / params.FEMUR );
    servo_angles.knee = wkq::PI/2 - servo_angles.hip;

    computeEFVars(height);          // Update vars - needs to be passed the same argument

#endif
}


void State_t::setAngles(double knee, double hip, double arm){
/*    servo_angles.knee = knee;
    servo_angles.hip = hip;
    servo_angles.arm = arm;
    computeVars();
*/
}


/*  @ Notes:
    Computes valid vars basing on servo_angles and assumes all vars can be defined
*/
void State_t::computeVars(){
/*
    double knee_height = params.TIBIA*cos(wkq::radians(90) - fabs(servo_angles.hip) - (wkq::PI - servo_angles.knee) );
    
    double height = knee_height - params.FEMUR*sin(fabs(servo_angles.hip));
    updateVar(&vars.height, height, false);                  // Only updates HEIGHT
    
    double hip_to_end_sq = pow(params.FEMUR,2) + pow(params.TIBIA,2) - 2*params.FEMUR*params.TIBIA*cos(wkq::PI - servo_angles.knee);
    updateVar(&vars.hip_to_end_sq, hip_to_end_sq, false);        // Only updates hip_to_end

    double arm_g_to_end = sqrt(vars.hip_to_end_sq-vars.height_sq) + params.COXA;
    updateVar(&vars.arm_ground_to_ef, arm_g_to_end, false);          // Only updates arm_ground_to_ef

    // Note that servo_angles are already centered so ef_center will be fine
    updateVar(&vars.ef_center, vars.arm_ground_to_ef+params.DIST_CENTER, false);
*/
}


void State_t::computeEFVars(double height /*=0.0*/){
/*    if(height!=0.0) updateVar(&vars.height, height, false);  // Only updates HEIGHT

    double hip_to_end_sq = pow(params.FEMUR,2) + pow(params.TIBIA,2) - 2*params.FEMUR*params.TIBIA*cos(wkq::PI - servo_angles.knee);
    updateVar(&vars.hip_to_end_sq, hip_to_end_sq, false);        // Only updates hip_to_end

    double arm_g_to_end = sqrt(vars.hip_to_end_sq-vars.height_sq) + params.COXA;
    updateVar(&vars.arm_ground_to_ef, arm_g_to_end, false);          // Only updates arm_ground_to_ef

    // Note that servo_angles are already centered so ef_center will be fine
    if(height!=0.0) updateVar(&vars.ef_center, vars.arm_ground_to_ef+params.DIST_CENTER, false);
*/
}


/*
void State_t::Verify(){
    return;
    // Best to call this function at the end of each manipulation function after all variables have been configured to be true
    // This way you make sure no errors that don't exist are not found and by calling from the function you know what modifications
    // you need to do in order to correct the state


    for(int i=0; i<JOINT_COUNT; i++){
        if(servo_angles.i<angle_limits[2*i]) throw wkq::leg_state_t(2*i);
        if(servo_angles.i>angle_limits[2*i+1]) throw wkq::leg_state_t(2*i+1);
    }

    for(int i=0; i<VAR_STEP; i++){
        if(vars.i<0) throw wkq::leg_state_t(/*state vars less than 0*);
    }

    // Triangle rule check
    double tri_side = vars.arm_ground_to_ef - params.COXA;
    if( vars.hip_to_end > (tri_side + vars.height) )    throw wkq::leg_state_t(leg_state_vars_broken);
    if( vars.height > (tri_side + vars.hip_to_end) )    throw wkq::leg_state_t(leg_state_vars_broken);
    if( tri_side > (vars.hip_to_end + vars.height) )    throw wkq::leg_state_t(leg_state_vars_broken);


    // angle + var check
    // arm_ground_to_ef<femur+tibia
    // torque check - is knee close enough - give it 20% freedom

}
*/
