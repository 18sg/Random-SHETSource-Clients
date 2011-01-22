#ifndef RGBLED_H
#define RGBLED_H

#include <WProgram.h>


struct Colour {
	int r;
	int g;
	int b;
};


class RGBLED {
	public:
		RGBLED(int pin_r, int pin_g, int pin_b);
		~RGBLED();
		
		// Setup pins
		void attach();
		
		// Update the colour
		void refresh(bool force);
		void refresh();
		
		// Fade the LED from the current colour to the specified one in the
		// specified duration
		void set_colour(const Colour &colour, int duration);
	
	private:
		int pin_r;
		int pin_g;
		int pin_b;
		
	public:
		Colour cur_col; // Current colour
		Colour old_col; // Old colour
		Colour new_col; // Target colour
		
		int step;
		int num_steps;
		
		unsigned long last_refresh;
		
		static const int UPDATE_PERIOD = 10;
};


#endif
