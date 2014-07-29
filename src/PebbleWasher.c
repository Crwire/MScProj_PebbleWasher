#include <pebble.h>
#define DEFAULT_MESSAGE "Hello!"

static Window *window;
static TextLayer *text_status, *app_logo, *seconds_layer;
static AppTimer *dots_timer;
static int start_washing = 0;

//Timing resources
static time_t start_time;
static int total_time, total_washes = 0;

//These functions handle the buttons
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {	
	text_layer_set_text(text_status, "Select");//clear the text
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_status, "My name is Vipul. Hahah.");
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_status, "That's me butt! ..on...");
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}
//End button handling functions

//Animation handler whilst handwash taking place
static void time_animate(){
	static char dots[4];
	for(int i = 0;i<3;i++){
		if(dots[2] == 'o'){
			for(int j = 0;j<4;j++){
				dots[j] = ' ';
			}
			dots[0] = '\0';
			break;
		}
		if(!dots[i] || dots[i] == '\0'){
			dots[i] = 'o';
			dots[i+1] = '\0';
			break;
		}
	}
	//Display dots
	text_layer_set_text(seconds_layer, dots);
	
	if(start_washing == 1) //Recall itself
		dots_timer = app_timer_register(600, (AppTimerCallback) time_animate, NULL);
}

void end_washing_handler(){
	start_washing = 0;
	vibes_long_pulse();
	text_layer_set_text(text_status, "STATUS: Home");
	app_timer_cancel(dots_timer);
	text_layer_set_text(seconds_layer, DEFAULT_MESSAGE);
	
	//Take end time and process
	total_time = time(NULL) - start_time;
	static char seconds[] = "000s";
	snprintf(seconds, 5, "%03ds", total_time);
	text_layer_set_text(seconds_layer, seconds);
}

void start_washing_handler(){
	start_washing = 1;
	vibes_short_pulse();
	text_layer_set_text(text_status, "STATUS: Washing...");
	dots_timer = app_timer_register(600, (AppTimerCallback) time_animate, NULL);
	
	//Implement take start time
	time(&start_time);
}

//Gesture recognition handler
static void tap_handler(AccelAxisType axis, int32_t direction){
	if(axis==ACCEL_AXIS_X && direction == -1){
		if(start_washing == 1) //if currently washing, stop washing
			end_washing_handler();
	}
	else if(axis == ACCEL_AXIS_X && direction == +1){
		if(start_washing == 0) //if not washing, start washing
			start_washing_handler();
	}
}

//Rest of initialisation
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  //Set status bar
  text_status = text_layer_create((GRect) { .origin = { 5, 5 }, .size = { bounds.size.w, 40 } });
  text_layer_set_text(text_status, "STATUS: Home");
  //text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  //text_layer_set_overflow_mode(text_layer, GTextOverflowModeWordWrap);
  layer_add_child(window_layer, text_layer_get_layer(text_status));
  
  //Set logo
  app_logo = text_layer_create((GRect) { .origin = {5, 130 }, .size = {bounds.size.w, 40 } });
  text_layer_set_text(app_logo, "Sw by Crwire (C) 2014");
  layer_add_child(window_layer, text_layer_get_layer(app_logo));
  
  //Testing seconds timer callback 
  seconds_layer = text_layer_create((GRect) { .origin = {0, 45 }, .size = {bounds.size.w, 50 } });
  text_layer_set_text(seconds_layer, DEFAULT_MESSAGE);
  text_layer_set_text_alignment(seconds_layer, GTextAlignmentCenter);
  text_layer_set_font(seconds_layer, fonts_get_system_font("RESOURCE_ID_BITHAM_42_LIGHT"));
  layer_add_child(window_layer, text_layer_get_layer(seconds_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(text_status);
}

static void init(void) {
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
  accel_tap_service_subscribe(tap_handler); //Tap handler for gestures
}

static void deinit(void) {
  window_destroy(window);
  accel_tap_service_unsubscribe();
}

int main(void) {
  init();
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);
  app_event_loop();
  deinit();
}
