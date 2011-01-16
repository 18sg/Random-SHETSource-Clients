#include "WProgram.h"
#include "pins.h"
#include "comms.h"
#include "SHETSource.h"


#define CLIENT_ADDR "livingroom"

const int LED_PORTS[] = {9, 10, 11};

enum {
	BTN_DOWN = 0,
	BTN_UP = 1,
};

const int NUM_BTNS    = 8;
const int BTN_PORTS[] = {2, 3, 4, 5, 6, 7, 8, 12};
int btn_state[NUM_BTNS] = {BTN_UP};
SHETSource::LocalEvent *btn_events[NUM_BTNS*2] = {NULL};
SHETSource::LocalEvent *btn_down;
SHETSource::LocalEvent *btn_up;

DirectPins pins = DirectPins(14,15);
Comms comms = Comms(&pins);
SHETSource::Client client = SHETSource::Client(&comms, CLIENT_ADDR);

#define  RGB_STEPS 100
#define  RGB_FREQUENCY 10

struct {
	int cur_r;
	int cur_g;
	int cur_b;
	
	int old_r;
	int old_g;
	int old_b;
	
	int new_r;
	int new_g;
	int new_b;
	
	int step;
} rgb;


void
rgb_update(void)
{
	if (((int)millis()) % RGB_FREQUENCY == 0) {
		rgb.step++;
		int delta_r = rgb.new_r - rgb.old_r;
		int delta_g = rgb.new_g - rgb.old_g;
		int delta_b = rgb.new_b - rgb.old_b;
		
		rgb.cur_r = rgb.old_r + ((delta_r * rgb.step) / RGB_STEPS);
		rgb.cur_g = rgb.old_g + ((delta_g * rgb.step) / RGB_STEPS);
		rgb.cur_b = rgb.old_b + ((delta_b * rgb.step) / RGB_STEPS);
		
		if (rgb.step >= RGB_STEPS) {
			rgb.cur_r = rgb.old_r = rgb.new_r;
			rgb.cur_g = rgb.old_g = rgb.new_g;
			rgb.cur_b = rgb.old_b = rgb.new_b;
			rgb.step = 0;
		}
		
		analogWrite(LED_PORTS[0], 255 - rgb.cur_r);
		analogWrite(LED_PORTS[1], 255 - rgb.cur_g);
		analogWrite(LED_PORTS[2], 255 - rgb.cur_b);
	}
}


void
set_rgb(int raw)
{
	rgb.old_r = rgb.cur_r;
	rgb.old_g = rgb.cur_g;
	rgb.old_b = rgb.cur_b;
	
	rgb.new_r = ((raw >>  0) & 0x1F) << 3;
	rgb.new_g = ((raw >>  5) & 0x1F) << 3;
	rgb.new_b = ((raw >> 10) & 0x1F) << 3;
	
	rgb.step = 0;
}


int
get_rgb(void)
{
	int out = 0;
	out = (out << 5) | (rgb.new_b >> 3);
	out = (out << 5) | (rgb.new_g >> 3);
	out = (out << 5) | (rgb.new_r >> 3);
}

void
monitor_buttons(void)
{
	for (int i = 0; i < NUM_BTNS; i++) {
		bool new_state = digitalRead(BTN_PORTS[i]);
		if (btn_state[i] != new_state) {
			btn_state[i] = new_state;
			if (new_state == BTN_DOWN)
				(*btn_down)(i);
			else
				(*btn_up)(i);
		}
	}
}


void
setup()
{
	pins.Init();
	client.Init();
	
	btn_down = client.AddEvent("btn_down");
	btn_up = client.AddEvent("btn_up");
	
	pinMode(LED_PORTS[0], OUTPUT);
	pinMode(LED_PORTS[1], OUTPUT);
	pinMode(LED_PORTS[2], OUTPUT);
	
	rgb.cur_r = 0; rgb.old_r = 0; rgb.new_r = 255;
	rgb.cur_g = 0; rgb.old_g = 0; rgb.new_g = 255;
	rgb.cur_b = 0; rgb.old_b = 0; rgb.new_b = 255;
	rgb.step = 0;
	
	client.AddProperty("rgb", set_rgb, get_rgb);
}


void
loop()
{
	client.DoSHET();
	rgb_update();
	monitor_buttons();
}
