/* 

Walking Quadcopter Namespace
===========================================================================================

	Used to define some functions and types specific to the robot without introducing name clashes

-------------------------------------------------------------------------------------------

*/

#ifndef wkq_NAMESPACE_H
#define wkq_NAMESPACE_H

#include <cmath>

namespace wkq{


	const double PI = 3.14159265358979323846264338327950288419716939937510;

	inline double radians(double degrees){	
		return degrees/180.0*wkq::PI; 
	}

	inline double degrees(double radians){
		return radians/wkq::PI*180.0;
	}


	/* 	SERVO NAMES: Number corresponds to ID of the servo
	 	NOTE: Try to keep IDs the same. Changing them will cause bugs in Tripod constructor parameters 
	 	and you will also have to change the ID of the physical servo */

	// SERIAL TX-9 RX-10 - KNEES
	const int knee_left_front =			11;				// M7
	const int knee_left_middle =		12;				// M9
	const int knee_left_back =			13;				// M1
	const int knee_right_front =		14; 			// M11
	const int knee_right_middle =		15;				// M6
	const int knee_right_back =			16; 			// M5

	// SERIAL TX-9 RX-10 - HIPS
	const int hip_left_front =			17; 			// M4
	const int hip_left_middle =			18; 			// M2
	const int hip_left_back =			19;				// M10
	const int hip_right_front =			20; 			// M12
	const int hip_right_middle =		21; 			// M8
	const int hip_right_back =			22; 			// M3

	// SERIAL TX-13 RX-14 - WINGS
	const int wing_left_front =			23; 			// M18
	const int wing_left_middle =		24; 			// M13
	const int wing_left_back =			25; 			// M14
	const int wing_right_front =		26; 			// M17
	const int wing_right_middle =		27; 			// M16
	const int wing_right_back =			28; 			// M15

	// SERIAL TX-13 RX-14 - ARMS
	const int arm_left_front =			29; 			// M18
	const int arm_left_middle =			30; 			// M13
	const int arm_left_back =			31; 			// M14
	const int arm_right_front =			32; 			// M17
	const int arm_right_middle =		33; 			// M16
	const int arm_right_back =			34; 			// M15

	const int knees_start = 			11;
	const int hips_start = 				17;
	const int wings_start = 			23;
	const int arms_start = 				29;

	// LIMITS FOR ROTATION ANGLES FOR SERVO
	const int knee_min =				0;
	const int knee_max =				1024;
	const int hip_min =					0;
	const int hip_max =					1024;
	const int wing_min =				0;
	const int wing_max =				1024;	
	const int arm_min =					0;
	const int arm_max =					1024;


	
	/* *** NOTE: Be very careful when introducing new states - if the new state is meaningless in terms of leg configuration
			on the ground, i.e. backwards consistency with current position does no need to be kept, you need to signify that
			somehow in Robot::stand()
	***/
	enum RobotState_t{
		RS_DEFAULT 				= 0,			// all servos configured to default positions, i.e. centralized for stable stand
		RS_STANDING				= 1,			// body height equal to knee height, all knees to 90 degrees, the rest centered
		RS_CENTERED				= 2,			// body height equal to knee height, all knees to 90 degrees, the rest centered
		RS_STANDING_QUAD 		= 4, 			// standing with legs configured in Quad mode and configured to stand on the ground
		RS_FLAT_QUAD			= 5,			// standing with legs configured in Quad mode and fully flat
		RS_QUAD_SETUP			= 6, 			// Legs set so that pixhawk configuration can be perfromed 

		RS_FLY_STANDING_QUAD 	= 20, 			// Flying with legs configured in Quad mode but not fully flat
		RS_FLY_STRAIGHT_QUAD 	= 21, 			// Flying with legs configured in Quad mode and fully flat
		/*
		RS_idle 		= 0,			// not doing anything
		RS_in_flight 	= 1,			// flying with legs fully flat
		RS_walking 		= 2,			// walking
		RS_landing 		= 3,			// currently landing - knees on 90 degrees for landing
		RS_error 		= 4,			// not sure what is happening
		RS_hex			= 6,			// not completely defined yet
		RS_quad			= 7,			// not completely defined yet
		*/
	}; 


	enum LegStatus_t{
		LS_KNEE_MAX_OVERFLOW 	= 0,
		LS_KNEE_MIN_OVERFLOW 	= 1,
		LS_HIP_MAX_OVERFLOW 	= 2,
		LS_HIP_MIN_OVERFLOW 	= 3,
		LS_ARM_MAX_OVERFLOW 	= 4,
		LS_ARM_MIN_OVERFLOW 	= 5,
		LS_WING_MAX_OVERFLOW 	= 6,
		LS_WING_MIN_OVERFLOW 	= 7,

	}; 


/* ------------------------------------------------- POINT ------------------------------------------------- */ 

	class Point{

	public:
		Point();
		Point(double x_in, double y_in);
		Point(double x_in, double y_in, double mod_in, double arg_in);
		Point(const Point& p_in);
		~Point();
		
		inline double get_x() const;
		inline double get_y() const;
		inline double get_mag() const;
		inline double get_arg() const;
		
		inline double origin_dist() const;
		//void origin_symmetric();

		inline void translate_y(double delta_y);
		inline void translate_x(double delta_x);
		inline void translate(const Point& p2);
		
		inline double dist(const Point& p_in) const;
		inline double dist_sq(const Point& p_in) const;
	
		void set_arg(double arg_in);
		void set_mag(double mag_in);
	/*
		friend bool operator<(const Point& p1, const Point& p2);
		friend bool operator==(const Point& p1, const Point& p2);
	*/

	private:
		void rect_coord();

		double x = 0;
		double y = 0;

		double mag = 0;
		double arg = 0;
	};


	double Point::get_x() const{
		return x;
	}
	double Point::get_y() const{
		return y;
	}
	double Point::get_arg() const{
		return arg;
	}
	double Point::get_mag() const{
		return mag;
	}

	double Point::origin_dist() const{
		return sqrt(pow(x,2) + pow(y,2));
	}

	double Point::dist(const Point& p_in) const{
		return sqrt( pow((x-p_in.x),2) + pow((y-p_in.y),2) );
	}

	double Point::dist_sq(const Point& p_in) const{
		return pow((x-p_in.x),2) + pow((y-p_in.y),2);
	}

	void Point::translate_y(double delta_y){
		y += delta_y;
	}

	void Point::translate_x(double delta_x){
		x += delta_x;
	}


/* ------------------------------------------------- END POINT ------------------------------------------------- */ 



}


#endif

