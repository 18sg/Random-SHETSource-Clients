#include <WProgram.h>
#include <Servo.h>
#include <RGBLED.h>
#include <Lighting.h>

#include "pins.h"
#include "comms.h"
#include "SHETSource.h"


/******************************************************************************
 * Pin Assignments                                                            *
 ******************************************************************************/

static const int PIN_SHETSOURCE_READ     = 14;
static const int PIN_SHETSOURCE_WRITE    = 15;

static const int PIN_SERVO_KITCHEN       = 9;
static const int PIN_SERVO_LOUNGE        = 10;

static const int PIN_LIGHTSWITCH         = 19;

static const int PIN_RGBLED_R            = 5;
static const int PIN_RGBLED_G            = 6;
static const int PIN_RGBLED_B            = 11;

static const int PIN_BTN[]               = {2,3,4,7,8,12,16,17};
static const int NUM_BTNS                = 8;

static const int APIN_WASHING            = 4;


/******************************************************************************
 * Constant Values                                                            *
 ******************************************************************************/

static const int SLOW_LOOP_PERIOD        = 1000;

static char *SHETSOURCE_PATH             = "livingroom";
static char *LIGHT_KITCHEN_NAME          = "light_kitchen";
static char *LIGHT_LOUNGE_NAME           = "light_lounge";

static char *RGBLED_SET_INSTANT_NAME     = "set_rgbled_instant";
static char *RGBLED_SET_FAST_NAME        = "set_rgbled";

static char *WASHING_FINISHED_NAME       = "washing_finished";
static char *WASHING_STATE_NAME          = "get_washing_state";

static const int RGBLED_FADE_FAST        = 250;

static const int WASHING_STATE_THRESHOLD = 512;

static const int SERVO_KITCHEN_ON        = 60;
static const int SERVO_KITCHEN_IDLE      = 90;
static const int SERVO_KITCHEN_OFF       = 120;

static const int SERVO_LOUNGE_ON         = 65;
static const int SERVO_LOUNGE_IDLE       = 90;
static const int SERVO_LOUNGE_OFF        = 115;

static const unsigned long LONG_PRESS    = 500;



/******************************************************************************
 * SHETSource Boiler-Plate                                                    *
 ******************************************************************************/

DirectPins pins = DirectPins(PIN_SHETSOURCE_READ, PIN_SHETSOURCE_WRITE);
Comms comms = Comms(&pins);
SHETSource::Client shetsource = SHETSource::Client(&comms, SHETSOURCE_PATH);

void
shetsource_init()
{
	pins.Init();
	shetsource.Init();
}




/******************************************************************************
 * Lights                                                                     *
 ******************************************************************************/

Lighting light_kitchen = Lighting(PIN_SERVO_KITCHEN,
                                  SERVO_KITCHEN_OFF,
                                  SERVO_KITCHEN_IDLE,
                                  SERVO_KITCHEN_ON);

Lighting light_lounge  = Lighting(PIN_SERVO_LOUNGE,
                                  SERVO_LOUNGE_OFF,
                                  SERVO_LOUNGE_IDLE,
                                  SERVO_LOUNGE_ON);

bool light_kitchen_requested, light_kitchen_state;
bool light_lounge_requested,  light_lounge_state;


// Setters
void set_light_kitchen(int s) { light_kitchen_requested = true; light_kitchen_state = s; }
void set_light_lounge(int s)  { light_lounge_requested = true;  light_lounge_state = s; }

// Getters
int get_light_kitchen() { return light_kitchen.get(); }
int get_light_lounge()  { return light_lounge.get(); }

void
lights_init()
{
	// Setup lightswitch
	pinMode(PIN_LIGHTSWITCH, INPUT);
	digitalWrite(PIN_LIGHTSWITCH, LOW);
	
	// Add SHET properties
	shetsource.AddProperty(LIGHT_KITCHEN_NAME, set_light_kitchen, get_light_kitchen);
	shetsource.AddProperty(LIGHT_LOUNGE_NAME,  set_light_lounge,  get_light_lounge);
}


void
lights_refresh()
{
	// TODO: Rewrite with button library
	// Handle button
	static bool btn_state = false;
	bool new_btn_state = !digitalRead(PIN_LIGHTSWITCH);
	if (new_btn_state != btn_state && new_btn_state == false) {
		// On button up
		light_kitchen_requested = light_lounge_requested = true;
		light_kitchen_state = light_lounge_state
			= !(light_lounge.get() && light_kitchen.get());
	}
	btn_state = new_btn_state;
	
	// Refresh servos
	light_kitchen.refresh();
	light_lounge.refresh();
	
	// Disalow simultaneous changes
	if (light_kitchen_requested && !light_lounge.is_busy()) {
		light_kitchen.set(light_kitchen_state);
		light_kitchen_requested = false;
	}
	if (light_lounge_requested && !light_kitchen.is_busy()) {
		light_lounge.set(light_lounge_state);
		light_lounge_requested = false;
	}
}




/******************************************************************************
 * RGB LED                                                                    *
 ******************************************************************************/

RGBLED rgbled = RGBLED(PIN_RGBLED_R, PIN_RGBLED_G, PIN_RGBLED_B);

// Colour Decoder
void
set_rgbled_colour(int encoded, int duration)
{
	// Expand 15-bit colour into 24-bit colour
	Colour colour;
	colour.r = ((encoded >>  0) & 0x1F) << 3;
	colour.g = ((encoded >>  5) & 0x1F) << 3;
	colour.b = ((encoded >> 10) & 0x1F) << 3;
	rgbled.set_colour(colour, duration);
}


// Setters
void set_rgbled_colour_instant(int encoded) { set_rgbled_colour(encoded, 0); }
void set_rgbled_colour_fast(int encoded) { set_rgbled_colour(encoded, RGBLED_FADE_FAST); }


void
rgbled_init()
{
	// Set up pins
	rgbled.attach();
	rgbled.refresh(true);
	
	// Add SHET actions
	shetsource.AddAction(RGBLED_SET_INSTANT_NAME, set_rgbled_colour_instant);
	shetsource.AddAction(RGBLED_SET_FAST_NAME, set_rgbled_colour_fast);
}


// Call an update on the LED (for fading etc).
void rgbled_refresh() { rgbled.refresh(true); }



/******************************************************************************
 * Washing Machine Sensor                                                     *
 ******************************************************************************/

// Event fired when washing finishes
SHETSource::LocalEvent *washing_finished;


// Getter for washing state
int
get_washing_state()
{
	return analogRead(APIN_WASHING) > WASHING_STATE_THRESHOLD;
}


void
washing_init()
{
	// Add SHET properties
	washing_finished = shetsource.AddEvent(WASHING_FINISHED_NAME);
	shetsource.AddAction(WASHING_STATE_NAME, get_washing_state);
}


void
washing_refresh()
{
	static unsigned long start_time = 0;
	static bool state = false;
	bool new_state = get_washing_state();
	
	//set_rgbled_colour_instant(new_state ? 0x1f << 5 : 0x1f);
	
	if (state != new_state && new_state == false) {
		// Washing finished
		(*washing_finished)((millis() - start_time) / 1000);
	} else if (state != new_state && new_state == true) {
		// Washing started
		start_time = millis();
	}
	
	state = new_state;
}



/******************************************************************************
 * Setup/Mainloop                                                             *
 ******************************************************************************/

void
setup()
{
	shetsource_init();
	lights_init();
	rgbled_init();
	washing_init();
}


// Mainloop executed every cycle
inline void
fast_loop()
{
	shetsource.DoSHET();
	rgbled_refresh();
	lights_refresh();
}


// Mainloop executed every few cycles
inline void
slow_loop()
{
	washing_refresh();
}


void
loop()
{
	// Execute a section of the main loop constantly
	fast_loop();
	
	// Execute a section of the main loop only occasionally
	static int counter = 0;
	counter++;
	if (counter % SLOW_LOOP_PERIOD == 0) {
		slow_loop();
	}
}
