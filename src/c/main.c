#include <pebble.h>

/*
Code written by Jordan Donaldson using the Pebble SDK, C, and JavaScript. Code written using
the basic watch face tutorial provided on the Pebble Developers blog.
Find the tutorial here: https://developer.pebble.com/tutorials/watchface-tutorial/part1/

The code in this file handles the time, date, printing of weather, and battery level capabilities.
*/

//Pointers
//Main window
static Window *s_main_window;
//Text layers
static TextLayer *s_date_layer, *s_time_layer;
static TextLayer *s_weather_layer;
//Fonts
static GFont s_time_font, s_date_font;
static GFont s_weather_font;
//Images
static BitmapLayer *s_background_layer, *s_bt_icon_layer;
static GBitmap *s_background_bitmap, *s_bt_icon_bitmap;
//Other pointers
static int s_battery_level;
static Layer *s_battery_layer;

/*
Next steps:
Comment out weather stuff (kills too much battery)
Move date to underneath time
Try to get bluetooth working
Rearrange functions
*/

/*
update_time takes no arguments
Creates structure to hold local time and prints this time
  based on whether the clock is 24hour style or not
Creates array to hold date and prints this date
  in day/mon/## order
*/
static void update_time()
{
  //Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  //Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
           "%H:%M" : "%I:%M", tick_time);
  //Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
  
  //Create a date buffer
  static char date_buffer[16];
  strftime(date_buffer, sizeof(date_buffer), "%a %b %e", tick_time);
  text_layer_set_text(s_date_layer, date_buffer);
}

/*
tick_handler takes 2 arguments: the strucutre for the time
  and a value to describe that the time has changed
Function runs update_time function
Function also updates the weather conditions on watch face
  every 30minutes.
*/
static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
  update_time();
  
  //Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0)
  {
    //Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    
    //Add a key-value pair
    dict_write_uint8(iter,0,0);
    
    //Send the message!
    app_message_outbox_send();
  }
}

/*
battery_callback takes one argument: a value to describe
  if the battery percentage has changed
Function updates pointer to battery level and marks the layer
  as dirty (updates often)
*/
static void battery_callback(BatteryChargeState state)
{
  //Record the new battery level
  s_battery_level = state.charge_percent;
  
  //Update meter
  layer_mark_dirty(s_battery_layer);
}

/*
battery_update_proc takes 2 arguments: the layer and the context
Function creates a rectangle to represent the battery percentage
*/
static void battery_update_proc(Layer *layer, GContext *ctx)
{
  GRect bounds = layer_get_bounds(layer);
  
  //Find the width of the bar
  int width = (int)(float)(((float)s_battery_level / 100.0F) * 114.0F);
  
  //Draw the background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, GCornerNone, 0);
  
  //Draw the bar
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), GCornerNone, 0);
}

/*
bluetooth_callback takes 1 argument: a boolean representing whether
  the phone and the watch are connectedd or not
If the phone is connected to the watch, the image of the bluetooth
  does not display
If the phone is not connected to the watch, the image of the
  bluetooth displays and the watch issues a vibrating alert
*/
static void bluetooth_callback(bool connected)
{
  //Show icon if disconnected
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);
  
  if(!connected)
  {
    //Issue a vibrating alert
    vibes_double_pulse();
  }
}


/*
main_window_load takes 1 argument: the main window of the watch face
Function builds the watch face (using the following steps):
1. Gets information about the size of the window
2. Creates a TextLayer for the date and prints this above the time
3. Creates a GBitmap for the background image behind the time
  and prints this in the center of the window
4. Creates a TextLayer for the time and prints this in the middle
  of the window
5. Creates a TextLayer for the weather conditions and prints this
  underneath the time layer
6. Creates a rectangle to represent the battery level of the Pebble
7. Creates the bluetooth image to represent when the watch is not connected
*/
static void main_window_load(Window *window)
{
  //Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  //Create the TextLayer with specific bounds (date)
  s_date_layer = text_layer_create(
    GRect(0, 20,bounds.size.w,50));
  //Create GFont
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_18));
  //Set values for TextLayer (date)
  text_layer_set_background_color(s_date_layer,GColorBlack);
  text_layer_set_text_color(s_date_layer,GColorWhite);
  text_layer_set_text_alignment(s_date_layer,GTextAlignmentCenter);
  text_layer_set_font(s_date_layer,s_date_font);
  //Add it as a child layer to the Window's root layer (date)
  layer_add_child(window_layer,text_layer_get_layer(s_date_layer));
  
  //Create GBitmap (background)
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BACKGROUND);
  //Create BitmapLayer to display the GBitmap (background)
  s_background_layer = bitmap_layer_create(bounds);
  //Set the bitmap onto the layer and add to the window (background)
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));
  
  //Create the TextLayer with specific bounds (time)
  s_time_layer = text_layer_create(
    GRect(0, PBL_IF_ROUND_ELSE(58,52), bounds.size.w, 50));
  //Create GFont (time)
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_48));
  //Improve the layout to be more like a watchface (time)
  text_layer_set_background_color(s_time_layer,GColorClear);
  text_layer_set_text_color(s_time_layer,GColorBlack);
  text_layer_set_font(s_time_layer,s_time_font);
  text_layer_set_text_alignment(s_time_layer,GTextAlignmentCenter);
  //Add it as a child layer to the Window's root layer (time)
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
  //Create temperature layer (weather)
  s_weather_layer = text_layer_create(
    GRect(0, PBL_IF_ROUND_ELSE(125,120), bounds.size.w, 25));
  //Create second custom font, apply it and add to Window (weather)
  s_weather_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_18));
  text_layer_set_font(s_weather_layer,s_weather_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));
  //Style the text (weather)
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorWhite);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  
  //Create battery meter Layer (battery)
  s_battery_layer = layer_create(GRect(14, 54, 115, 2));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  //Add to Window(battery)
  layer_add_child(window_get_root_layer(window), s_battery_layer);
  
  //Create the Bluetooth icon GBitmap (bluetooth)
  s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);
  //Create the BitmapLayer to display the GBitmap (bluetooth)
  s_bt_icon_layer = bitmap_layer_create(GRect(59, 12, 30, 30));
  bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bt_icon_layer));
  //Show the correct state of the BT connection from the start (bluetooth)
  bluetooth_callback(connection_service_peek_pebble_app_connection());
}

/*
main_window_unload takes 1 argument: the main window of the
  watch face
Function destroys any TextLayers, GFonts, GBitmaps, or Bitmaps
  created in the main_window_load function
*/
static void main_window_unload(Window *window)
{
  //Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  
  //Unload GFont
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_date_font);
  
  //Destroy GBitmap
  gbitmap_destroy(s_background_bitmap);
  //Destroy BitmapLayer
  bitmap_layer_destroy(s_background_layer);
  
  //Destroy weather elements
  text_layer_destroy(s_weather_layer);
  fonts_unload_custom_font(s_weather_font);
  
  //Destroy the battery meter
  layer_destroy(s_battery_layer);
  
  //Destroy the objects associated with the bluetooth thing
  gbitmap_destroy(s_bt_icon_bitmap);
  bitmap_layer_destroy(s_bt_icon_layer);
}

/*
inbox_received_callback takes 2 arguments: a DictionaryIterator
  and the context
Function stores the data for the weather fetched from the JavaScript
  and only uses it if all of the data is available
*/
static void inbox_received_callback(DictionaryIterator *iterator, void *context)
{
  //Store incoming information
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char weather_layer_buffer[32];
  
  //Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);
  
  //If all data is available, use it
  if(temp_tuple && conditions_tuple)
  {
    snprintf(temperature_buffer, sizeof(temperature_buffer), "%dF", (int)temp_tuple->value->int32);
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
    
    //Assemble full string and display
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
  }
}

/*
inbox_dropped_callback takes 2 arguments: the reason the message
  dropped and the context
Function prints to the app log that the message dropped
*/
static void inbox_dropped_callback(AppMessageResult reason, void *context)
{
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

/*
outbox_failed_callback takes 3 arguments: a DictionaryIterator,
  the reason the outbox send failed, and the context
Function prints to the app log that the outbox send failed
*/
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context)
{
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

/*
outbox_sent_callback takes 2 arguments: a DictionaryIterator and the context
Function prints to the app log that the outbox send was a success
*/
static void outbox_sent_callback(DictionaryIterator *iterator, void *context)
{
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

/*
init takes no arguments
Function creates the elements of the watch face (in the following order):
1. Creates main Window element
2. Sets handlers to manage elements inside the Window
3. Shows the window on the watch and makes it animated
4. Registers TickTimerService to change the time
5. Ensures that the time is displayed from when the watch face
  is opened
6. Sets the background color of the window to black
7. Registers callbacks for the weather functionality
8. Opens AppMessage
9. Registers for battery level updates and ensures that
  battery level is displayed from when the watch face
  is opened
10. Registers for bluetooth connection updates
*/
static void init()
{
  //Create main Window element and assign to pointer
  s_main_window = window_create();
  //Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window,(WindowHandlers)
  {
    .load = main_window_load,
    .unload = main_window_unload
  });
  //Show the Window on the watch, with animated = true
  window_stack_push(s_main_window,true);

  //Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  //Make sure the time is displayed from the start
  update_time();
  
  //Sets background color of the Window to black
  window_set_background_color(s_main_window,GColorBlack);
  
  //Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  //Open AppMessage
  const int inbox_size = 128;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);
  
  //Register for battery level updates
  battery_state_service_subscribe(battery_callback);
  //Ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());
  
  //Register for Bluetooth connection updates
  connection_service_subscribe((ConnectionHandlers)
  {
    .pebble_app_connection_handler = bluetooth_callback
  });
}

/*
deinit takes no arguments
Function destroys the Window element when the user exits the
  watch face
*/
static void deinit()
{
  //Destroy Window
  window_destroy(s_main_window);
}

/*
main takes no arguments (void)
Main function builds watch face, waits for user action, 
  and destroys watch face when user exits watch face
*/
int main(void)
{
  init();
  app_event_loop();
  deinit();
}