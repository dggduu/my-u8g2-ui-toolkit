![](https://raw.githubusercontent.com/dggduu/my-u8g2-ui-toolkit/main/IMG/title.jpg)
## dggduu's OLED UI Toolkit
尝试做一套基于 u8g2 的 UI 工具集，目标是在裸机环境下能使用 MVC 设计模式进行多级菜单页面的开发。
- 中间层: u8g2  
测试平台:  
- stm32f103c8t6 （标准库实现）  
- SSD1306(128x64)  
### 例程空间使用情况
**Program Size: Code=22392 RO-data=18172 RW-data=72 ZI-data=5768**
### 推荐使用方式
```c
(splash_log-)SplashScreen - HList - VList - VList - ...
                              |- component  |- component
                              |- ...        |- ...
```
![](https://raw.githubusercontent.com/dggduu/my-u8g2-ui-toolkit/main/IMG/recommended-structure.jpg)
## 相关概念
### splash_log
一套用于提供类似终端输出的组件，基于 vsnprintf , 适用于初始化阶段，文本溢出后自动换行。注意该函数不是页面栈的一部分，通过调用 splash_log_printf 才会刷新屏幕缓冲区，不建议与会经常刷新缓冲区的函数一起使用。
### splash_screen
类似前端概念中的 应用启动页（splash_screen），可以用于锁屏界面，也可以当黑屏。使用通过添加全局回调或者使用 `splash_screen_jump()` 函数，可以实现在任意界面直接跳转到 splash_screen。  
### Portal
（仅对页面栈类组件有效，splash_log状态无法调用）  
类似 React 开发中的 portal 组件，用于将自定义绘制回调绘制在最顶层，当激活时将拦截原本要传入到底层的 btn_fifo 并传给相应 portal组件 的 input 回调  
#### 如何使用
portal 的使用很简单
```c
void page_stack_portal_toggle(page_stack_t *ps, const portal_component_t *comp,
                              void *ctx, size_t ctx_size);
// Exmaple
page_stack_portal_toggle(
    &g_page_stack, &PORTAL_MESSAGE_BOX,
    &(portal_ctx_message_box_t){.title = "Warning", .msg = it->alert_text},
    sizeof(portal_ctx_message_box_t));
```
**访问 `portal_component.h` 来查看相对应的 portal 上下文结构体定义以及portal组件的名称**  
#### 如何编写新的 portal 组件
访问 `portal_component.h`后，你会发现 portal 组件这样定义的
```c
const portal_component_t PORTAL_MESSAGE_BOX = {
    .draw = portal_messagebox_draw, 
    .input = portal_mbox_input,
    .w = 100, 
    .h = 35
};
```
是不是与组件的很像，`.w`代表该 portal 的宽度，`.h`代表该 portal 的显示高度，组件只能居中显示。
你需要编写
```c
// 推荐的命名规范 portal_*，portal_ctx_*

// portal 组件上下文
typedef struct {
    const char *title;
    const char *msg;
} portal_ctx_message_box_t;

// portal 组件绘制回调（注意：x,y:当前portal外框左上角第一个像素点的位置；w,h:当前Portal 外框的宽高）
static void portal_messagebox_draw(u8g2_t *u8g2, int16_t x, int16_t y, uint8_t w, uint8_t h, void *ctx)
// portal 组件输入回调
static void portal_message_box_input(int btn, void *ctx)
// 注册组件
const portal_component_t PORTAL_MESSAGE_BOX = {
    .draw = portal_messagebox_draw, 
    .input = portal_mbox_input,
    .w = 100, 
    .h = 35
};
// 导出组件
extern const portal_component_t PORTAL_MESSAGE_BOX;
```
### Protected 组件
灵感来自 Expo 中的Protected Route  
当 guard_flag 为 false 时弹出弹窗，并无法进行下一步操作（如跳转到下一个page,menu...）  
当 guard_flag 为 true 时不会弹出弹窗，直接进行下一步操作  
#### 子类
##### protected_action：进入自定义页面
##### protected_submenu：进入下一级List菜单
##### protected_progress(实验性功能，需要前往`portal_component.h` 将对应宏定义开启)
此组件目标是在无OS环境提供一个便捷的执行回调并打印对应进度的功能，适用于连接网络，上传表单等需要长时间等待的任务  
用于传入一个执行回调，并能够显示回调的进度，失败后显示`[error]`,成功显示`[OK]`。  
执行流程：  
用户点击组件->弹出portal,询问再次确认->portal与page的draw与input回调暂停执行，执行权交由自定义执行回调->执行回调可以通过`Progress_Log(ctx, msg)`刷新portal组件所在的屏幕缓冲区，以实现告知用户进度，在执行回调执行过程中允许多次调用->执行回调主要逻辑结束，执行`Progress_SetSuccess(ctx);`告知完成，如果失败则需要调用`Progress_SetFailed(ctx,msg);`告知失败并返回失败原因->执行权交由页面栈管理，此时portal与页面正常刷新，这是可以关闭页面。  
**这里的ctx是portal组件定义的，不需要用户手动编写，只需照抄即可**  

### 自定义组件
通过编写绘制回调（draw_handler）以及输入回调（input_handler）,并使用注册函数进行注册
```c
const page_component_t OSC_APP_COMP = {.draw = osc_app_draw,.input = osc_app_input};
```
通过该方式可以实现一个简单的MVC模式（Model: 全局变量/上下文，View：draw_handler，Controller：input_handler）  
你可以查看main.c(下面的例程)中的`OSC_APP_COMP`以及`User - UI - Component`下面看看如何使用  

## UTF8 support?
本工具集将文本绘制函数进行了一层抽象，使用 `screen.h` 配置的 `.draw_text` 回调进行文本绘制，配置为`screen_draw_utf8`即可将通用组件的文本渲染逻辑更换为UTF8。  
推荐自定义组件使用`screen.h`定义的变量，方便管理（这个文件就相当于dotenv）  
**注意：`.is_utf8` 只是一个标志位，用于告知需要对utf8进行适配的组件，不是切换渲染逻辑的开关，即使用utf8时需要手动开启**
## How to porting
将 `User - UI` 路径下的文件以及 btn_fifo 复制出来即可。btn_fifo.h 中定义了框架中所使用的按键类型，目前没有做按键类型桥接层的计划，需要自己实现。
## How to use
```c
#include "Delay.h"
#include "HList.h"
#include "VList.h"
#include "brick_break.h"
#include "btn_fifo.h"
#include "page_stack.h"
#include "screen.h"
#include "splash_log.h"
#include "splash_screen.h"
#include "stm32f10x.h"
#include "u8g2.h"
#include "uart.h"
#include "ui.h"
#include <math.h>
#include "portal_component.h"

extern const uint8_t icon_list[][128];

#define ICON_SETTINGS 0x0081 // 设置图标
#define ICON_ABOUT 0x0114    // 关于图标
#define ICON_LOCK 0x0057     // 叉叉图标
#define ICON_UNLOCK 0x0078   // 对勾图标

const Screen_t g_screen_cfg = DEFAULT_SCREEN_CONFIG;
u8g2_t u8g2;
hlist_t g_main_hlist;
vlist_t g_setting_main_menu;
vlist_t g_setting_sub_menu;
vlist_t g_about_menu;
brick_break_ctx_t g_brick_break_ctx;

bool g_wifi_state = false;
bool g_bt_state = true;
bool g_mute_mode = false;
float g_screen_brightness = 50.0f;
float test_num = 0;

// ===================== 自定义SplashScreen =====================
static void my_splash_draw(u8g2_t *u8g2, const Screen_t *screen_cfg) {
  static uint32_t tick_count = 0;
  tick_count++;

  // 电量绘制
  u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
  u8g2_DrawFrame(u8g2, screen_cfg->width - 20, 2, 18, 10);
  u8g2_DrawBox(u8g2, screen_cfg->width - 18, 4, 14, 6);
  u8g2_DrawStr(u8g2, screen_cfg->width - 40, 10, "80%");

  // 数字时钟
  uint8_t hour = 12, min = 30, sec = (tick_count / 10) % 60;
  char time_str[10];
  sprintf(time_str, "%02d:%02d:%02d", hour, min, sec);
  u8g2_SetFont(u8g2, u8g2_font_logisoso20_tn);
  int time_width = u8g2_GetStrWidth(u8g2, time_str);
  u8g2_DrawStr(u8g2, (screen_cfg->width - time_width) / 2,
               screen_cfg->height / 2 + 10, time_str);

  // 提示文字
  u8g2_SetFont(u8g2, u8g2_font_5x7_tf);
  const char *hint = "Press any btn to enter";
  u8g2_DrawStr(u8g2, (screen_cfg->width - u8g2_GetStrWidth(u8g2, hint)) / 2,
               screen_cfg->height - 5, hint);
}

// ===================== 全局按键回调 =====================
static void global_btn_handler(btn_type_t btn) {
  if (btn == BTN_LONG_PRESS) {
    splash_screen_jump();
  }
}

// ===================== 自定义Page =====================
static void osc_app_draw(u8g2_t *u8g2, void *ctx) {
  static uint8_t wave_offset = 0;
  wave_offset = (wave_offset + 1) % 255;

  u8g2_SetFont(u8g2, u8g2_font_6x10_tf);
  u8g2_DrawStr(u8g2, 2, 12, "OSCILLOSCOPE");
  u8g2_DrawHLine(u8g2, 0, 15, 128);

  for (uint8_t x = 0; x < 128; x++) {
    uint8_t y = 40 + (sin((x + wave_offset) * 0.15f) * 15);
    u8g2_DrawPixel(u8g2, x, y);
  }
  u8g2_DrawStr(u8g2, 10, 62, "BACK:Exit  LONG:Home");
}
// 自定义Page btn 消费函数
static void osc_app_input(int btn, void *ctx) {
  if (btn == BTN_BACK) {
    page_stack_pop(&g_page_stack);
  }
}
// 注册自定义组件
const page_component_t OSC_APP_COMP = {.draw = osc_app_draw,
                                       .input = osc_app_input};

void my_long_task(void *ctx) {
    Progress_Log(ctx, "Initializing...");
    Delay_ms(500);

    Progress_Log(ctx, "Erasing...");
    Delay_ms(800);

    for (int i = 0; i <= 100; i += 20) {
        Progress_Log(ctx, "Writing: %d%%", i);
        Delay_ms(200);
    }

    // 成功结束
    //Progress_SetSuccess(ctx);
  Progress_SetFailed(ctx,"no idea");
}
                     
// ===================== 菜单初始化 =====================
static void ui_menu_init(void) {
  // 初始化列表
  vlist_init(&g_setting_main_menu, &g_page_stack.main_tick);
  vlist_init(&g_setting_sub_menu, &g_page_stack.main_tick);
  vlist_init(&g_about_menu, &g_page_stack.main_tick);
  hlist_init(&g_main_hlist, &g_page_stack.main_tick);

  // 初始化打砖块游戏
  brick_break_init(&g_brick_break_ctx, &g_page_stack.main_tick, &g_screen_cfg);

  // 设置菜单
  vlist_add_toggle(&g_setting_sub_menu, "WIFI Link", &g_wifi_state);
  vlist_add_num(&g_setting_sub_menu, "Brightness", &g_screen_brightness, 0, 100,
                5);
  vlist_add_submenu(&g_setting_main_menu, "System Config", &g_setting_sub_menu);
  vlist_add_precise_num(&g_setting_main_menu, "Precise Num Test", &test_num,
                        -100, 200, 3,0);
  vlist_add_protected_progress(&g_setting_main_menu, "Save Config", my_long_task);
  vlist_add_toggle(&g_setting_main_menu, "Mute Mode", &g_mute_mode);
  vlist_add_toggle(&g_setting_main_menu, "Bluetooth", &g_bt_state);
  vlist_add_protected_submenu(&g_setting_main_menu, "Protect.false",
                              &g_setting_sub_menu, false, "warinng",
                              "Try Again!");
  vlist_add_protected_submenu(&g_setting_main_menu, "Protect.true",
                              &g_setting_sub_menu, true, "warinng",
                              "Try Again!");

  // 添加OSC组件入口
  vlist_add_action(&g_setting_main_menu, "Oscilloscope", &OSC_APP_COMP, NULL);
  // 添加打砖块游戏入口
  vlist_add_action(&g_setting_main_menu, "Brick Break", &BRICK_BREAK_COMP,
                   &g_brick_break_ctx);
  vlist_add_protected_action(&g_setting_main_menu, "Brick Break (locked)",
                             &BRICK_BREAK_COMP, &g_brick_break_ctx, false,
                             "warinng", "This action is locked!");
  vlist_add_protected_action(&g_setting_main_menu, "Brick Break (unlocked)",
                             &BRICK_BREAK_COMP, &g_brick_break_ctx, true,
                             "warinng", "This action is locked!");

  // 关于菜单
  vlist_add_plain_text(&g_about_menu, "Version: 0.0.1");
  vlist_add_plain_text(&g_about_menu, "Author: dggdoo");
  vlist_add_plain_text(&g_about_menu, "Build: 2026-01");

  // 主菜单
  hlist_add_glyph_item(&g_main_hlist, "SETTINGS", ICON_SETTINGS, &VLIST_COMP,
                       &g_setting_main_menu);
  hlist_add_xbm_item(&g_main_hlist, "OSCILLO", icon_list[3], &OSC_APP_COMP,
                     NULL);
  // 添加打砖块游戏到主菜单
  hlist_add_glyph_item(&g_main_hlist, "BRICK GAME", ICON_ABOUT,
                       &BRICK_BREAK_COMP, &g_brick_break_ctx);
  hlist_add_glyph_item(&g_main_hlist, "ABOUT", ICON_ABOUT, &VLIST_COMP,
                       &g_about_menu);
  hlist_add_protected_glyph_item(&g_main_hlist, "Protected.false", ICON_LOCK,
                                 &VLIST_COMP, &g_about_menu, false,
                                 "Try Again!");
  hlist_add_protected_glyph_item(&g_main_hlist, "Protected.true", ICON_UNLOCK,
                                 &VLIST_COMP, &g_about_menu, true,
                                 "Try Again!");

  // 初始化SplashScreen
  splash_screen_init(&g_main_hlist, my_splash_draw);
  // 注册全局按键回调
  page_stack_register_global_btn_cb(&g_page_stack, global_btn_handler);
  // 初始页面
  splash_screen_jump();
  // 如需直接进入Hlist，启用下面的函数，并将上面的SplashScreen相关的代码删除
  // page_stack_push(&g_page_stack, &HLIST_COMP, &g_main_hlist);
}

// ===================== 串口按键处理 =====================
static void uart_btn_process(void) {
  char ch = uart_dma_read_byte();
  if (ch != 0) {
    switch (ch) {
    case 'w':
      btn_fifo_push(BTN_UP);
      break;
    case 's':
      btn_fifo_push(BTN_DOWN);
      break;
    case 'a':
      btn_fifo_push(BTN_ENTER);
      break;
    case 'd':
      btn_fifo_push(BTN_BACK);
      break;
    case 'h':
      btn_fifo_push(BTN_LONG_PRESS);
      break;
    case 'l':
      btn_fifo_push(BTN_LEFT);
      break;
    case 'r':
      btn_fifo_push(BTN_RIGHT);
      break;
    default:
      break;
    }
    uart_debug_printf("rev:%c\r\n", ch);
  }
}

int main(void) {
  // 硬件初始化
  uart_init();
  btn_fifo_init();
  IIC_Init();
  u8g2Init(&u8g2);

  // 这一段可以直接去掉
  splash_log_init(&u8g2, g_screen_cfg.font_height, u8g2_font_5x7_tf);
  splash_log_clear();
  splash_log_printf("splash_log inited");
  Delay_ms(100);
  splash_log_printf("btn_fifo inited");
  Delay_ms(100);
  splash_log_printf("PWM inited");
  Delay_ms(100);
  splash_log_printf("UART inited");
  Delay_ms(100);
  splash_log_printf("...ok");
  Delay_ms(100);
  splash_log_printf("        /Nya!  Powered");
  splash_log_printf("   /|/|        By");
  splash_log_printf("  (- - |       dggduu's");
  splash_log_printf("   |、~\        U8g2 UI");
  splash_log_printf("  //_,)/       Toolkit");
  Delay_ms(3000);
  splash_log_printf("test_float %.1f", 12.8);
  splash_log_printf("test_interger %2d", 120);
  splash_log_printf("test_string %s", "hello world");
  splash_log_printf("test_ovweflow %s", "hello world sdhjsdjdshj");
  Delay_ms(200);
  splash_log_printf("ready to test clear");
  Delay_ms(1000);
  splash_log_clear();
  // 上面这些可以去掉
  page_stack_init(&g_page_stack, &u8g2);
  ui_menu_init();

  while (1) {
    uart_btn_process();
    btn_type_t btn = btn_fifo_pop();

    page_update(&g_page_stack, btn);
  }
}
```
## Special Thanks
[AHEasing](https://github.com/warrenm/AHEasing)