#include <pebble.h>
  
#define TEXT_LINE_HEIGHT 30

static Window *s_main_window;
static TextLayer *s_output_layer_upper;
static TextLayer *s_output_layer_lower;

static BitmapLayer *background_layer;
static GBitmap     *background_bitmap;

char* str_initial = "Press any key.";
char* str_pinging = "Pinging...";
char* str_empty = NULL;

static void click_handler(ClickRecognizerRef recognizer, void *context) {
  Window *window = (Window *)context;
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  
  text_layer_set_text(s_output_layer_upper,str_pinging);
  
  /*bitmap_layer_destroy(background_layer);
  gbitmap_destroy(background_bitmap);
  
  background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TEST);
  background_layer = bitmap_layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h - 2*TEXT_LINE_HEIGHT));
  bitmap_layer_set_bitmap(background_layer, background_bitmap);
  layer_add_child(window_get_root_layer(s_main_window), bitmap_layer_get_layer(background_layer));*/
  
  
  
}

static void click_config_provider(void *context) {
  // Register the ClickHandlers
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
  s_output_layer_upper = text_layer_create(GRect(0, window_bounds.size.h - 2*TEXT_LINE_HEIGHT, window_bounds.size.w , window_bounds.size.h - TEXT_LINE_HEIGHT));
  text_layer_set_font(s_output_layer_upper, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_background_color(s_output_layer_upper, GColorBlack);
  text_layer_set_text_color(s_output_layer_upper, GColorWhite);
  text_layer_set_text(s_output_layer_upper, str_initial);
  text_layer_set_text_alignment(s_output_layer_upper, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_output_layer_upper, GTextOverflowModeWordWrap);
  layer_add_child(window_layer, text_layer_get_layer(s_output_layer_upper));
  
   // Create output TextLayer (lower)
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