#ifndef BUTTONS_H
#define BUTTONS_H

#include <WProgram.h>


class ButtonManager {
	public:
		ButtonManager(const int long_press_duration,
                  const int     num_modes,
                  const int     num_btn_norm,
                  const uint8_t btn_norm_masks[],
                  const int     num_btn_mode,
                  const uint8_t btn_mode_masks[],
                  const int     num_btn_mod,
                  const uint8_t btn_mod_masks[]);
		
		// Register the current state of a button
		void set_btn_states(uint8_t new_btn_states);
	
	private:
		// Length of a long-press in milliseconds
		const int long_press_duration;
		
		// Number of modes per mode button
		int num_modes;
		
		// Normal buttons
		const int     num_btn_norm;
		const uint8_t *btn_norm_masks;
		
		// Mode buttons
		const int     num_btn_mode;
		const uint8_t *btn_mode_masks;
		
		// Modifiers
		const int     num_btn_mod;
		const uint8_t *btn_mod_masks;
	
	
	public:
		int mode;
	
	private:
		uint8_t cur_btn_states; // Currently pressed buttons
		uint8_t cum_btn_states; // (Cumaltive) pressed buttons
		uint8_t add_btn_states; // Newly pressed (added) buttons
		uint8_t sub_btn_states; // Newly depressed (subtracted) buttons
		
		bool hold_started;
		unsigned long hold_start_time;
		
		bool event_fired; // Has an event been fired for this key press
		
		void fire_mode_change_event();
		void fire_press_event();
		
		void stop_hold_timer(bool finished);
		void reset_hold_timer();
		bool hold_timer_expired();
		
		bool is_any_set(uint8_t state, const int num_masks, const uint8_t mask[]);
		
		bool is_norm(uint8_t state);
		bool is_mode(uint8_t state);
		bool is_mod(uint8_t state);
		
		uint8_t get_bits(uint8_t state, const int num_masks, const uint8_t mask[]);
		
		uint8_t get_norm(uint8_t state);
		uint8_t get_mode(uint8_t state);
		uint8_t get_mod(uint8_t state);
		
		int get_first(uint8_t bits);
	
	public:
		void (*on_mode_change)(int mode);
		void (*on_press)(int mode, uint8_t modifiers, bool long_press, uint8_t buttons);
		
		void (*on_hold_start)(bool starting);
		void (*on_hold_end)(bool finished);
};


#endif
