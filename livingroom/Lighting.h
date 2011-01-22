#ifndef LIGHTING_H
#define LIGHTING_H

#include <WProgram.h>
#include "Servo.h"

class Lighting {
	public:
		Lighting(int pin, int off_angle, int idle_angle, int on_angle);
		~Lighting();
		
		void set(bool state);
		bool get();
		
		void refresh();
		
		bool is_busy();
	
	private:
		int pin;
		Servo servo;
		
		bool state;
		
		int off_angle;
		int idle_angle;
		int on_angle;
		unsigned long last_change;
		
		static const unsigned long TIME_PRESSED = 250;
		static const unsigned long TIME_OFF     = 250;
};


#endif
