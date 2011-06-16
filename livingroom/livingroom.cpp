#include <WProgram.h>
#include <Servo.h>
#include <RGBLED.h>
#include <Lighting.h>
#include <Buttons.h>

#include "pins.h"
#include "comms.h"
#include "SHETSource.h"
#include "Wire.h"
#include "i2c_expander.h"


/******************************************************************************
 * Pin Assignments                                                            *
 ******************************************************************************/

static const int PIN_SHETSOURCE_READ     = 7;
static const int PIN_SHETSOURCE_WRITE    = 8;

static const int PIN_SERVO_KITCHEN       = 9;
static const int PIN_SERVO_LOUNGE        = 10;

static const int PIN_LIGHTSWITCH         = RB3;

static const int PIN_BACKDOOR            = RB4;

static const int PIN_RGBLED_R            = 3;
static const int PIN_RGBLED_G            = 5;
static const int PIN_RGBLED_B            = 6;

static const int PIN_IO_INTERRUPT        = 12;

static const int PIN_BTN[]               = {RC6,RC7,RB0,RB1,RB2,
                                                RC0,RC1,RC2    };
static const int NUM_BTNS                = 8;

static const int APIN_WASHING            = 0;
static const int APIN_OVEN               = 1;
static const int APIN_PIR                = 2;

static const int PIN_AMP_A               = RB5;
static const int PIN_AMP_B               = RB6;


/******************************************************************************
 * SHET Node Names                                                            *
 ******************************************************************************/

static char *SHETSOURCE_PATH             = "lounge/arduino";

static char *LIGHT_KITCHEN_NAME          = "light_kitchen";
static char *LIGHT_LOUNGE_NAME           = "light_lounge";
static char *LIGHT_TOGGLE_NAME           = "light_toggle";

static char *RGBLED_SET_INSTANT_NAME     = "set_rgbled_instant";
static char *RGBLED_SET_FAST_NAME        = "set_rgbled";

static char *WASHING_FINISHED_NAME       = "washing_finished";
static char *WASHING_STARTED_NAME        = "washing_started";
static char *WASHING_STATE_NAME          = "get_washing_state";

static char *BTNS_ON_PRESS_NAME          = "btn_pressed";
static char *BTNS_ON_MODE_CHANGE_NAME    = "btn_mode_changed";
static char *BTNS_MODE                   = "btn_mode";

static char *LIGHTSWITCH_PRESSED_NAME    = "btn_lightswitch";

static char *PIR_NAME                    = "pir";

static char *OVEN_ON_NAME                = "on_oven_on";
static char *OVEN_OFF_NAME               = "on_oven_off";
static char *OVEN_STATE_NAME             = "oven_heating";

static char *BACKDOOR_STATE_NAME         = "backdoor_open";
static char *BACKDOOR_OPENED_NAME        = "backdoor_opened";
static char *BACKDOOR_CLOSED_NAME        = "backdoor_closed";

static char *AMP_INC_NAME                = "amp_inc";
static char *AMP_DEC_NAME                = "amp_dec";

/******************************************************************************
 * Constant Values                                                            *
 ******************************************************************************/

static const int IO_EXPANDER_ADDRESS     = 2;

static const int BTN_LOOP_PERIOD         = 100;
static const int SLOW_LOOP_PERIOD        = 1000;

static const int RGBLED_FADE_FAST        = 250;

static const int OVEN_STATE_THRESHOLD    = 512;

static const int PIR_THRESHOLD           = 712;

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

static const int LIGHTSWITCH_THRESHOLD   = 200;



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
 * I/O Expander Board Boiler-Plate                                            *
 ******************************************************************************/

io_expander expander;

void
io_init(void)
{
	Wire.begin();
	pinMode(PIN_IO_INTERRUPT, INPUT);
	digitalWrite(PIN_IO_INTERRUPT, LOW);
	init_io_expander(&expander, IO_EXPANDER_ADDRESS);
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

SHETSource::LocalEvent *lightswitch_pressed;


// Setters
void set_light_kitchen(int s) { light_kitchen_requested = true; light_kitchen_state = s; }
void set_light_lounge(int s)  { light_lounge_requested = true;  light_lounge_state = s; }

// Getters
int get_light_kitchen() { return light_kitchen.get(); }
int get_light_lounge()  { return light_lounge.get(); }

void
lights_toggle()
{
	light_kitchen_requested = light_lounge_requested = true;
	light_kitchen_state = light_lounge_state
		= !(light_lounge.get() && light_kitchen.get());
}


void
lights_init()
{
	// Setup lightswitch
	//pinMode(APIN_LIGHTSWITCH, INPUT);
	//digitalWrite(APIN_LIGHTSWITCH, LOW);
	
	// Add SHET properties
	shetsource.AddProperty(LIGHT_KITCHEN_NAME, set_light_kitchen, get_light_kitchen);
	shetsource.AddProperty(LIGHT_LOUNGE_NAME,  set_light_lounge,  get_light_lounge);
	
	shetsource.AddAction(LIGHT_TOGGLE_NAME, lights_toggle);
	
	// Move servos to rest position
	light_kitchen.init();
	light_lounge.init();
}


void
lights_refresh()
{
	//int pin = analogRead(APIN_LIGHTSWITCH);
	//bool new_btn_state = pin < LIGHTSWITCH_THRESHOLD;
	//bool new_pir_state = (pin > LIGHTSWITCH_THRESHOLD) && (pin < PIR_THRESHOLD);
	
	// TODO: Rewrite with button library
	// Handle button
	//static bool btn_state = false;
	//if (new_btn_state != btn_state && new_btn_state == true)
	//	(*lightswitch_pressed)();
	//btn_state = new_btn_state;
	
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
 * PIR                                                                        *
 ******************************************************************************/

// Lounge PIR event
SHETSource::LocalEvent *pir;

void
pir_init(void)
{
	pir = shetsource.AddEvent(PIR_NAME);
}


void
pir_refresh(void)
{
	static bool pir_state = false;
	bool new_pir_state = analogRead(APIN_PIR) < PIR_THRESHOLD;
	
	// Handle PIR
	if (new_pir_state != pir_state && new_pir_state == true)
		(*pir)();
	pir_state = new_pir_state;
}



/******************************************************************************
 * rgb led                                                                    *
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
SHETSource::LocalEvent *washing_started;
SHETSource::LocalEvent *washing_finished;

static unsigned long washing_start_time = 0;


// Getter for washing state
int
get_washing_state()
{
	return (analogRead(APIN_WASHING) < WASHING_STATE_THRESHOLD)
	       ? (int)((millis() - washing_start_time) / 1000l)
	       : 0;
}


void
washing_init()
{
	// Add SHET properties
	washing_finished = shetsource.AddEvent(WASHING_FINISHED_NAME);
	washing_started = shetsource.AddEvent(WASHING_STARTED_NAME);
	shetsource.AddAction(WASHING_STATE_NAME, get_washing_state);
}


void
washing_refresh()
{
	static bool state = false;
	bool new_state = analogRead(APIN_WASHING) < WASHING_STATE_THRESHOLD;
	
	if (state != new_state && new_state == false) {
		// Washing finished
		int run_time = (millis() - washing_start_time) / 1000;
		if (run_time >= WASHING_MIN)
			(*washing_finished)(run_time);
	} else if (state != new_state && new_state == true) {
		// Washing started
		if (millis() - washing_start_time > 5000)
			(*washing_started)();
		washing_start_time = millis();
	}
	
	state = new_state;
}



/******************************************************************************
 * Oven Sensor                                                                *
 ******************************************************************************/

SHETSource::LocalEvent *oven_on;
SHETSource::LocalEvent *oven_off;

int oven_state = false;


void
oven_init(void)
{
	oven_on  = shetsource.AddEvent(OVEN_ON_NAME);
	oven_off = shetsource.AddEvent(OVEN_OFF_NAME);
	shetsource.AddProperty(OVEN_STATE_NAME, &oven_state);
}


void
oven_refresh()
{
	int new_state = analogRead(APIN_OVEN) > OVEN_STATE_THRESHOLD;
	
	if (oven_state != new_state) {
		if (new_state)
			(*oven_on)();
		else
			(*oven_off)();
	}
	
	oven_state = new_state;
}



/******************************************************************************
 * Lightswitch                                                                *
 ******************************************************************************/

SHETSource::LocalEvent *lightswitch;

void
lightswitch_init(void)
{
	lightswitch = shetsource.AddEvent(LIGHTSWITCH_PRESSED_NAME);
	pinMode(&expander, PIN_LIGHTSWITCH, INPUT);
	digitalWrite(&expander, PIN_LIGHTSWITCH, HIGH);
}


void
lightswitch_refresh()
{
	static int state = false;
	int new_state = digitalRead(&expander, PIN_LIGHTSWITCH);
	
	if (state != new_state && new_state == true)
		(*lightswitch)();
	
	state = new_state;
}



/******************************************************************************
 * Backdoor Sensor                                                            *
 ******************************************************************************/

SHETSource::LocalEvent *backdoor_opened;
SHETSource::LocalEvent *backdoor_closed;
int backdoor_state = false;

void
backdoor_init(void)
{
	shetsource.AddProperty(BACKDOOR_STATE_NAME, &backdoor_state);
	backdoor_opened = shetsource.AddEvent(BACKDOOR_OPENED_NAME);
	backdoor_closed = shetsource.AddEvent(BACKDOOR_CLOSED_NAME);
	
	pinMode(&expander, PIN_BACKDOOR, INPUT);
	digitalWrite(&expander, PIN_BACKDOOR, HIGH);
}


void
backdoor_refresh()
{
	int new_state = digitalRead(&expander, PIN_BACKDOOR);
	
	if (backdoor_state != new_state)
		if (new_state)
			(*backdoor_opened)();
		else
			(*backdoor_closed)();
	
	backdoor_state = new_state;
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
btn_set_mode(int mode)
{
	btns.mode = mode;
	btn_on_mode_change(mode);
}


int
btn_get_mode()
{
	return btns.mode;
}


void
btns_init()
{
	// Setup pins
	for (int i = 0; i < NUM_BTNS; i++) {
		pinMode(&expander, PIN_BTN[i], INPUT);
		attachInterrupt(&expander, PIN_BTN[i]);
	}
	
	// Setup SHET events
	evt_on_press = shetsource.AddEvent(BTNS_ON_PRESS_NAME);
	evt_on_mode_change = shetsource.AddEvent(BTNS_ON_MODE_CHANGE_NAME);
	shetsource.AddProperty(BTNS_MODE, btn_set_mode, btn_get_mode);
	
	
	// Bind callbacks
	btns.on_mode_change = btn_on_mode_change;
	btns.on_press       = btn_on_press;
	btns.on_hold_start  = btn_on_hold_start;
	btns.on_hold_end    = btn_on_hold_end;
	
	btn_set_colour(btns.mode);
}


void
btns_refresh()
{
	// Read in pins
	uint8_t btn_states = 0;
	for (int i = 0; i < NUM_BTNS; i++) {
		btn_states |= (!digitalRead(&expander, PIN_BTN[i])) << i;
	}
	
	btns.set_btn_states(btn_states);
}



/******************************************************************************
 * Amplifier stuff                                                            *
 ******************************************************************************/

static const int AMP_WAIT_TIME = 10;

void
amp_wave(uint8_t pin_a, uint8_t pin_b)
{
	digitalWrite(&expander, pin_a, HIGH);
	delay(AMP_WAIT_TIME);
	digitalWrite(&expander, pin_b, HIGH);
	delay(AMP_WAIT_TIME);
	digitalWrite(&expander, pin_a, LOW);
	delay(AMP_WAIT_TIME);
	digitalWrite(&expander, pin_b, LOW);
	delay(AMP_WAIT_TIME);
}


void
amp_inc_vol(int repeat)
{
	for (int i = 0; i < repeat; i++)
		amp_wave(PIN_AMP_A, PIN_AMP_B);
}


void
amp_dec_vol(int repeat)
{
	for (int i = 0; i < repeat; i++)
		amp_wave(PIN_AMP_B, PIN_AMP_A);
}


void
amp_init()
{
	digitalWrite(&expander, PIN_AMP_A, LOW);
	pinMode(&expander, PIN_AMP_A, OUTPUT);
	
	digitalWrite(&expander, PIN_AMP_B, LOW);
	pinMode(&expander, PIN_AMP_B, OUTPUT);
	
	shetsource.AddAction(AMP_DEC_NAME, amp_dec_vol);
	shetsource.AddAction(AMP_INC_NAME, amp_inc_vol);
}



/******************************************************************************
 * Setup/Mainloop                                                             *
 ******************************************************************************/

void
setup()
{
	shetsource_init();
	io_init();
	lights_init();
	rgbled_init();
	washing_init();
	oven_init();
	pir_init();
	btns_init();
	lightswitch_init();
	backdoor_init();
	amp_init();
}


// Mainloop executed every cycle
inline void
fast_loop()
{
	shetsource.DoSHET();
	rgbled_refresh();
}


// Mainloop executed less frequently for button scanning
inline void
btn_loop()
{
	lights_refresh();
	btns_refresh();
	lightswitch_refresh();
}


// Mainloop executed every few cycles
inline void
slow_loop()
{
	washing_refresh();
	oven_refresh();
	pir_refresh();
	backdoor_refresh();
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
	
	// Execute a section of the main loop for button reading (pseudo debounce)
	if (counter % BTN_LOOP_PERIOD == 0) {
		btn_loop();
	}
}
