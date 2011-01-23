#include <WProgram.h>
#include <Servo.h>
#include <RGBLED.h>
#include <Lighting.h>
#include <Buttons.h>

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
 * SHET Node Names                                                            *
 ******************************************************************************/

static char *SHETSOURCE_PATH             = "livingroom";

static char *LIGHT_KITCHEN_NAME          = "light_kitchen";
static char *LIGHT_LOUNGE_NAME           = "light_lounge";

static char *RGBLED_SET_INSTANT_NAME     = "set_rgbled_instant";
static char *RGBLED_SET_FAST_NAME        = "set_rgbled";

static char *WASHING_FINISHED_NAME       = "washing_finished";
static char *WASHING_STATE_NAME          = "get_washing_state";

static char *BTNS_ON_PRESS_NAME          = "btn_pressed";
static char *BTNS_ON_MODE_CHANGE_NAME    = "btn_mode_changed";


/******************************************************************************
 * Constant Values                                                            *
 ******************************************************************************/

static const int SLOW_LOOP_PERIOD        = 1000;

static const int RGBLED_FADE_FAST        = 250;

static const int WASHING_STATE_THRESHOLD = 512;
static const unsigned long WASHING_MIN   = 5;

static const int SERVO_KITCHEN_ON        = 60;
static const int SERVO_KITCHEN_IDLE      = 90;
static const int SERVO_KITCHEN_OFF       = 120;

static const int SERVO_LOUNGE_ON         = 65;
static const int SERVO_LOUNGE_IDLE       = 90;
static const int SERVO_LOUNGE_OFF        = 115;

static const unsigned long LONG_PRESS    = 750;

static const uint8_t NUM_MODES           = 5;

static const uint8_t NUM_BTN_NORM        = 5;
static const uint8_t BTN_NORM_MASKS[]    = {0x01, 0x02, 0x04, 0x08, 0x10};

static const uint8_t NUM_BTN_MODE        = 2;
static const uint8_t BTN_MODE_MASKS[]    = {0x20, 0x80};

static const uint8_t NUM_BTN_MOD         = 1;
static const uint8_t BTN_MOD_MASKS[]     = {0x40};

static const int MODE_COLOURS[1<<NUM_BTN_MODE][NUM_BTN_NORM][3]
                                         = {{{  0,  0,  0},  // Ignored
                                             {  0,  0,  0},  // Ignored
                                             {  0,  0,  0},  // Ignored
                                             {  0,  0,  0},  // Ignored
                                             {  0,  0,  0}}, // Ignored
                                            {{255,  0,  0},  // Mode 1:0
                                             {  0,255,  0},  // Mode 1:1
                                             {  0,  0,255},  // Mode 1:2
                                             {255,  0,255},  // Mode 1:3
                                             {255,255,  0}}, // Mode 1:4
                                            {{ 63,  0,  0},  // Mode 2:0
                                             {  0, 63,  0},  // Mode 2:1
                                             {  0,  0, 63},  // Mode 2:2
                                             { 63,  0, 63},  // Mode 2:3
                                             { 63, 63,  0}}  // Mode 2:4
                                           };



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
	
	// Move servos to rest position
	light_kitchen.init();
	light_lounge.init();
}


void
lights_refresh()
{
	// TODO: Rewrite with button library
	// Handle button
	static bool btn_state = false;
	bool new_btn_state = !digitalRead(PIN_LIGHTSWITCH);
	if (new_btn_state != btn_state && new_btn_state == false) {
		// "Debounce"
		if (!(light_kitchen_requested || light_lounge_requested)) {
			// On button up
			light_kitchen_requested = light_lounge_requested = true;
			light_kitchen_state = light_lounge_state
				= !(light_lounge.get() && light_kitchen.get());
		}
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

void
set_rgbled_colour(int r, int g, int b, int duration)
{
	Colour colour;
	colour.r = r;
	colour.g = g;
	colour.b = b;
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
		int run_time = (millis() - start_time) / 1000;
		if (run_time >= WASHING_MIN)
			(*washing_finished)(run_time);
	} else if (state != new_state && new_state == true) {
		// Washing started
		start_time = millis();
	}
	
	state = new_state;
}



/******************************************************************************
 * Button Panel                                                               *
 ******************************************************************************/

ButtonManager btns = ButtonManager(LONG_PRESS,
                                   NUM_MODES,
                                   NUM_BTN_NORM,
                                   BTN_NORM_MASKS,
                                   NUM_BTN_MODE,
                                   BTN_MODE_MASKS,
                                   NUM_BTN_MOD,
                                   BTN_MOD_MASKS);

SHETSource::LocalEvent *evt_on_press;
SHETSource::LocalEvent *evt_on_mode_change;

Colour btn_old_colour;


void
btn_set_colour(int mode)
{
	set_rgbled_colour(MODE_COLOURS[mode&0x3][mode>>2][0],
	                  MODE_COLOURS[mode&0x3][mode>>2][1],
	                  MODE_COLOURS[mode&0x3][mode>>2][2],
	                  RGBLED_FADE_FAST);
}


void
btn_on_press(int mode, uint8_t modifiers, bool long_press, uint8_t buttons)
{
	int encoded = 0;
	encoded = mode;
	encoded = (encoded<<NUM_BTN_MOD) | modifiers;
	encoded = (encoded<<1) | long_press;
	encoded = (encoded<<NUM_BTN_NORM) | buttons;
	(*evt_on_press)(encoded);
}


void
btn_on_mode_change(int mode)
{
	btn_set_colour(mode);
	(*evt_on_mode_change)(mode);
}


void
btn_on_hold_start(bool starting)
{
	if (starting) {
		btn_old_colour.r = rgbled.new_col.r;
		btn_old_colour.g = rgbled.new_col.g;
		btn_old_colour.b = rgbled.new_col.b;
	}
	
	// Start fading the LED off
	set_rgbled_colour(0,0,0, LONG_PRESS);
}


void
btn_on_hold_end(bool finished)
{
	if (finished) {
		rgbled.cur_col.r = 0;
		rgbled.cur_col.g = 255;
		rgbled.cur_col.b = 0;
	}
	rgbled.set_colour(btn_old_colour, RGBLED_FADE_FAST);
}


void
btns_init()
{
	// Setup pins
	for (int i = 0; i < NUM_BTNS; i++) {
		pinMode(PIN_BTN[i], INPUT);
		digitalWrite(PIN_BTN[i], LOW);
	}
	
	// Setup SHET events
	evt_on_press = shetsource.AddEvent(BTNS_ON_PRESS_NAME);
	evt_on_mode_change = shetsource.AddEvent(BTNS_ON_MODE_CHANGE_NAME);
	
	
	// Bind callbacks
	btns.on_mode_change = btn_on_mode_change;
	btns.on_press = btn_on_press;
	btns.on_hold_start = btn_on_hold_start;
	btns.on_hold_end = btn_on_hold_end;
	
	btn_set_colour(btns.mode);
}


void
btns_refresh()
{
	// Read in pins
	uint8_t btn_states = 0;
	for (int i = 0; i < NUM_BTNS; i++)
		btn_states |= (!digitalRead(PIN_BTN[i])) << i;
	
	btns.set_btn_states(btn_states);
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
	btns_init();
}


// Mainloop executed every cycle
inline void
fast_loop()
{
	shetsource.DoSHET();
	rgbled_refresh();
	lights_refresh();
	btns_refresh();
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
