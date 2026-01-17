#include "vertical_list.h"
#include <math.h>
#include <string.h>

// 默认动画配置
void vertical_list_set_default_anim_config(list_anim_config_t *cfg) {
  if (!cfg) return;
  cfg->easing_func = QuadraticEaseInOut; // 平滑缓动函数
  cfg->anim_duration_ticks = 15;         // 动画时长（可调整）
  cfg->anim_interval_ticks = 0;
}

// 默认显示配置（预留短横线空间）
void vertical_list_set_default_display_config(list_display_config_t *cfg) {
  if (!cfg) return;
  cfg->font = u8g2_font_6x10_tf;
  cfg->screen_width = 128;
  cfg->screen_height = 64;
  cfg->item_height = 14;
  cfg->text_x = 12;  // 短横线预留空间（X=4~8）
  cfg->start_y = 14;
  cfg->box_round = 3;
  cfg->box_height = 12;
  cfg->padding_x = 5;
  cfg->scroll_bar_width = 4;
}

// 添加列表项
int vertical_list_add_item(vertical_list_t *list, const char *title,
                           void (*on_click)(void *ctx)) {
  if (!list || !title || list->item_count >= MAX_LIST_ITEMS) return -1;
  
  list->items[list->item_count].title = title;
  list->items[list->item_count].on_click = on_click;
  list->item_count++;
  return list->item_count - 1;
}

// 仅按键触发：启动动画切换
void vertical_list_switch_item(vertical_list_t *list, int target_index) {
  if (!list || target_index < 0 || target_index >= list->item_count) return;
  
  if (list->to_index != target_index) {
    list->from_index = list->to_index;
    list->to_index = target_index;
    list->start_tick = *list->main_tick;
    list->last_switch_tick = *list->main_tick;
  }
}

// 核心绘制：有回调的项显示短横线，仅按键触发动画
void vertical_list_draw(u8g2_t *u8g2, void *ctx) {
  vertical_list_t *list = (vertical_list_t *)ctx;
  if (!list || !list->main_tick) return;

  // 动画进度锁死（无按键时t=1，静止）
  uint32_t elapsed = *list->main_tick - list->start_tick;
  float t = (float)elapsed / (float)list->anim_cfg.anim_duration_ticks;
  t = (t > 1.0f) ? 1.0f : t;
  float eased_t = list->anim_cfg.easing_func(t);

  // 计算当前位置
  float current_pos = list->from_index + (list->to_index - list->from_index) * eased_t;
  current_pos = (current_pos < 0) ? 0 : current_pos;
  current_pos = (current_pos >= list->item_count) ? (list->item_count - 1) : current_pos;

  // 滚动偏移计算
  int visible_items = (list->disp_cfg.screen_height - 2) / list->disp_cfg.item_height;
  int scroll_offset = (int)current_pos - (visible_items / 2);
  scroll_offset = (scroll_offset < 0) ? 0 : scroll_offset;
  int max_scroll = list->item_count - visible_items;
  scroll_offset = (scroll_offset > max_scroll) ? max_scroll : scroll_offset;

  // 清空缓冲区
  u8g2_ClearBuffer(u8g2);

  // 绘制列表项 + 有回调的项显示短横线
  u8g2_SetFont(u8g2, list->disp_cfg.font);
  u8g2_SetDrawColor(u8g2, 1);
  for (int i = scroll_offset; (i < scroll_offset + visible_items) && (i < list->item_count); i++) {
    int y = list->disp_cfg.start_y + (i - scroll_offset) * list->disp_cfg.item_height;
    if (y >= list->disp_cfg.start_y && y < list->disp_cfg.screen_height) {
      // 核心修改：仅on_click不为空时，绘制短横线（-）
      if (list->items[i].on_click != NULL) {
        // 短横线坐标：X从4到8，Y为文字基线-3（水平居中）
        u8g2_DrawLine(u8g2, 4, y - 3, 8, y - 3);
      }
      // 绘制菜单文字
      u8g2_DrawStr(u8g2, list->disp_cfg.text_x, y, list->items[i].title);
    }
  }

  // 绘制平滑移动的高亮框
  int base_y = list->disp_cfg.start_y - 10;
  float float_rel_index = current_pos - scroll_offset;
  int box_y = base_y + (int)(float_rel_index * list->disp_cfg.item_height);
  box_y = (box_y < 0) ? 0 : box_y;
  box_y = (box_y + list->disp_cfg.box_height > list->disp_cfg.screen_height) ? 
          (list->disp_cfg.screen_height - list->disp_cfg.box_height - 1) : box_y;

  // 高亮框宽度平滑过渡
  int from_width = u8g2_GetStrWidth(u8g2, list->items[list->from_index].title) + list->disp_cfg.padding_x * 2;
  int to_width = u8g2_GetStrWidth(u8g2, list->items[list->to_index].title) + list->disp_cfg.padding_x * 2;
  int box_w = (int)(from_width + (to_width - from_width) * eased_t);
  box_w = (box_w < list->disp_cfg.padding_x * 2) ? list->disp_cfg.padding_x * 2 : box_w;

  // 绘制圆角高亮框
  int box_x = list->disp_cfg.text_x - list->disp_cfg.padding_x;
  u8g2_DrawRBox(u8g2, box_x, box_y, box_w, list->disp_cfg.box_height, list->disp_cfg.box_round);

  // 绘制反色文字
  u8g2_SetDrawColor(u8g2, 0);
  int text_y = base_y + (int)(float_rel_index * list->disp_cfg.item_height) + 10;
  u8g2_DrawStr(u8g2, list->disp_cfg.text_x, text_y, list->items[(int)roundf(current_pos)].title);
  u8g2_SetDrawColor(u8g2, 1);

  // 绘制滚动条
  float progress = current_pos / (list->item_count - 1);
  int bar_height = (int)(progress * (list->disp_cfg.screen_height - 2));
  bar_height = (bar_height < 1) ? 1 : bar_height;
  bar_height = (bar_height > list->disp_cfg.screen_height - 2) ? (list->disp_cfg.screen_height - 2) : bar_height;
  int scroll_bar_x = list->disp_cfg.screen_width - list->disp_cfg.scroll_bar_width;
  u8g2_DrawBox(u8g2, scroll_bar_x, 0, list->disp_cfg.scroll_bar_width, bar_height);

  // 刷新屏幕
  u8g2_SendBuffer(u8g2);
}