#ifndef VERTICAL_LIST_H
#define VERTICAL_LIST_H

// 仅包含必要头文件，不引用任何不存在的类型
#include "easing.h"
#include "u8g2.h"

// 最大列表项数量
#define MAX_LIST_ITEMS 32
// 页面栈最大深度（和你的page_stack.h保持一致）
#define PAGE_STACK_MAX_DEPTH 8

// 列表项结构体（名称 + 点击后的显示函数）
typedef struct {
  const char *title;           // 列表项名称
  void (*on_click)(void *ctx); // 点击回调（显示函数）
} list_item_t;

// 动画配置结构体
typedef struct {
  AHEasingFunction easing_func; // 缓动函数
  uint32_t anim_duration_ticks; // 动画时长（tick数）
  uint32_t anim_interval_ticks; // 动画间隔（tick数）
} list_anim_config_t;

// 显示配置结构体（核心修正：字体用uint8_t数组指针存储）
typedef struct {
  const uint8_t *font;      // U8g2字体数组指针（匹配实际类型）
  uint8_t screen_width;     // 屏幕宽度
  uint8_t screen_height;    // 屏幕高度
  uint8_t item_height;      // 列表项高度（含间距）
  uint8_t text_x;           // 文字X偏移
  uint8_t start_y;          // 第一行文字基线Y
  uint8_t box_round;        // 高亮框圆角
  uint8_t box_height;       // 高亮框高度
  uint8_t padding_x;        // 高亮框X内边距
  uint8_t scroll_bar_width; // 滚动条宽度
} list_display_config_t;

// 列表状态结构体（作为页面栈的ctx）
typedef struct {
  list_item_t items[MAX_LIST_ITEMS]; // 列表项数组
  uint8_t item_count;                // 实际列表项数量
  list_anim_config_t anim_cfg;       // 动画配置
  list_display_config_t disp_cfg;    // 显示配置

  // 运行时状态
  int from_index;            // 动画起始索引
  int to_index;              // 动画目标索引
  uint32_t start_tick;       // 动画开始tick
  uint32_t last_switch_tick; // 上次切换tick
  uint32_t *main_tick;       // 全局tick指针
} vertical_list_t;

// 默认配置初始化
void vertical_list_set_default_anim_config(list_anim_config_t *cfg);
void vertical_list_set_default_display_config(list_display_config_t *cfg);

// 列表项注册
int vertical_list_add_item(vertical_list_t *list, const char *title,
                           void (*on_click)(void *ctx));

// 列表绘制函数（适配页面栈的draw函数）
void vertical_list_draw(u8g2_t *u8g2, void *ctx);

// 手动切换选中项（外部控制用）
void vertical_list_switch_item(vertical_list_t *list, int target_index);

#endif