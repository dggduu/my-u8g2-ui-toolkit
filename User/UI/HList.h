#ifndef __HLIST_H__
#define __HLIST_H__

#include "btn_fifo.h"
#include "easing.h"
#include "page_stack.h"
#include "screen.h"

// ==========  配置项 ==========
#define ICON_WIDTH 			32
#define ICON_HEIGHT 		32
#define ICON_GAP 			50
// 配置文本使用的字体（可以使用screen_cfg->font）
#define HLIST_TEXT_FONT		u8g2_font_8x13_tr
//动画时长
#define HLIST_ANIM_TICKS 	15
// 图标使用的缓动动画
#define HLIST_ICON_ANIM		QuinticEaseInOut
#define HLIST_TEXT_ANIM		QuinticEaseOut

// ========== 结构体定义  ==========
typedef struct {
  bool guard_flag; // 保护标志：true=可进入，false=弹出提示
  char *alert_text;
} hlist_protect_t;

typedef struct {
  const char *title;
  const uint8_t *icon_xbm;
  const page_component_t *comp;
  void *ctx;
  hlist_protect_t protect;
} hlist_item_t;

typedef struct {
  hlist_item_t items[8];
  uint8_t count;
  int from_index, to_index;
  uint32_t start_tick;
  uint32_t *main_tick;
  struct {
    bool active; // 弹窗是否激活
    char *text;  // 提示文本
  } alert;
} hlist_t;

// ==========  外部定义 ==========
extern const page_component_t HLIST_COMP;

void hlist_init(hlist_t *hl, uint32_t *tick_ptr);
void hlist_add_item(hlist_t *hl, const char *title, const uint8_t *icon,
                    const page_component_t *comp, void *ctx);
void hlist_add_protected_item(hlist_t *hl, const char *title,
                              const uint8_t *icon, const page_component_t *comp,
                              void *ctx, bool guard_flag, char *alert_text);

#endif