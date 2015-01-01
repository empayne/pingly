#include <pebble.h>

#define TEXT_LINE_HEIGHT 30

#define KEY_CONNECTED 0
#define KEY_LATENCY 1
  
#define PING_TIMEOUT 10000

static Window *s_main_window;
static TextLayer *s_output_layer_upper;
static TextLayer *s_output_layer_lower;

static BitmapLayer *background_layer;
static GBitmap     *background_bitmap;

char* str_initial = "Press any key.";
char* str_pinging = "Pinging...";
char* str_connected = "Connected.";
char* str_disconnected = "Disconnected.";
char* str_empty = NULL;

static char buffer_latency[32]; // ex. will contain "latency: 123ms"

static void click_handler(ClickRecognizerRef recognizer, void *context) {
  Window *window = (Window *)context;
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  
  text_layer_set_text(s_output_layer_upper,str_pinging);
  text_layer_set_text(s_output_layer_lower,str_empty);
  app_message_outbox_send();
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Inbox message received!");
  
  int connected = -1;
  int latency = -1;

  Tuple *t = dict_read_first(iterator);
  
  while(t != NULL) {
    switch(t->key)
    {
      case KEY_CONNECTED:
        APP_LOG(APP_LOG_LEVEL_INFO, "Connected: %d", (int)t->value->int32);
        connected = (int)t->value->int32;
        break;
      case KEY_LATENCY:
        APP_LOG(APP_LOG_LEVEL_INFO, "Latency: %d", (int)t->value->int32);
        latency = (int)t->value->int32;
        break;
      default:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
        break;
    }
    
    t = dict_read_next(iterator);
  }
  
  memset(buffer_latency, 0, sizeof(buffer_latency));
  
  if (connected) {
    // Connected, show latency:
    snprintf(buffer_latency, sizeof(buffer_latency), "latency: %dms", latency); 
  }
  else if (!connected && latency < PING_TIMEOUT) {
    // Disconnected, but not from timeout (ex. in airplane mode)
    snprintf(buffer_latency, sizeof(buffer_latency), "failed: %dms", latency); 
  }
  else {
    // Disconnected (in the sense that latency > timeout)
    snprintf(buffer_latency, sizeof(buffer_latency), "timeout: %ds", (int) PING_TIMEOUT / 1000); 
  }
  
  text_layer_set_text(s_output_layer_upper, connected ? str_connected : str_disconnected);
  text_layer_set_text(s_output_layer_lower, buffer_latency);
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

static void click_config_provider(void *context) {
  // Register the ClickHandlers:
  window_single_click_subscribe(BUTTON_ID_UP, click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, click_handler);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  
  // Create image layer. Will contain a PNG image indicating signal strength.
  background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLANK);
  background_layer = bitmap_layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h - 2*TEXT_LINE_HEIGHT));
  bitmap_layer_set_bitmap(background_layer, background_bitmap);
  layer_add_child(window_get_root_layer(s_main_window), bitmap_layer_get_layer(background_layer));

  // Create output TextLayer (upper)
  // ex. will contain "Press any key", "Pinging", "Connected", "Disconnected"
  s_output_layer_upper = text_layer_create(GRect(0, window_bounds.size.h - 2*TEXT_LINE_HEIGHT, window_bounds.size.w , window_bounds.size.h - TEXT_LINE_HEIGHT));
  text_layer_set_font(s_output_layer_upper, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_background_color(s_output_layer_upper, GColorBlack);
  text_layer_set_text_color(s_output_layer_upper, GColorWhite);
  text_layer_set_text(s_output_layer_upper, str_initial);
  text_layer_set_text_alignment(s_output_layer_upper, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_output_layer_upper, GTextOverflowModeWordWrap);
  layer_add_child(window_layer, text_layer_get_layer(s_output_layer_upper));
  
   // Create output TextLayer (lower)
  // ex. will contain "latency: 123ms", "failed: 10ms", "timeout: 10s"
  s_output_layer_lower = text_layer_create(GRect(0, window_bounds.size.h - TEXT_LINE_HEIGHT, window_bounds.size.w , window_bounds.size.h));
  text_layer_set_font(s_output_layer_lower, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_background_color(s_output_layer_lower, GColorBlack);
  text_layer_set_text_color(s_output_layer_lower, GColorWhite);
  text_layer_set_text(s_output_layer_lower, str_empty);
  text_layer_set_text_alignment(s_output_layer_lower, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_output_layer_lower, GTextOverflowModeWordWrap);
  layer_add_child(window_layer, text_layer_get_layer(s_output_layer_lower));
}

static void main_window_unload(Window *window) {
  // Destroy output TextLayer
  text_layer_destroy(s_output_layer_upper);
  text_layer_destroy(s_output_layer_lower);
  bitmap_layer_destroy(background_layer);
  gbitmap_destroy(background_bitmap);
  
}

static void init() {
  // Register AppMessage callbacks and open:
  app_message_register_inbox_received(inbox_received_callback);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Create main Window
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_stack_push(s_main_window, true);
}

static void deinit() {
  // Destroy main Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}