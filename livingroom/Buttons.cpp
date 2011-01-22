#include <WProgram.h>
#include "Buttons.h"

static const int     NUM_BUTTONS   = 5;
static const uint8_t BUTTON_MASK   = 0x1F;
static const uint8_t MODIFIER_MASK = 0x40;
static const int     NUM_MODE_BTNS = 2;
static const uint8_t MODE_MASKS[]  = {0x20, 0x80};


ButtonManager::ButtonManager()
	: button_states(0)
	, buttons_pressed(0)
	, start_time(0)
	, mode(0)
	, on_press(NULL)
	, on_mode_change(NULL)
	, event_fired(false)
{
	// Do nothing
}


void
ButtonManager::set_button_state(int button, bool state)
{
	// First button to be pressed?
	if ((buttons_pressed & BUTTON_MASK) == 0 && state)
		reset_press_duration();
	
	// Has the button been pressed for the first time
	if (!get_button_pressed(button) && state)
		on_any_down(button);
	
	// Set button state
	bitWrite(button_states, button, state);
	
	// Record what buttons have been pressed in total
	buttons_pressed |= button_states;
	
	// Has the last button been released?
	if (!event_fired) {
		if (button_states == 0 && buttons_pressed != 0 && !state) {
			on_up();
		}
		
		if (buttons_pressed != 0 && get_press_duration() > LONG_PRESS_DURATION) {
			on_up();
			event_fired = true;
		}
	}
	
	if (button_states == 0) {
		// Reset
		button_states = 0;
		buttons_pressed = 0;
		start_time = 0;
		event_fired = false;
	}
}


bool
ButtonManager::get_button_pressed(int button)
{
	return (bool) ((buttons_pressed >> button) & 0x01);
}


unsigned long
ButtonManager::get_press_duration()
{
	return millis() - start_time;
}


void
ButtonManager::reset_press_duration()
{
	start_time = millis();
}


void
ButtonManager::on_up()
{
	if (get_mode_btns() != 0) {
		// Mode change
		set_new_mode();
		
		// Callback
		if (on_mode_change)
			on_mode_change(mode);
	} else {
		bool long_press = get_press_duration() > LONG_PRESS_DURATION;
		uint8_t buttons = get_buttons();
		bool modifier = get_modifier();
		
		// Callback
		if (on_press)
			on_press(mode, long_press, modifier, buttons);
	}
}


void
ButtonManager::on_any_down(int button)
{
	uint8_t buttons = get_buttons();
	if (buttons == 0 && button < NUM_MODE_BTNS && on_button_down) {
		on_button_down();
	}
}


uint8_t
ButtonManager::get_buttons()
{
	return buttons_pressed & BUTTON_MASK;
}


bool
ButtonManager::get_modifier()
{
	return (buttons_pressed & MODIFIER_MASK) != 0;
}

uint8_t
ButtonManager::get_mode_btns()
{
	uint8_t out = 0x00;
	for (int i = 0; i < NUM_MODE_BTNS; i++)
		out |= (((buttons_pressed&MODE_MASKS[i]) != 0) << i);
	return out;
}


// Set the new mode based on the current key combination
void
ButtonManager::set_new_mode()
{
	uint8_t buttons = get_buttons();
	
	// Extract the mode-type (last bit)
	uint8_t new_mode_type = ((mode & 0x8) != 0);
	
	// Extract the mode number (first 3 bits)
	uint8_t new_mode_num = mode & 0x7;
	
	if (buttons != 0) {
		// Specific mode specified by holding down a key
		for (int i = 0; i < NUM_BUTTONS; i++) {
			if ((buttons & (1<<i)) != 0)
				new_mode_num = i;
		}
	} else {
		// Cycle mode num if mode type not changed
		if (new_mode_type == (get_mode_btns() == 2))
			new_mode_num = (new_mode_num + 1) % NUM_BUTTONS;
		else
			new_mode_num = 0;
	}
	
	new_mode_type = (get_mode_btns() == 2);
	
	// Set the mode
	mode = (new_mode_type << 3) | new_mode_num;
}
