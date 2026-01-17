#include "VList.h"
#include "u8g2.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief 工具函数，获取右侧元素宽度
 * @return 宽度（u8）
 */
static uint8_t get_right_item_width(u8g2_t *u8g2, const Screen_t *screen_cfg,
                                    const vitem_t *item) {
  if (u8g2 == NULL || screen_cfg == NULL || item == NULL)
    return 0;

  switch (item->type) {
  case VITEM_CLICK:
    return screen_cfg->click_switch_width;
  case VITEM_NUM_EDIT: {
    if (item->user_data == NULL)
      return screen_cfg->num_min_width;
    char buf[16];
    sprintf(buf, "%.1f", *(float *)item->user_data);
    int num_width = u8g2_GetStrWidth(u8g2, buf);
    return (num_width > screen_cfg->num_min_width ? num_width
                                                  : screen_cfg->num_min_width);
  }
  case VITEM_ACTION:
    return screen_cfg->action_width;
  case VITEM_SUBMENU:
  case VITEM_PROTECTED_SUBMENU: // 保护子菜单和普通子菜单右侧宽度一致
    return 0;
  default:
    return 0;
  }
}

/**
 * @brief  工具函数：绘制带停顿的滚动文本
 *
 * @param[in]  a
 * @param[in]  b
 *
 * @return
 */
static void draw_scroll_text_with_pause(u8g2_t *u8g2,
                                        const Screen_t *screen_cfg,
                                        const char *text, uint8_t start_x,
                                        uint8_t max_width, uint8_t y,
                                        uint32_t tick, uint8_t clip_y1,
                                        uint8_t clip_y2) {
  if (u8g2 == NULL || screen_cfg == NULL || text == NULL || max_width == 0)
    return;

  int text_width = u8g2_GetStrWidth(u8g2, text);
  if (text_width <= max_width) {
    u8g2_DrawStr(u8g2, start_x, y, text);
    return;
  }

  int gap = 20;
  int total_len = text_width + gap;
  int pause_ticks = screen_cfg->scroll_pause_ticks;
  int scroll_ticks = total_len * screen_cfg->scroll_speed_divisor;
  int total_cycle_ticks = pause_ticks + scroll_ticks;
  uint32_t cycle_tick = tick % total_cycle_ticks;

  if (cycle_tick < pause_ticks) {
    u8g2_SetClipWindow(u8g2, start_x, clip_y1, start_x + max_width, clip_y2);
    u8g2_DrawStr(u8g2, start_x, y, text);
    u8g2_SetMaxClipWindow(u8g2);
    return;
  }

  uint32_t scroll_start_tick = cycle_tick - pause_ticks;
  int offset = scroll_start_tick / screen_cfg->scroll_speed_divisor;
  int draw_x = start_x - offset;

  u8g2_SetClipWindow(u8g2, start_x, clip_y1, start_x + max_width, clip_y2);
  u8g2_DrawStr(u8g2, draw_x, y, text);
  u8g2_DrawStr(u8g2, draw_x + total_len, y, text);
  u8g2_SetMaxClipWindow(u8g2);
}

/**
 * @brief  绘制数值编辑弹窗
 *
 * @param[in]  a
 * @param[in]  b
 *
 * @return
 */
static void draw_num_window(u8g2_t *u8g2, const Screen_t *screen_cfg,
                            vlist_t *list) {
  if (u8g2 == NULL || screen_cfg == NULL || list == NULL ||
      list->editor.target_idx >= list->count) {
    return;
  }

  vitem_t *it = &list->items[list->editor.target_idx];
  if (it->type != VITEM_NUM_EDIT || it->user_data == NULL) {
    return;
  }

  float val = *(float *)it->user_data;
  char buf[16];

  int w = 100, h = 48;
  int x = (screen_cfg->width - w) / 2;
  int y = (screen_cfg->height - h) / 2;

  u8g2_SetDrawColor(u8g2, 0);
  u8g2_DrawBox(u8g2, x, y, w, h);
  u8g2_SetDrawColor(u8g2, 1);
  u8g2_DrawFrame(u8g2, x, y, w, h);

  u8g2_SetFont(u8g2, screen_cfg->font);

  uint8_t clip_y1 = y + 1;
  uint8_t clip_y2 = y + 13;

  int title_max_w = 60;
  draw_scroll_text_with_pause(u8g2, screen_cfg, it->title, x + 5, title_max_w,
                              y + 12, *list->main_tick, clip_y1, clip_y2);

  sprintf(buf, "%.1f", val);
  int num_width = u8g2_GetStrWidth(u8g2, buf);
  int num_max_w = 25;
  int num_start_x = x + w - num_max_w - 5;
  draw_scroll_text_with_pause(u8g2, screen_cfg, buf, num_start_x, num_max_w,
                              y + 12, *list->main_tick, clip_y1, clip_y2);

  int bx = x + 10, by = y + 22, bw = w - 20, bh = 7;
  u8g2_DrawFrame(u8g2, bx, by, bw, bh);
  float ratio = (val - it->min) / (it->max - it->min);
  if (it->max <= it->min)
    ratio = 0.0f;
  u8g2_DrawBox(u8g2, bx + 2, by + 2, (int)((bw - 4) * ratio), bh - 4);

  u8g2_SetFont(u8g2, screen_cfg->font);
  sprintf(buf, "%.1f", it->min);
  u8g2_DrawStr(u8g2, bx, y + 42, buf);

  sprintf(buf, "%.1f", it->step);
  u8g2_DrawStr(u8g2, x + (w - u8g2_GetStrWidth(u8g2, buf)) / 2, y + 42, buf);

  sprintf(buf, "%.1f", it->max);
  u8g2_DrawStr(u8g2, x + w - u8g2_GetStrWidth(u8g2, buf) - 10, y + 42, buf);
}

/**
 * @brief  绘制保护子菜单提示弹窗
 *
 * @param[in]  a
 * @param[in]  b
 *
 * @return
 */
static void draw_alert_window(u8g2_t *u8g2, const Screen_t *screen_cfg,
                              vlist_t *list) {
  if (u8g2 == NULL || screen_cfg == NULL || list == NULL ||
      !list->alert.active || list->alert.text == NULL) {
    return;
  }

  int w = 100, h = 35;
  int x = (screen_cfg->width - w) / 2;
  int y = (screen_cfg->height - h) / 2;

  u8g2_SetDrawColor(u8g2, 0);
  u8g2_DrawBox(u8g2, x, y, w, h);
  u8g2_SetDrawColor(u8g2, 1);
  u8g2_DrawFrame(u8g2, x, y, w, h);

  u8g2_SetFont(u8g2, screen_cfg->font);
  const char *alert_title = ALERT_TITLE;
  int title_width = u8g2_GetStrWidth(u8g2, alert_title);
  u8g2_DrawStr(u8g2, x + (w - title_width) / 2, y + 12, alert_title);

  u8g2_DrawHLine(u8g2, x + 5, y + 15, w - 10);

  uint8_t clip_y1 = y + 16;
  uint8_t clip_y2 = y + 28;
  int text_max_w = w - 10;
  draw_scroll_text_with_pause(u8g2, screen_cfg, list->alert.text, x + 5,
                              text_max_w, y + 27, *list->main_tick, clip_y1,
                              clip_y2);
}

/**
 * @brief  核心绘制逻辑
 *
 * @param[in]  a
 * @param[in]  b
 *
 * @return
 */
void vlist_draw(u8g2_t *u8g2, void *ctx) {
  if (u8g2 == NULL || ctx == NULL) {
    return;
  }

  vlist_t *list = (vlist_t *)ctx;
  const Screen_t *screen_cfg = &g_screen_cfg;

  if (list->count == 0) {
    return;
  }

  float p = fminf((float)(*list->main_tick - list->start_tick) /
                      screen_cfg->animation_duration,
                  1.0f);
  float ease_idx = list->from_index +
                   (list->to_index - list->from_index) * QuadraticEaseOut(p);

  int scroll_y = 0;
  if (ease_idx > 3.0f) {
    scroll_y = (int)((ease_idx - 3.0f) * (screen_cfg->font_height + 3));
  }

  u8g2_SetDrawColor(u8g2, 1);
  int bar_len = (int)(64.0f * ((float)(list->to_index + 1) / list->count));
  u8g2_DrawVLine(u8g2, screen_cfg->width - 1, 0, bar_len);

  u8g2_SetFont(u8g2, screen_cfg->font);

  for (int i = 0; i < list->count; i++) {
    int item_y = (i * (screen_cfg->font_height + 3)) - scroll_y +
                 screen_cfg->font_baseline + 2;
    if (item_y < -screen_cfg->font_height || item_y > screen_cfg->height) {
      continue;
    }

    vitem_t *curr_item = &list->items[i];

    // ========== 计算右侧元素信息 ==========
    uint8_t right_item_width =
        get_right_item_width(u8g2, screen_cfg, curr_item);
    uint8_t right_item_start_x =
        screen_cfg->width - right_item_width - screen_cfg->right_item_margin;

    // ========== 统一计算标题可用宽度 ==========
    uint8_t title_available_width = right_item_start_x -
                                    screen_cfg->title_left_margin -
                                    screen_cfg->right_item_left_padding;

    if (title_available_width <= 0)
      title_available_width = 1;

    bool is_highlighted = (i == list->to_index);
    int box_y = 0;
    int highlight_box_width = 0;

    // ========== 高亮框绘制和裁切区域计算 ==========
    uint8_t clip_y1, clip_y2;

    if (is_highlighted) {
      // 计算高亮框的实际位置
      box_y = (int)(ease_idx * (screen_cfg->font_height + 3)) - scroll_y + 2;

      // 计算高亮框宽度
      int title_text_width = u8g2_GetStrWidth(u8g2, curr_item->title);
      highlight_box_width = title_text_width + screen_cfg->highlight_padding;

      if (highlight_box_width > title_available_width) {
        highlight_box_width = title_available_width;
      }

      if (highlight_box_width < 20)
        highlight_box_width = 20;

      int highlight_box_x =
          screen_cfg->title_left_margin - (screen_cfg->highlight_padding / 2);
      if (highlight_box_x < 2)
        highlight_box_x = 2;

      // 绘制高亮框
      u8g2_SetDrawColor(u8g2, 1);
      u8g2_DrawRBox(u8g2, highlight_box_x, box_y, highlight_box_width,
                    screen_cfg->highlight_height, 2);

      clip_y1 = box_y + 1;
      clip_y2 = box_y + screen_cfg->highlight_height - 1;

      u8g2_SetDrawColor(u8g2, 0);
    } else {
      clip_y1 = item_y - screen_cfg->font_height + 2;
      clip_y2 = item_y + 2;
      u8g2_SetDrawColor(u8g2, 1);
    }

    if (curr_item->type == VITEM_SUBMENU ||
        curr_item->type == VITEM_PROTECTED_SUBMENU) {
      u8g2_DrawStr(u8g2, 5, item_y, "-");
    }

    // ========== 绘制标题 ==========
    draw_scroll_text_with_pause(
        u8g2, screen_cfg, curr_item->title, screen_cfg->title_left_margin,
        title_available_width, item_y, *list->main_tick, clip_y1, clip_y2);

    // ========== 绘制右侧元素 ==========
    u8g2_SetDrawColor(u8g2, 1);

    if (curr_item->type == VITEM_CLICK && curr_item->user_data != NULL) {
      bool v = *(bool *)curr_item->user_data;
      int switch_x = right_item_start_x;
      if (v) {
        u8g2_DrawBox(u8g2, switch_x, item_y - 8, 6, 6);
      } else {
        u8g2_DrawFrame(u8g2, switch_x, item_y - 8, 6, 6);
      }
    } else if (curr_item->type == VITEM_NUM_EDIT &&
               curr_item->user_data != NULL) {
      char b[16];
      sprintf(b, "%.1f", *(float *)curr_item->user_data);

      uint8_t num_max_width =
          right_item_width + screen_cfg->right_item_left_padding;
      draw_scroll_text_with_pause(u8g2, screen_cfg, b, right_item_start_x,
                                  num_max_width, item_y, *list->main_tick,
                                  clip_y1, clip_y2);
    }
  }

  // 绘制数值编辑弹窗
  if (list->editor.active) {
    draw_num_window(u8g2, screen_cfg, list);
  }

  // 绘制提示弹窗
  if (list->alert.active) {
    draw_alert_window(u8g2, screen_cfg, list);
  }
}

/**
 * @brief  初始化/注册函数
 *
 * @param[in]  a
 * @param[in]  b
 *
 * @return
 */
void vlist_init(vlist_t *list, uint32_t *tick_ptr) {
  if (list == NULL || tick_ptr == NULL) {
    return;
  }

  memset(list, 0, sizeof(vlist_t));
  list->main_tick = tick_ptr;
  list->from_index = 0;
  list->to_index = 0;
  list->start_tick = *tick_ptr;
  list->alert.active = false;
  list->alert.text = NULL;
}

void vlist_add_action(vlist_t *list, const char *title,
                      void (*callback)(void *), void *user_data) {
  if (list == NULL || title == NULL) {
    return;
  }

  if (list->count < MAX_LIST_ITEMS) {
    list->items[list->count].title = title;
    list->items[list->count].type = VITEM_ACTION;
    list->items[list->count].callback = callback;
    list->items[list->count].user_data = user_data;
    list->count++;
  }
}

void vlist_add_toggle(vlist_t *list, const char *title, bool *val) {
  if (list == NULL || title == NULL || val == NULL) {
    return;
  }

  if (list->count < MAX_LIST_ITEMS) {
    list->items[list->count].title = title;
    list->items[list->count].type = VITEM_CLICK;
    list->items[list->count].user_data = val;
    list->count++;
  }
}

void vlist_add_num(vlist_t *list, const char *title, float *val, float min,
                   float max, float step) {
  if (list == NULL || title == NULL || val == NULL || step <= 0) {
    return;
  }

  if (list->count < MAX_LIST_ITEMS) {
    vitem_t *it = &list->items[list->count++];
    it->title = title;
    it->type = VITEM_NUM_EDIT;
    it->user_data = val;
    it->min = min;
    it->max = max;
    it->step = step;
  }
}

void vlist_add_submenu(vlist_t *list, const char *title, vlist_t *child) {
  if (list == NULL || title == NULL || child == NULL) {
    return;
  }

  if (list->count < MAX_LIST_ITEMS) {
    list->items[list->count].title = title;
    list->items[list->count].type = VITEM_SUBMENU;
    list->items[list->count].user_data = child;
    list->count++;
  }
}

void vlist_add_text(vlist_t *list, const char *title) {
  if (list == NULL || title == NULL) {
    return;
  }
  if (list->count < MAX_LIST_ITEMS) {
    list->items[list->count].title = title;
    list->items[list->count].type = VITEM_ACTION;
    list->items[list->count].callback = NULL;
    list->items[list->count].user_data = NULL;
    list->count++;
  }
}

void vlist_add_protected_submenu(vlist_t *list, const char *title,
                                 vlist_t *child, bool guard_flag,
                                 char *alert_text) {
  if (list == NULL || title == NULL || child == NULL || alert_text == NULL) {
    return;
  }

  if (list->count < MAX_LIST_ITEMS) {
    vitem_t *it = &list->items[list->count++];
    it->title = title;
    it->type = VITEM_PROTECTED_SUBMENU;
    it->user_data = child;
    it->guard_flag = guard_flag;
    it->alert_text = alert_text;
  }
}

/**
 * @brief  输入处理函数
 *
 * @param[in]  a
 * @param[in]  b
 *
 * @return
 */
void vlist_input_handler(int btn, void *ctx) {
  if (ctx == NULL)
    return;

  vlist_t *list = (vlist_t *)ctx;

  // 优先处理提示弹窗
  if (list->alert.active) {
    if (btn == BTN_ENTER || btn == BTN_BACK) {
      list->alert.active = false; // 关闭弹窗
      list->alert.text = NULL;
    }
    return;
  }

  // 处理数值编辑器
  if (list->editor.active) {
    vitem_t *it = &list->items[list->editor.target_idx];
    if (list->editor.target_idx >= list->count || it->type != VITEM_NUM_EDIT ||
        !it->user_data) {
      list->editor.active = false;
      return;
    }

    float *val = (float *)it->user_data;
    if (btn == BTN_UP) {
      *val = fminf(*val + it->step, it->max);
    } else if (btn == BTN_DOWN) {
      *val = fmaxf(*val - it->step, it->min);
    } else if (btn == BTN_ENTER || btn == BTN_BACK) {
      list->editor.active = false;
    }
    return;
  }

  // 处理上下键滚动
  bool animation_triggered = false;
  if (btn == BTN_UP && list->to_index > 0) {
    list->from_index = list->to_index;
    list->to_index--;
    animation_triggered = true;
  } else if (btn == BTN_DOWN && list->to_index < list->count - 1) {
    list->from_index = list->to_index;
    list->to_index++;
    animation_triggered = true;
  }

  if (animation_triggered) {
    list->start_tick = *list->main_tick;
    return;
  }

  // 处理返回键
  if (btn == BTN_BACK) {
    page_stack_pop(&g_page_stack);
    return;
  }

  // 处理确认键
  if (btn == BTN_ENTER) {
    vitem_t *it = &list->items[list->to_index];
    switch (it->type) {
    case VITEM_CLICK:
      if (it->user_data)
        *(bool *)it->user_data = !*(bool *)it->user_data;
      break;
    case VITEM_NUM_EDIT:
      list->editor.active = true;
      list->editor.target_idx = list->to_index;
      break;
    case VITEM_SUBMENU:
      if (it->user_data) {
        vlist_t *child = (vlist_t *)it->user_data;
        child->from_index = 0;
        child->to_index = 0;
        child->start_tick = *child->main_tick;
        page_stack_push(&g_page_stack, &VLIST_COMP, child);
      }
      break;
    case VITEM_PROTECTED_SUBMENU:
      if (it->guard_flag) {
        // 保护标志为true，直接进入子菜单
        if (it->user_data) {
          vlist_t *child = (vlist_t *)it->user_data;
          child->from_index = 0;
          child->to_index = 0;
          child->start_tick = *child->main_tick;
          page_stack_push(&g_page_stack, &VLIST_COMP, child);
        }
      } else {
        // 保护标志为false，弹出提示弹窗
        list->alert.active = true;
        list->alert.text = it->alert_text;
      }
      break;
    case VITEM_ACTION:
      if (it->callback)
        it->callback(it->user_data);
      break;
    }
  }
}

const page_component_t VLIST_COMP = {vlist_draw, vlist_input_handler};