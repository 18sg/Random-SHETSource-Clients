#ifndef BUTTONS_H
#define BUTTONS_H

#include <WProgram.h>

static const unsigned long LONG_PRESS_DURATION = 750;


class ButtonManager {
	public:
		ButtonManager();
		
		void set_button_state(int button, bool state);
		bool get_button_pressed(int button);
		
		void reset_press_duration();
		unsigned long get_press_duration();
		
		void on_up();
		void on_any_down(int button);
		
		uint8_t get_buttons();
		bool get_modifier();
		uint8_t get_mode_btns();
		
		void set_new_mode();
		
		bool get_mode();
	
	public:
		uint8_t button_states;
		uint8_t buttons_pressed;
		unsigned long start_time;
		
		int mode;
		
		bool event_fired;
	
	public:
		void (*on_press)(int mode, bool long_press, bool modifier, uint8_t buttons);
		void (*on_mode_change)(int mode);
		
		void (*on_button_down)();
};


#endif
