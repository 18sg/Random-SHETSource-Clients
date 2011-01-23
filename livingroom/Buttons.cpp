#include <WProgram.h>
#include "Buttons.h"


ButtonManager::ButtonManager(const int long_press_duration,
                             const int     num_modes,
                             const int     num_btn_norm,
                             const uint8_t btn_norm_masks[],
                             const int     num_btn_mode,
                             const uint8_t btn_mode_masks[],
                             const int     num_btn_mod,
                             const uint8_t btn_mod_masks[])
	: long_press_duration(long_press_duration)
	, num_modes(num_modes)
	, num_btn_norm(num_btn_norm)
	, btn_norm_masks(btn_norm_masks)
	, num_btn_mode(num_btn_mode)
	, btn_mode_masks(btn_mode_masks)
	, num_btn_mod(num_btn_mod)
	, btn_mod_masks(btn_mod_masks)
	, mode(1)
	, cur_btn_states(0)
	, cum_btn_states(0)
	, add_btn_states(0)
	, sub_btn_states(0)
	, hold_started(false)
	, hold_start_time(0)
	, event_fired(false)
	, on_mode_change(NULL)
	, on_press(NULL)
	, on_hold_start(NULL)
	, on_hold_end(NULL)
{
	// Do nothing
}


void
ButtonManager::set_btn_states(uint8_t new_btn_states)
{
	// What buttons are newly pressed
	add_btn_states = (~cur_btn_states) & new_btn_states;
	
	// What buttons are newly depressed
	sub_btn_states = cur_btn_states & (~new_btn_states);
	
	// Update button states
	cur_btn_states = new_btn_states;
	cum_btn_states |= new_btn_states;
	
	// A new normal/modifier button was held down, reset the hold timer
	if (is_norm(add_btn_states) || is_mod(add_btn_states)) {
		reset_hold_timer();
	} else if (is_mode(cum_btn_states)) {
		stop_hold_timer(false);
	}
	
	if (!event_fired) {
		// An event hasn't been fired, can we fire one?
		if ((cur_btn_states == 0 && sub_btn_states != 0) || hold_timer_expired()) {
			if (is_mode(cum_btn_states))
				// Mode change requested
				fire_mode_change_event();
			else
				// Normal keypress
				fire_press_event();
			
			stop_hold_timer(true);
			
			// An event has been fired
			event_fired = true;
		}
	} else {
		// An event has been fired, can we start again yet?
		if (cur_btn_states == 0) {
			event_fired = false;
			cum_btn_states = 0;
			add_btn_states = 0;
		}
	}
}


void
ButtonManager::fire_mode_change_event()
{
	uint8_t buttons = get_norm(cum_btn_states);
	
	int old_mode_type = mode & ((1<<num_btn_mode)-1);
	int old_mode_num   = mode >> num_btn_mode;
	
	int new_mode_type = get_mode(cum_btn_states);
	int new_mode_num;
	
	if (new_mode_type != old_mode_type)
		new_mode_num = 0;
	else
		new_mode_num = (old_mode_num + 1) % num_modes;
	
	if (buttons != 0)
		// A mode has been picked
		new_mode_num = get_first(buttons);
	
	// Calculate new mode
	mode = (new_mode_num << num_btn_mode) | new_mode_type;
	
	// Raise callback 
	if (on_mode_change)
		on_mode_change(mode);
}


void
ButtonManager::fire_press_event()
{
	uint8_t buttons = get_norm(cum_btn_states);
	bool long_press = hold_timer_expired();
	bool modifiers  = get_mod(cum_btn_states);
	
	if (on_press)
		on_press(mode, modifiers, long_press, buttons);
}


void
ButtonManager::reset_hold_timer()
{
	if (on_hold_start)
		on_hold_start(!hold_started);
	hold_started = true;
	hold_start_time = millis();
}


void
ButtonManager::stop_hold_timer(bool finished)
{
	if (hold_started && on_hold_end)
		on_hold_end(finished && hold_timer_expired());
	hold_started = false;
}


bool
ButtonManager::hold_timer_expired()
{
	return hold_started && (millis() - hold_start_time > long_press_duration);
}


bool
ButtonManager::is_any_set(uint8_t state, const int num_masks, const uint8_t mask[])
{
	for (int i = 0; i < num_masks; i++) {
		if ((state & mask[i]) != 0)
			return true;
	}
	return false;
}


bool ButtonManager::is_norm(uint8_t s) {return is_any_set(s, num_btn_norm, btn_norm_masks);};
bool ButtonManager::is_mode(uint8_t s) {return is_any_set(s, num_btn_mode, btn_mode_masks);};
bool ButtonManager::is_mod(uint8_t s) {return is_any_set(s, num_btn_mod, btn_mod_masks);};


uint8_t
ButtonManager::get_bits(uint8_t state, const int num_masks, const uint8_t mask[])
{
	uint8_t out = 0;
	for (int i = 0; i < num_masks; i++) {
		out |= ((state & mask[i]) != 0) << i;
	}
	return out;
}


uint8_t ButtonManager::get_norm(uint8_t s) {return get_bits(s, num_btn_norm, btn_norm_masks);};
uint8_t ButtonManager::get_mode(uint8_t s) {return get_bits(s, num_btn_mode, btn_mode_masks);};
uint8_t ButtonManager::get_mod(uint8_t s)  {return get_bits(s, num_btn_mod, btn_mod_masks);};


int
ButtonManager::get_first(uint8_t bits)
{
	for (int i = 0; i < 8; i++)
		if ((bits>>i)&0x1)
			return i;
	
	return 0;
}
