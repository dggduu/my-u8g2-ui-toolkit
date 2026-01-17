#include "splash_screen.h"
#include "btn_fifo.h"
#include <string.h>

// ===================== 静态全局变量 =====================
static hlist_t *s_hlist_ctx = NULL;       // 跳转目标HList上下文
static splash_draw_cb_t s_draw_cb = NULL; // 用户注册的绘制函数

// ===================== 组件核心逻辑 =====================
/**
 * @brief SplashScreen绘制函数
 */
static void splash_screen_draw(u8g2_t *u8g2, void *ctx) {
  (void)ctx;
  // 如果用户注册了绘制函数，则调用
  if (s_draw_cb != NULL) {
    s_draw_cb(u8g2, &g_screen_cfg);
  }
}

/**
 * @brief SplashScreen输入处理函数
 */
static void splash_screen_input(int btn, void *ctx) {
  (void)ctx;
  // 任意有效按键 -> 跳转到绑定的HList
  if (btn != BTN_NONE && s_hlist_ctx != NULL) {
    page_stack_push(&g_page_stack, &HLIST_COMP, s_hlist_ctx);
  }
}

/**
 * @brief 初始化SplashScreen
 */
void splash_screen_init(hlist_t *hlist_ctx, splash_draw_cb_t draw_cb) {
  s_hlist_ctx = hlist_ctx; // 绑定跳转目标
  s_draw_cb = draw_cb;     // 注册用户绘制函数
}

/**
 * @brief 跳转到SplashScreen
 */
void splash_screen_jump(void) {
  // 清空页面栈，确保SplashScreen为栈顶
  g_page_stack.top = 0;
  // 压入SplashScreen组件
  page_stack_push(&g_page_stack, &SPLASH_SCREEN_COMP, NULL);
}

const page_component_t SPLASH_SCREEN_COMP = {
    .draw = splash_screen_draw,  // 绘制
    .input = splash_screen_input // 核心跳转逻辑
};