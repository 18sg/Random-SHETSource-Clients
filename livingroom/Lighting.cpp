#include <WProgram.h>
#include "Lighting.h"


Lighting::Lighting(int pin, int off_angle, int idle_angle, int on_angle)
	: pin(pin)
	, state(false)
	, off_angle(off_angle)
	, idle_angle(idle_angle)
	, on_angle(on_angle)
	, last_change(0)
{
	// Do nothing
}


Lighting::~Lighting(){}


void
Lighting::set(bool new_state)
{
	servo.attach(pin);
	servo.write(new_state ? on_angle : off_angle);
	state = new_state;
	last_change = millis();
}


void
Lighting::init()
{
	servo.attach(pin);
	servo.write(idle_angle);
	last_change = millis();
}


bool
Lighting::get()
{
	return state;
}


void
Lighting::refresh()
{
	unsigned long delta = millis() - last_change;
	bool time_pressed_expired = delta > TIME_PRESSED;
	bool time_off_expired = delta > TIME_PRESSED + TIME_OFF;
	
	if (time_pressed_expired && !time_off_expired && servo.read() != idle_angle) {
		servo.write(idle_angle);
	} else if (time_off_expired && servo.attached()) {
		servo.detach();
	}
}


bool
Lighting::is_busy()
{
	unsigned long delta = millis() - last_change;
	return delta < TIME_PRESSED + TIME_OFF;
}
