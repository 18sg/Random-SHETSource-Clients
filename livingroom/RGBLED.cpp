#include <WProgram.h>
#include <RGBLED.h>


RGBLED::RGBLED(int pin_r, int pin_g, int pin_b)
	: pin_r(pin_r)
	, pin_g(pin_g)
	, pin_b(pin_b)
	, step(0)
	, num_steps(1)
	, last_refresh(0)
{
	// Do nothing
}

RGBLED::~RGBLED()
{
	// Do nothing
}

void
RGBLED::attach()
{
	pinMode(pin_r, OUTPUT);
	pinMode(pin_g, OUTPUT);
	pinMode(pin_b, OUTPUT);
	last_refresh = millis();
}


void RGBLED::refresh() { refresh(false); }


void
RGBLED::refresh(bool force)
{
	// Calculate the step number
	unsigned long delta_steps = (millis() - last_refresh) / UPDATE_PERIOD;
	last_refresh += delta_steps * UPDATE_PERIOD;
	step = step + delta_steps;
	
	// No point continuing if the LED wont change
	//if (delta_steps == 0 && !force)
	//	return;
	
	if (step >= num_steps) {
		// Cycle has finished, make colour constant
		step = 0;
		old_col.r = new_col.r;
		old_col.g = new_col.g;
		old_col.b = new_col.b;
	}
	
	// Get deltas for new and old colours
	Colour delta;
	delta.r = new_col.r - old_col.r;
	delta.g = new_col.g - old_col.g;
	delta.b = new_col.b - old_col.b;
	
	// Calculate the current colour for the lights
	cur_col.r = old_col.r + ((delta.r * step) / num_steps);
	cur_col.g = old_col.g + ((delta.g * step) / num_steps);
	cur_col.b = old_col.b + ((delta.b * step) / num_steps);
	
	// Set the LED pins
	analogWrite(pin_r, 255 - cur_col.r);
	analogWrite(pin_g, 255 - cur_col.g);
	analogWrite(pin_b, 255 - cur_col.b);
}


void
RGBLED::set_colour(const Colour &colour, int duration)
{
	// The "old" colour is now the new one
	old_col.r = cur_col.r;
	old_col.g = cur_col.g;
	old_col.b = cur_col.b;
	
	// Set the new target colour
	new_col.r = colour.r;
	new_col.g = colour.g;
	new_col.b = colour.b;
	
	// Set up the duration of the transition
	step = 0;
	num_steps = (duration / UPDATE_PERIOD) + 1;
}
