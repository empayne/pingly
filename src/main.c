#include <pebble.h>

#define TEXT_LINE_HEIGHT 30

#define KEY_CONNECTED 0
#define KEY_LATENCY 1

#define PING_TIMEOUT 10000
#define BLUETOOTH_TIMEOUT 15000
#define WEAK_SIGNAL_LATENCY 500
  
#define WIFI_ANIMATION_START_DELAY 300
#define WIFI_ANIMATION_SPEED 300
#define WIFI_ANIMATION_END_DELAY 300

static Window *s_main_window;
static TextLayer *s_output_layer_upper;
static TextLayer *s_output_layer_lower;

static BitmapLayer *background_layer;
static GBitmap     *background_bitmap;

char* str_initial = "Press any key.";
char* str_pinging = "Pinging...";
char* str_connected = "Connected.";
char* str_disconnected = "Disconnected.";
char* str_bluetooth_timeout = "Bluetooth timeout.";
char* str_check_phone = "Check connection.";
char* str_empty = NULL;

static char send_message_locked;
static char buffer_latency[32]; // ex. will contain "latency: 123ms"

static AppTimer* bluetooth_timeout_apptimer;
static AppTimer* wifi_animation_apptimer;
static void* context_imageUpdate;
static int current_wifi_state;

static void updateImage (int identifier, Layer* window_layer) {
  GRect window_bounds = layer_get_bounds(window_layer);
  
  bitmap_layer_destroy(background_layer);
  gbitmap_destroy(background_bitmap);
  
  background_bitmap = gbitmap_create_with_resource(identifier);
  background_layer = bitmap_layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h - 2*TEXT_LINE_HEIGHT));
  bitmap_layer_set_bitmap(background_layer, background_bitmap);
  layer_add_child(window_get_root_layer(s_main_window), bitmap_layer_get_layer(background_layer));
}

static void bluetooth_timeout_callback(void* data) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Bluetooth timeout occurred...");
  bluetooth_timeout_apptimer = NULL;
  
  app_timer_cancel(wifi_animation_apptimer);
  wifi_animation_apptimer = NULL;
  
  Window *window = (Window *)context_imageUpdate;
  Layer *window_layer = window_get_root_layer(window);
  
  text_layer_set_text(s_output_layer_upper, str_bluetooth_timeout);
  text_layer_set_text(s_output_layer_lower, str_check_phone);
  updateImage(RESOURCE_ID_IMAGE_BLUETOOTH_FAIL, window_layer);
  
  send_message_locked = 0;
}

static void wifi_animation_callback (void* data) {
  Window *window = (Window *)context_imageUpdate;
  Layer *window_layer = window_get_root_layer(window);
  int currentImage = (int) data;
  int nextImage = -1;
  int delay = -1;
  
  switch(currentImage) {
    case RESOURCE_ID_IMAGE_WIFI_0:
      nextImage = RESOURCE_ID_IMAGE_WIFI_1;
      delay = WIFI_ANIMATION_SPEED;
      break;
    case RESOURCE_ID_IMAGE_WIFI_1:
      nextImage = RESOURCE_ID_IMAGE_WIFI_2;
      delay = WIFI_ANIMATION_SPEED + WIFI_ANIMATION_END_DELAY;
      break;
    case RESOURCE_ID_IMAGE_WIFI_2:
      nextImage = RESOURCE_ID_IMAGE_BLANK;
      delay = WIFI_ANIMATION_SPEED;
      break;
    case RESOURCE_ID_IMAGE_BLANK:
      nextImage = RESOURCE_ID_IMAGE_WIFI_0;
      delay = WIFI_ANIMATION_SPEED;
      break;
  }
  
  updateImage(nextImage, window_layer);
  wifi_animation_apptimer = app_timer_register(delay, wifi_animation_callback, (void*) nextImage);
}



static void click_handler(ClickRecognizerRef recognizer, void *context) {
  
  
  if (!send_message_locked) {
    
    if (context_imageUpdate == NULL) {
      // Initialize this so that we can use it in timer, app callbacks.
      context_imageUpdate = context;
    }

    Window *window = (Window *)context_imageUpdate;
    Layer *window_layer = window_get_root_layer(window);
    
    send_message_locked = 1;
    bluetooth_timeout_apptimer = app_timer_register(BLUETOOTH_TIMEOUT, bluetooth_timeout_callback, NULL);
    
    text_layer_set_text(s_output_layer_upper,str_pinging);
    text_layer_set_text(s_output_layer_lower,str_empty);
    
    updateImage(RESOURCE_ID_IMAGE_WIFI_0, window_layer);
    wifi_animation_apptimer = app_timer_register(WIFI_ANIMATION_START_DELAY, wifi_animation_callback, (void*) RESOURCE_ID_IMAGE_WIFI_0);
    
    app_message_outbox_send();    
  }
  else {
    APP_LOG(APP_LOG_LEVEL_INFO, "Dropped keypress.");
  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Inbox message received!");

  app_timer_cancel(bluetooth_timeout_apptimer);
  bluetooth_timeout_apptimer = NULL;
  app_timer_cancel(wifi_animation_apptimer);
  wifi_animation_apptimer = NULL;
  
  
  send_message_locked = 0;
  
  int connected = -1;
  int latency = -1;
  
  Window *window = (Window *)context_imageUpdate;
  Layer *window_layer = window_get_root_layer(window);

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
    updateImage(latency > WEAK_SIGNAL_LATENCY ? RESOURCE_ID_IMAGE_SIGNAL_WEAK : RESOURCE_ID_IMAGE_SIGNAL_STRONG, window_layer);
  }
  else if (!connected && latency < PING_TIMEOUT) {
    // Disconnected, but not from timeout (ex. in airplane mode)
    snprintf(buffer_latency, sizeof(buffer_latency), "failed: %dms", latency);
    updateImage(RESOURCE_ID_IMAGE_SIGNAL_FAIL, window_layer);
  }
  else {
    // Disconnected (in the sense that latency > timeout)
    snprintf(buffer_latency, sizeof(buffer_latency), "timeout: %ds", (int) PING_TIMEOUT / 1000);
    updateImage(RESOURCE_ID_IMAGE_SIGNAL_FAIL, window_layer);
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
  background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PINGLY);
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
  // Initialize the lock for sending messages:
  send_message_locked = 0;
  bluetooth_timeout_apptimer = NULL;
  wifi_animation_apptimer = NULL;
  context_imageUpdate = NULL;
    
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