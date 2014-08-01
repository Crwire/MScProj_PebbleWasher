#include <pebble.h>
//#include "Survey.h"
#define DEFAULT_MESSAGE "Hello!"
#define USER "Chuck,"
#define MTRUE 1
#define MFALSE 0
//Use pre-defined keys for persistent storage
#define NUM_WASHES_KEY 1
#define NUM_WASHES_DEFAULT 0
#define CURR_DAY_KEY 2
#define CURR_DAY_DEFAULT 0
#define DAY_DATA_KEY 50
#define DAY_END_KEY 3
#define DAY_STARTED_KEY 4
#define DAY_DATA_TEMP 5
//Use pre-defined menu item keys
#define NAV_HOME 0
#define NAV_WASHES 1
#define NAV_NEW_DAY 2
#define NAV_END_DAY 3
#define NAV_DELETE 4
#define NAV_HELP 5
#define NAV_ITEMS_MAX 4
#define NAV_ITEMS_MIN 1

static Window *window;
static TextLayer *text_status, *app_logo, *seconds_layer, *curr_day_layer;
static AppTimer *dots_timer, *countdown_timer;
static int start_washing = MFALSE;

//Menu resources; home by default
static int menu_selector = NAV_HOME;

//Handwash timing resources
static time_t start_time;
static int total_time;
static int total_washes = NUM_WASHES_DEFAULT;

//Day resources
static int start_day = MFALSE;
static int current_day = CURR_DAY_DEFAULT;
static time_t end_of_day;

//Store in curr_day_data like so: DDWWXXSSSXXSSSXXSSSXXSSS D=day, W=total washes, X=wash, S=time
char reading_day_data[256], curr_day_data[256];

static void day_handler(int i);//prototype

static void check_if_day_end(){
	if(time(NULL) >= end_of_day){
		day_handler(NAV_END_DAY);
		return;
	}
	text_layer_set_text(app_logo, "Recording... Crwire(C)");
	countdown_timer = app_timer_register(30000, (AppTimerCallback) check_if_day_end, NULL);
}

//Handles new day or end day calls
static void day_handler(int menuItem){
	if(menuItem == NAV_NEW_DAY){
		if(start_day == MTRUE){
			text_layer_set_text(text_status, "Already started..");
			return;
		}
		start_day = MTRUE;
		text_layer_set_text(text_status, "Day begun!");
		total_washes = NUM_WASHES_DEFAULT;
		vibes_double_pulse();
		current_day++; //Increment current day and display
	    static char my_days[] = "Day: 00";
	    snprintf(my_days, 8, "Day: %02d", current_day);
	    text_layer_set_text(curr_day_layer, my_days);
		text_layer_set_text(app_logo, "Recording... Crwire(C)");
		//Implement data recording mechanism here
		end_of_day = time(NULL) + (time_t) 28800;
		countdown_timer = app_timer_register(30000, (AppTimerCallback) check_if_day_end, NULL);
	}
	else if(menuItem == NAV_END_DAY){
		if(start_day == MFALSE){
			text_layer_set_text(text_status, "Day not begun..");
			return;
		}
		start_day = MFALSE;
		vibes_long_pulse();
		text_layer_set_text(text_status, "Day ended!");
		text_layer_set_text(app_logo, "Sw by Crwire (C) 2014");
		//Implement persist data into final storage
		if(persist_exists(DAY_DATA_KEY + current_day)){
			text_layer_set_text(text_status, "Abort:Today exists..");
			return;
		}
		//Write current day to memory
		curr_day_data[255] = '\0'; //NULL terminate it. 256 is okay...
	    static char my_days[] = "00"; snprintf(my_days, 3, "%02d", current_day);
		for(int i = 0;i<=1;i++){curr_day_data[i] = my_days[i];} //Append days to beginning
		persist_write_data(DAY_DATA_KEY + current_day, &curr_day_data, sizeof(curr_day_data));
	}
}

/* ==================================
Buttons implementation and handlers */
static void update_menu_item(){
	static char my_washes[] = "Washes: 00";
	snprintf(my_washes, 11, "Washes: %02d", total_washes);
	
	switch(menu_selector){
		case NAV_HELP: text_layer_set_text(text_status, "Help"); break;
		case NAV_WASHES: text_layer_set_text(text_status, my_washes); break;
		case NAV_DELETE: text_layer_set_text(text_status, "Delete prev. wash"); break;
		case NAV_NEW_DAY: text_layer_set_text(text_status, "New Day"); break;
		case NAV_END_DAY: text_layer_set_text(text_status, "End Day"); break;
		case NAV_HOME: text_layer_set_text(text_status, USER); break;
		default: text_layer_set_text(text_status, "Error");
	}
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {	
	//Implement a select based on option highlighted
	static char seconds[] = "000s"; //TODO: check records for last wash time
	snprintf(seconds, 5, "%03ds", total_time);
	
	switch(menu_selector){
		case NAV_WASHES: 
			text_layer_set_text(seconds_layer, seconds);
			break;
		case NAV_HELP: text_layer_set_text(text_status, "Hold for Help"); break;
		case NAV_DELETE: if(total_washes>=1){
			text_layer_set_text(text_status, "Deleted!"); 
			for(int i = 1;i<=3;i++){curr_day_data[(5*total_washes)+i] = (int) NULL;} 
			total_washes--; 
			break;
		}else{text_layer_set_text(text_status, "None to delete..."); break;}
		case NAV_NEW_DAY: day_handler(NAV_NEW_DAY); break;
		case NAV_END_DAY: day_handler(NAV_END_DAY); break;
		case NAV_HOME: text_layer_set_text(seconds_layer, DEFAULT_MESSAGE); break;
		default: text_layer_set_text(seconds_layer, DEFAULT_MESSAGE);
	}
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(menu_selector >=NAV_ITEMS_MIN)
	  menu_selector--;
  update_menu_item();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  //Implement navigate down
  if(menu_selector <=NAV_ITEMS_MAX)
	  menu_selector++;
  update_menu_item();
}

static void reset_all_text_handler(){
	text_layer_set_text(text_status, USER);
	text_layer_set_text(seconds_layer, DEFAULT_MESSAGE);
    static char my_days[] = "Day: 00";
    snprintf(my_days, 8, "Day: %02d", current_day);
	text_layer_set_text(curr_day_layer, my_days);
	text_layer_set_text(app_logo, "Sw by Crwire (C) 2014");
}

static void down_long_click_handler(ClickRecognizerRef recognizer, void *context){
	//DELETES ALL DATA. BE CAREFUL WITH THIS.
	static int i = DAY_DATA_KEY;//first wipe the records
	i = i + current_day;
	for(;i>=DAY_DATA_KEY;i--){
		if(persist_exists(i))
			persist_delete(i);
	} //wipe the persists
	total_washes = NUM_WASHES_DEFAULT;
	if(persist_exists(NUM_WASHES_KEY))
		persist_delete(NUM_WASHES_KEY);
	current_day = CURR_DAY_DEFAULT;
	if(persist_exists(CURR_DAY_KEY))
		persist_delete(CURR_DAY_KEY);
	//Reset booleans and navigation
	start_washing = MFALSE;
	start_day = MFALSE;
	menu_selector = NAV_HOME;
	//Notify of deletion and display normal text on finish
	text_layer_set_text(text_status, "All data is...");
	text_layer_set_text(seconds_layer, "GONE");
	dots_timer = app_timer_register(7500, (AppTimerCallback) reset_all_text_handler, NULL);
}

static void select_long_click_handler(ClickRecognizerRef recognizer, void *context){
	//Change seconds layer to help text
	if(menu_selector == NAV_HELP){
		text_layer_set_font(seconds_layer, fonts_get_system_font("RESOURCE_ID_GOTHIC_12"));
		text_layer_set_overflow_mode(seconds_layer, GTextOverflowModeTrailingEllipsis);
		text_layer_set_text(seconds_layer, "Punch to record a wash. Swing up to end wash. Record 8hrs/day.\n Accident? Delete prev. wash!");
	}
}

static void select_long_click_release_handler(ClickRecognizerRef recognizer, void *context){
	//Implement default message here
	if(menu_selector == NAV_HELP){
		text_layer_set_font(seconds_layer, fonts_get_system_font("RESOURCE_ID_BITHAM_42_LIGHT"));
		text_layer_set_text(seconds_layer, DEFAULT_MESSAGE);
	}
}

static void up_long_click_handler(ClickRecognizerRef recognizer, void *context){
	//Display records
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Begin data logging");
	text_layer_set_text(seconds_layer, "LOG'N");
	char reading_stuff[256];
	
	for(int i = DAY_DATA_KEY + current_day;i>=DAY_DATA_KEY;i--){
			if(persist_exists(i)){
				persist_read_string(i, reading_stuff, sizeof(reading_stuff));
				APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", reading_stuff);
			}	
	}
	APP_LOG(APP_LOG_LEVEL_DEBUG, "End data logging");
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  //Long press providers
  window_long_click_subscribe(BUTTON_ID_DOWN, 5000, down_long_click_handler, NULL);
  window_long_click_subscribe(BUTTON_ID_UP, 5000, up_long_click_handler, NULL);
  window_long_click_subscribe(BUTTON_ID_SELECT, 2000, select_long_click_handler, select_long_click_release_handler);
}

/* ==============================
Gesture and Animation handlers */
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
	text_layer_set_text(text_status, "Home");
	app_timer_cancel(dots_timer);
	text_layer_set_text(seconds_layer, DEFAULT_MESSAGE);
	
	//Take end time and process
	total_time = time(NULL) - start_time;
	static char seconds[] = "000s";
	snprintf(seconds, 5, "%03ds", total_time);
	text_layer_set_text(seconds_layer, seconds);
	
	//Update and display total washes
	total_washes++;
	static char my_washes[] = "Washes: 00";
	snprintf(my_washes, 11, "Washes: %02d", total_washes);
	text_layer_set_text(text_status, my_washes);
	menu_selector = NAV_WASHES;
	
	//Save washing instance to curr_day_data if start_day == MTRUE
	if(start_day == MTRUE){
		//Write to curr_day_data
		static char num_washes[] = "00";
		snprintf(num_washes, 3, "%02d", total_washes);
		//Write total washes in [2],[3]
		curr_day_data[2] = num_washes[0]; curr_day_data[3] = num_washes[1];
		//Write current wash before seconds
		curr_day_data[5*total_washes] = num_washes[1]; curr_day_data[(5*total_washes)-1] = num_washes[0];		
		//Write seconds into the structure
		for(int i = 1;i<=3;i++){
			curr_day_data[(5*total_washes)+i] = seconds[i-1];
		}
	}
}

void start_washing_handler(){
	start_washing = 1;
	vibes_short_pulse();
	text_layer_set_text(text_status, "Washing...");
	dots_timer = app_timer_register(600, (AppTimerCallback) time_animate, NULL);
	
	//Take start time
	time(&start_time);
}

static void tap_handler(AccelAxisType axis, int32_t direction){
	AccelData check; accel_service_peek(&check);
	if(check.did_vibrate){return;} //if vibrating, don't activate
	
	if(axis==ACCEL_AXIS_Z && direction == 1){
		if(start_washing == 1) //if currently washing, stop washing
			end_washing_handler();
	}
	else if(axis == ACCEL_AXIS_X && direction == +1){
		if(start_washing == 0) //if not washing, start washing
			start_washing_handler();
	}
}

/* ==================================
General initialisation and graphics */
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  //Set status bar
  text_status = text_layer_create((GRect) { .origin = { 5, 5 }, .size = { bounds.size.w, 40 } });
  text_layer_set_text(text_status, USER);
  layer_add_child(window_layer, text_layer_get_layer(text_status));
  text_layer_set_font(text_status, fonts_get_system_font("RESOURCE_ID_GOTHIC_24"));
  
  //Set logo
  app_logo = text_layer_create((GRect) { .origin = {5, 130 }, .size = {bounds.size.w, 40 } });
  text_layer_set_text(app_logo, "Sw by Crwire (C) 2014");
  layer_add_child(window_layer, text_layer_get_layer(app_logo));
  
  //Set current day
  curr_day_layer = text_layer_create((GRect) { .origin = {0, 110 }, .size = {bounds.size.w, 20 } });
  text_layer_set_text_alignment(curr_day_layer, GTextAlignmentRight);
  text_layer_set_text(curr_day_layer, "Day: XX");
  static char my_days[] = "Day: 00";
  snprintf(my_days, 8, "Day: %02d", current_day);
  text_layer_set_text(curr_day_layer, my_days);
  layer_add_child(window_layer, text_layer_get_layer(curr_day_layer));
  
  //Set seconds text
  seconds_layer = text_layer_create((GRect) { .origin = {0, 45 }, .size = {bounds.size.w, 70 } });
  text_layer_set_text(seconds_layer, DEFAULT_MESSAGE);
  text_layer_set_text_alignment(seconds_layer, GTextAlignmentCenter);
  text_layer_set_font(seconds_layer, fonts_get_system_font("RESOURCE_ID_BITHAM_42_LIGHT"));
  layer_add_child(window_layer, text_layer_get_layer(seconds_layer));
}

static void window_unload(Window *window) { //Cleanup resources
  text_layer_destroy(text_status);
  text_layer_destroy(app_logo);
  text_layer_destroy(curr_day_layer);
  text_layer_destroy(seconds_layer);
}

static void init(void) {
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  
  accel_tap_service_subscribe(tap_handler); //Tap handler for gestures
  //Restore data from persistent storage
  total_washes = persist_exists(NUM_WASHES_KEY) ? persist_read_int(NUM_WASHES_KEY) : NUM_WASHES_DEFAULT;
  current_day = persist_exists(CURR_DAY_KEY) ? persist_read_int(CURR_DAY_KEY) : CURR_DAY_DEFAULT;
  if(persist_exists(DAY_END_KEY)){ end_of_day = (time_t) persist_read_int(DAY_END_KEY); persist_delete(DAY_END_KEY); countdown_timer = app_timer_register(10, (AppTimerCallback) check_if_day_end, NULL); }
  start_day = persist_exists(DAY_STARTED_KEY) ? persist_read_int(DAY_STARTED_KEY) : MFALSE;
  
  //Restore curr_day_data
  if(persist_exists(DAY_DATA_TEMP)){
  	  persist_read_string(DAY_DATA_TEMP, curr_day_data, sizeof(curr_day_data));
  }
  
  window_stack_push(window, animated);
}

static void deinit(void) {
  //Save data to persistent storage
  persist_write_int(NUM_WASHES_KEY, total_washes);
  persist_write_int(CURR_DAY_KEY, current_day);
  if(time(NULL) < end_of_day){ persist_write_int(DAY_END_KEY, (int) end_of_day); }
  if(start_day == MTRUE){ persist_write_int(DAY_STARTED_KEY, start_day); }
  //Save curr_day data to temporary storage
  if(persist_exists(DAY_DATA_TEMP)){ persist_delete(DAY_DATA_TEMP); }  
  persist_write_string(DAY_DATA_TEMP, curr_day_data);
  
  window_destroy(window);
  accel_tap_service_unsubscribe();
}

int main(void) {
  init();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);
  app_event_loop();
  deinit();
}
