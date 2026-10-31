#include <pebble.h>
#include <pebble-fctx/fctx.h>
#include <pebble-fctx/ffont.h>

// Every position below is in the mockup's 200x220 space, scaled to the screen.
#define MOCK_W 200
#define MOCK_H 220
#define APEX_Y 143
#define PYRAMID_R 109    // base radius; off the screen at rest, so its corners are cut
#define ANIM_MS 2000
#define ANIM_SPIN_DEG 90

#define DATE_Y PBL_IF_ROUND_ELSE(36, 25)
#define DATE_CAP 11
#define TIME_Y PBL_IF_ROUND_ELSE(96, 85)
#define TIME_CAP PBL_IF_ROUND_ELSE(40, 48)
#define TIME_OUTLINE 1
#define TITLE_CAP 12
#define TITLE_Y PBL_IF_ROUND_ELSE(164, 180)
#define WEATHER_Y PBL_IF_ROUND_ELSE(182, 202)
#define WEATHER_W PBL_IF_ROUND_ELSE(140, 182)  // the rest is cut by the bezel or the screen edge

#define PERSIST_WEATHER 1
// 2 and 3 held colors in earlier builds, and may still be on the watch.
#define PERSIST_ANIM_HOUR 4
#define PERSIST_ANIM_FLICK 5
#define PERSIST_ANIM_LOAD 6

static Window *s_window;
static Layer *s_layer;
static FFont *s_font;
static char s_time[6];
static char s_date[16];
static char s_weather[64] = "No weather yet";
static Animation *s_anim;
static bool s_anim_hour;
static bool s_anim_flick;
static int s_anim_left;  // 0 at rest, up to ANIMATION_NORMALIZED_MAX as it starts

// The pyramid, seen level from the side, apex down. Base corners in degrees
// around it, 0 facing the viewer. A wide face looks straight on at rest with a
// narrow one each side, which is what lines up with the mockup's black and gray.
// The last corner is the first one again.
static const int16_t CORNERS[] = { 52, 90, 128, 232, 270, 308, 412 };
#define NUM_FACES (ARRAY_LENGTH(CORNERS) - 1)
#define FACE_IS_WIDE(i) (CORNERS[(i) + 1] - CORNERS[i] > 90)

typedef enum { SHADE_BLACK, SHADE_DARK, SHADE_LIGHT } Shade;

// How far the spin still has to go. Falls to 0, turning the faces right to left.
static int32_t prv_spin(void) {
  return s_anim_left * ANIM_SPIN_DEG / 360;
}

// Where the corner is, off center, at the top of the pyramid.
static fixed_t prv_corner_x(int i, int w) {
  const int32_t angle = DEG_TO_TRIGANGLE(CORNERS[i]) + prv_spin();
  return INT_TO_FIXED(PYRAMID_R) * w / MOCK_W * sin_lookup(angle) / TRIG_MAX_RATIO;
}

// Narrow faces catch the light as they turn to the viewer. Only four grays
// exist here, so it is a step; black and gray stay apart on every frame.
static Shade prv_face_shade(int i) {
  if (FACE_IS_WIDE(i)) {
    return SHADE_BLACK;
  }
  const int32_t mid = DEG_TO_TRIGANGLE((CORNERS[i] + CORNERS[i + 1]) / 2) + prv_spin();
  return cos_lookup(mid) > TRIG_MAX_RATIO * 7 / 10 ? SHADE_LIGHT : SHADE_DARK;
}

static fixed_t prv_apex_y(int h) {
  return INT_TO_FIXED(APEX_Y) * h / MOCK_H;
}

// The slide: how far above its place the pyramid is, as a negative y.
static fixed_t prv_top_y(int h) {
  return -prv_apex_y(h) * s_anim_left / ANIMATION_NORMALIZED_MAX;
}

#if !defined(PBL_BW)
static void prv_pyramid(FContext *f, int w, int h) {
  const fixed_t cx = INT_TO_FIXED(w / 2);
  const fixed_t top = prv_top_y(h);
  for (unsigned i = 0; i < NUM_FACES; i++) {
    const fixed_t x0 = prv_corner_x(i, w), x1 = prv_corner_x(i + 1, w);
    if (x1 <= x0) {
      continue;  // faces away; the ones left never overlap, so no sorting
    }
    // Half a pixel under the next visible face: antialiased edges that only meet let the background through.
    const fixed_t overlap = prv_corner_x((i + 1) % NUM_FACES + 1, w) > x1 ? FIXED_POINT_SCALE / 2 : 0;
    FPoint points[] = { FPoint(cx + x0, top), FPoint(cx + x1 + overlap, top), FPoint(cx, top + prv_apex_y(h)) };
    const Shade shade = prv_face_shade(i);
    fctx_set_fill_color(f, shade == SHADE_BLACK ? GColorBlack : shade == SHADE_DARK ? GColorDarkGray : GColorLightGray);
    fctx_begin_fill(f);
    fctx_draw_path(f, points, ARRAY_LENGTH(points));
    fctx_end_fill(f);
  }
}
#endif

static void prv_draw(FContext *f, const char *text, GColor color, int x, int y) {
  fctx_set_fill_color(f, color);
  fctx_set_offset(f, FPointI(x, y));
  fctx_begin_fill(f);
  fctx_draw_string(f, text, s_font, GTextAlignmentCenter, FTextAnchorBaseline);
  fctx_end_fill(f);
}

static void prv_text(FContext *f, const char *text, GColor color, int cap, int x, int y) {
  fctx_set_text_cap_height(f, s_font, cap);
  prv_draw(f, text, color, x, y);
}

#if defined(PBL_BW)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// No color here: the time was drawn black on white, and the pyramid goes in
// under it, inverting it where it crosses a black face. Grays are dithered.
static void prv_pyramid_under_time(GContext *ctx, int w, int h) {
  GBitmap *fb = graphics_capture_frame_buffer(ctx);
  const fixed_t height = prv_apex_y(h);
  const fixed_t apex = prv_top_y(h) + height;
  const int rows = MIN(FIXED_TO_INT(apex), h);
  for (unsigned i = 0; i < NUM_FACES; i++) {
    const fixed_t x0 = prv_corner_x(i, w), x1 = prv_corner_x(i + 1, w);
    if (x1 <= x0) {
      continue;  // faces away
    }
    const Shade shade = prv_face_shade(i);
    for (int y = 0; y < rows; y++) {
      GBitmapDataRowInfo row = gbitmap_get_data_row_info(fb, y);
      const fixed_t left = apex - INT_TO_FIXED(y);
      const int from = MAX(w / 2 + FIXED_TO_INT(x0 * left / height), row.min_x);
      const int to = MIN(w / 2 + FIXED_TO_INT(x1 * left / height), row.max_x + 1);
      for (int x = from; x < to; x++) {
        uint8_t *byte = &row.data[x / 8];
        const uint8_t bit = 1 << (x % 8);
        if (shade == SHADE_BLACK) {
          *byte ^= bit;
        } else if (shade == SHADE_DARK ? ((x + y) & 1) : ((x & 1) && (y & 1))) {
          *byte &= ~bit;
        }
      }
    }
  }
  graphics_release_frame_buffer(ctx, fb);
}
#endif

// The font only has capitals.
static void prv_upper(char *text) {
  for (char *c = text; *c; c++) {
    if (*c >= 'a' && *c <= 'z') {
      *c -= 'a' - 'A';
    }
  }
}

static void prv_update(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);
  const int w = bounds.size.w, h = bounds.size.h;
  const int cx = w / 2;
#define SY(y) ((y) * h / MOCK_H)
#define SX(x) ((x) * w / MOCK_W)

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  FContext f;
  fctx_init_context(&f, ctx);
#if defined(PBL_BW)
  prv_text(&f, s_time, GColorBlack, SX(TIME_CAP), cx, SY(TIME_Y));
  fctx_deinit_context(&f);
  prv_pyramid_under_time(ctx, w, h);
  fctx_init_context(&f, ctx);
#else
  prv_pyramid(&f, w, h);
  // Thin outline: the time in black, once per neighbouring pixel, under the fill.
  // While animating only the four corners, as every frame counts.
  const int step = s_anim_left ? 2 * TIME_OUTLINE : 1;
  for (int dy = -TIME_OUTLINE; dy <= TIME_OUTLINE; dy += step) {
    for (int dx = -TIME_OUTLINE; dx <= TIME_OUTLINE; dx += step) {
      if (dx || dy) {
        prv_text(&f, s_time, GColorBlack, SX(TIME_CAP), cx + dx, SY(TIME_Y) + dy);
      }
    }
  }
  prv_text(&f, s_time, GColorYellow, SX(TIME_CAP), cx, SY(TIME_Y));
#endif
  prv_text(&f, s_date, GColorWhite, SX(DATE_CAP), cx, SY(DATE_Y));

  // The Board speaks once the pyramid is down.
  if (!s_anim_left) {
    prv_text(&f, "THE BOARD", PBL_IF_COLOR_ELSE(GColorRed, GColorBlack), SX(TITLE_CAP), cx, SY(TITLE_Y));

    char line[sizeof(s_weather) + 4];
    snprintf(line, sizeof(line), "< %s >", s_weather);
    prv_upper(line);
    // Scaled to fill the line exactly, whatever its length: width at scale 1, then
    // a scale from that to the line. Cap heights are whole pixels, too coarse for this.
    fctx_set_scale(&f, FPoint(1, 1), FPoint(1, 1));
    const fixed_t width = fctx_string_width(&f, line, s_font);
    fctx_set_scale(&f, FPoint(width, -width), FPointI(SX(WEATHER_W), SX(WEATHER_W)));
    prv_draw(&f, line, GColorBlack, cx, SY(WEATHER_Y));
  }

  fctx_deinit_context(&f);
}

static void prv_anim_update(Animation *anim, const AnimationProgress progress) {
  s_anim_left = ANIMATION_NORMALIZED_MAX - progress;
  layer_mark_dirty(s_layer);
}

static void prv_anim_stopped(Animation *anim, bool finished, void *context) {
  s_anim = NULL;
  s_anim_left = 0;
  layer_mark_dirty(s_layer);
}

static void prv_anim_start(void) {
  if (s_anim) {
    return;
  }
  static const AnimationImplementation impl = { .update = prv_anim_update };
  s_anim = animation_create();
  animation_set_duration(s_anim, ANIM_MS);
  animation_set_curve(s_anim, AnimationCurveEaseOut);
  animation_set_implementation(s_anim, &impl);
  animation_set_handlers(s_anim, (AnimationHandlers) { .stopped = prv_anim_stopped }, NULL);
  s_anim_left = ANIMATION_NORMALIZED_MAX;
  animation_schedule(s_anim);
  layer_mark_dirty(s_layer);
}

static void prv_flick(AccelAxisType axis, int32_t direction) {
  prv_anim_start();
}

// Only subscribed while wanted: it keeps the accelerometer awake.
static void prv_flick_apply(void) {
  if (s_anim_flick) {
    accel_tap_service_subscribe(prv_flick);
  } else {
    accel_tap_service_unsubscribe();
  }
}

static void prv_tick(struct tm *now, TimeUnits changed) {
  strftime(s_time, sizeof(s_time), clock_is_24h_style() ? "%H:%M" : "%l:%M", now);
  // %l pads with a space, which would push the time off center.
  if (s_time[0] == ' ') {
    memmove(s_time, s_time + 1, sizeof(s_time) - 1);
  }
  strftime(s_date, sizeof(s_date), "%A %d", now);
  prv_upper(s_date);
  layer_mark_dirty(s_layer);
  if (s_anim_hour && (changed & HOUR_UNIT)) {
    prv_anim_start();
  }
}

static void prv_inbox(DictionaryIterator *iter, void *context) {
  Tuple *t = dict_find(iter, MESSAGE_KEY_WEATHER);
  if (t) {
    strncpy(s_weather, t->value->cstring, sizeof(s_weather) - 1);
    persist_write_string(PERSIST_WEATHER, s_weather);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_ANIM_HOUR))) {
    s_anim_hour = t->value->int32;
    persist_write_bool(PERSIST_ANIM_HOUR, s_anim_hour);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_ANIM_FLICK))) {
    s_anim_flick = t->value->int32;
    persist_write_bool(PERSIST_ANIM_FLICK, s_anim_flick);
    prv_flick_apply();
  }
  if ((t = dict_find(iter, MESSAGE_KEY_ANIM_LOAD))) {
    persist_write_bool(PERSIST_ANIM_LOAD, t->value->int32);
  }
  layer_mark_dirty(s_layer);
}

static void prv_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_layer = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_layer, prv_update);
  layer_add_child(root, s_layer);
}

static void prv_window_unload(Window *window) {
  layer_destroy(s_layer);
}

static void prv_init(void) {
  persist_read_string(PERSIST_WEATHER, s_weather, sizeof(s_weather));
  s_anim_hour = !persist_exists(PERSIST_ANIM_HOUR) || persist_read_bool(PERSIST_ANIM_HOUR);
  s_anim_flick = !persist_exists(PERSIST_ANIM_FLICK) || persist_read_bool(PERSIST_ANIM_FLICK);
  s_font = ffont_create_from_resource(RESOURCE_ID_FONT);

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  window_stack_push(s_window, true);

  time_t t = time(NULL);
  prv_tick(localtime(&t), MINUTE_UNIT);
  tick_timer_service_subscribe(MINUTE_UNIT, prv_tick);
  prv_flick_apply();
  // A watchface is relaunched on the way back from the menu.
  if (!persist_exists(PERSIST_ANIM_LOAD) || persist_read_bool(PERSIST_ANIM_LOAD)) {
    prv_anim_start();
  }

  app_message_register_inbox_received(prv_inbox);
  app_message_open(128, 64);
}

static void prv_deinit(void) {
  tick_timer_service_unsubscribe();
  accel_tap_service_unsubscribe();
  animation_unschedule_all();
  window_destroy(s_window);
  ffont_destroy(s_font);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
