#include <pebble.h>

#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_weather_layer;

static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static GFont s_time_font;
static GFont s_weather_font;

/******************************************************************************/
/******************************************************************************/

static BitmapLayer *s_bluetooth_icon_layer;
static GBitmap *s_bluetooth_icon_bitmap;

static TextLayer *s_date_layer;
static GFont s_date_font;

static int s_battery_level;
static Layer *s_battery_layer;

static TextLayer *s_local_time_layer;

static BitmapLayer *s_black_background_layer;
static GBitmap *s_black_background_bitmap;
static TextLayer *s_de_text_layer;
static TextLayer *s_de_time_layer;
static TextLayer *s_in_text_layer;
static TextLayer *s_in_time_layer;

static TextLayer *s_cur_xchange_layer;
static GFont s_cur_xchange_font;

/******************************************************************************/
/******************************************************************************/

static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
  
  // Update meter
  layer_mark_dirty(s_battery_layer);
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int i, level = 10 - (s_battery_level / 10);
  APP_LOG(APP_LOG_LEVEL_ERROR, "Battery Level Changed to %d.", level);
  
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_draw_rect(ctx, GRect(0, 2, 2, bounds.size.h - 4));
  graphics_draw_rect(ctx, GRect(2, 0, bounds.size.w - 2, bounds.size.h));
  
  for(i = 0 /* level * 2 */; i < 20; i += 2) {
    graphics_draw_line(ctx, GPoint(4 + i, 2), GPoint(4 + i, bounds.size.h - 3));
  }
}

/******************************************************************************/
/******************************************************************************/

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char weather_layer_buffer[32];

  // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, KEY_CONDITIONS);

  // If all data is available, use it
  if(temp_tuple && conditions_tuple) {
    snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)temp_tuple->value->int32);
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);

    // Assemble full string and display
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

/******************************************************************************/
/******************************************************************************/

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
}

/******************************************************************************/
/******************************************************************************/

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();

  // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    // Send the message!
    app_message_outbox_send();
  }
}

/******************************************************************************/
/******************************************************************************/

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  
  
  // Create GBitmap
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH_ERROR);

  // Create BitmapLayer to display the GBitmap
  s_background_layer = bitmap_layer_create(bounds);

  // Set the bitmap onto the layer and add to the window
  //bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  //layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));

  
  
  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Create GFont
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_COMIC_SANS_10));

  // Apply to TextLayer
  //text_layer_set_font(s_time_layer, s_time_font);

  // Add it as a child layer to the Window's root layer
  //layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  
  
  // Create temperature Layer
  s_weather_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(125, 120), bounds.size.w, 25));

  // Style the text
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorBlack);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  text_layer_set_text(s_weather_layer, "Loading...");

  // Create second custom font, apply it and add to Window
  s_weather_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_COMIC_SANS_10));
  //text_layer_set_font(s_weather_layer, s_weather_font);
  //layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));
  
  ////////////////////////////////////////////////////////////////////////////////
  
  s_bluetooth_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH_ERROR);
  s_bluetooth_icon_layer = bitmap_layer_create(GRect(1, 1, 11, 11));
  bitmap_layer_set_bitmap(s_bluetooth_icon_layer, s_bluetooth_icon_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bluetooth_icon_layer));
  
  s_cur_xchange_layer = text_layer_create(GRect(14, 1, 102, 12));
  text_layer_set_background_color(s_cur_xchange_layer, GColorClear);
  text_layer_set_text_color(s_cur_xchange_layer, GColorBlack);
  text_layer_set_text_alignment(s_cur_xchange_layer, GTextAlignmentCenter);
  text_layer_set_text(s_cur_xchange_layer, "1 USD = 66.37 INR");
  s_cur_xchange_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_COMIC_SANS_10));
  text_layer_set_font(s_cur_xchange_layer, s_cur_xchange_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_cur_xchange_layer));
  
  s_battery_layer = layer_create(GRect(118, 1, 25, 11));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  layer_add_child(window_get_root_layer(window), s_battery_layer);
  
  s_local_time_layer = text_layer_create(GRect(0, 13, bounds.size.w, 36));
  text_layer_set_background_color(s_local_time_layer, GColorBlack);
  text_layer_set_text_color(s_local_time_layer, GColorClear);
  text_layer_set_text_alignment(s_local_time_layer, GTextAlignmentCenter);
  text_layer_set_text(s_local_time_layer, "12:46:08");
  text_layer_set_font(s_local_time_layer, fonts_get_system_font(FONT_KEY_DROID_SERIF_28_BOLD));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_local_time_layer));
  
  s_date_layer = text_layer_create(GRect(0, 46, bounds.size.w, 20));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorBlack);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  text_layer_set_text(s_date_layer, "Mon, Mar 21st");
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
  
  s_black_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLACK_BACKGROUND);
  s_black_background_layer = bitmap_layer_create(GRect(0, 68, bounds.size.w, 20));
  bitmap_layer_set_bitmap(s_black_background_layer, s_black_background_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_black_background_layer));
  
  s_de_text_layer = text_layer_create(GRect(0, 66, 30, 20));
  text_layer_set_background_color(s_de_text_layer, GColorClear);
  text_layer_set_text_color(s_de_text_layer, GColorClear);
  text_layer_set_text_alignment(s_de_text_layer, GTextAlignmentCenter);
  text_layer_set_text(s_de_text_layer, "DEU");
  text_layer_set_font(s_de_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_de_text_layer));
  
  s_de_time_layer = text_layer_create(GRect(30, 66, 42, 20));
  text_layer_set_background_color(s_de_time_layer, GColorClear);
  text_layer_set_text_color(s_de_time_layer, GColorClear);
  text_layer_set_text_alignment(s_de_time_layer, GTextAlignmentCenter);
  text_layer_set_text(s_de_time_layer, "12:46");
  text_layer_set_font(s_de_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_de_time_layer));
  
  s_in_text_layer = text_layer_create(GRect(74, 66, 30, 20));
  text_layer_set_background_color(s_in_text_layer, GColorClear);
  text_layer_set_text_color(s_in_text_layer, GColorClear);
  text_layer_set_text_alignment(s_in_text_layer, GTextAlignmentCenter);
  text_layer_set_text(s_in_text_layer, "IND");
  text_layer_set_font(s_in_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_in_text_layer));
  
  s_in_time_layer = text_layer_create(GRect(102, 66, 42, 20));
  text_layer_set_background_color(s_in_time_layer, GColorClear);
  text_layer_set_text_color(s_in_time_layer, GColorClear);
  text_layer_set_text_alignment(s_in_time_layer, GTextAlignmentCenter);
  text_layer_set_text(s_in_time_layer, "12:46");
  text_layer_set_font(s_in_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_in_time_layer));
  
  
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);

  // Unload GFont
  fonts_unload_custom_font(s_time_font);

  // Destroy GBitmap
  gbitmap_destroy(s_background_bitmap);

  // Destroy BitmapLayer
  bitmap_layer_destroy(s_background_layer);

  // Destroy weather elements
  text_layer_destroy(s_weather_layer);
  fonts_unload_custom_font(s_weather_font);
  
  ////////////////////////////////////////////////////////////////////////////////
  
  bitmap_layer_destroy(s_bluetooth_icon_layer);
  
  text_layer_destroy(s_date_layer);
  fonts_unload_custom_font(s_date_font);
  
  layer_destroy(s_battery_layer);
  
  text_layer_destroy(s_local_time_layer);
  
  text_layer_destroy(s_de_text_layer);
  text_layer_destroy(s_de_time_layer);
  text_layer_destroy(s_in_text_layer);
  text_layer_destroy(s_in_time_layer);
  
  text_layer_destroy(s_cur_xchange_layer);
  fonts_unload_custom_font(s_cur_xchange_font);
  
}


/******************************************************************************/
/******************************************************************************/

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set the background color
  //window_set_background_color(s_main_window, GColorBlack);
  window_set_background_color(s_main_window, GColorClear);

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // Make sure the time is displayed from the start
  //update_time();

  // Register with TickTimerService
  //tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  // Register for battery level updates
  battery_state_service_subscribe(battery_callback);
  // Ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}