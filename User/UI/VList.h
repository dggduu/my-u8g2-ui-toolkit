#ifndef __VLIST_H__
#define __VLIST_H__

#include "btn_fifo.h"
#include "easing.h"
#include "page_stack.h"
#include "screen.h"
// ========== 配置项  ==========
#define MAX_LIST_ITEMS 16
#define VLIST_ITEM_H 14
#define ALERT_TITLE		"alert"

extern const Screen_t g_screen_cfg;

typedef enum {
  VITEM_CLICK = 0,        // 按钮
  VITEM_NUM_EDIT,         // 数字
  VITEM_SUBMENU,          // 子菜单
  VITEM_ACTION,           // 普通（不传入Callback时可以当作文本使用）
  VITEM_PROTECTED_SUBMENU // 带保护的子菜单
} vitem_type_t;

typedef struct {
  const char *title;
  vitem_type_t type;
  void *user_data; // bool, float 或 vlist_t，存储用户数据
  float min, max, step;
  void (*callback)(void *ctx);
  // 保护子菜单扩展字段
  bool guard_flag;  // 保护标志
  char *alert_text; // 保护提示文本
} vitem_t;

typedef struct {
  vitem_t items[MAX_LIST_ITEMS];
  uint8_t count;
  int from_index;
  int to_index;
  uint32_t start_tick;
  uint32_t *main_tick;

  // 数字编辑器状态
  struct {
    bool active;
    int target_idx;
  } editor;

  // 提示弹窗状态
  struct {
    bool active; // 弹窗是否激活
    char *text;  // 提示文本
  } alert;
} vlist_t;

extern const page_component_t VLIST_COMP;

// 注册函数
void vlist_init(vlist_t *list, uint32_t *tick_ptr);
void vlist_add_toggle(vlist_t *list, const char *title, bool *val);
void vlist_add_num(vlist_t *list, const char *title, float *val, float min,
                   float max, float step);
void vlist_add_submenu(vlist_t *list, const char *title, vlist_t *child);
void vlist_add_action(vlist_t *list, const char *title, void (*cb)(void *ctx),
                      void *ctx);
void vlist_add_text(vlist_t *list, const char *title);
void vlist_add_protected_submenu(vlist_t *list, const char *title,
                                 vlist_t *child, bool guard_flag,
                                 char *alert_text);

#endif