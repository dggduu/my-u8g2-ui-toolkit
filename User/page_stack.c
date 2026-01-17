#include "page_stack.h"

// 全局页面栈实例
page_stack_t g_page_stack;

void page_stack_init(page_stack_t *ps, u8g2_t *u8g2) {
    memset(ps, 0, sizeof(page_stack_t));
    ps->u8g2 = u8g2;
    ps->global_btn_handler = NULL;
}

int page_stack_push(page_stack_t *ps, const page_component_t *comp, void *ctx) {
    if (ps->top >= PAGE_STACK_MAX_DEPTH || comp == NULL) return -1;
    ps->stack[ps->top].comp = comp;
    ps->stack[ps->top].ctx = ctx;
    ps->top++;
    return 0;
}

int page_stack_pop(page_stack_t *ps) {
    if (ps->top > 1) { 
        ps->top--; 
        return 0; 
    }
    return -1;
}

page_t* page_stack_current(page_stack_t *ps) {
    return (ps->top > 0) ? &ps->stack[ps->top - 1] : NULL;
}

// 注册全局按键回调
void page_stack_register_global_btn_cb(page_stack_t *ps, global_btn_cb_t cb) {
    if (ps != NULL) {
        ps->global_btn_handler = cb;
    }
}

void page_update(page_stack_t *ps, btn_type_t btn) {
    if (ps == NULL) return;

    // 处理按键
    if (btn != BTN_NONE) {
        // 处理全局按键事件
        if (ps->global_btn_handler != NULL) {
            ps->global_btn_handler(btn);
            // 长按事件处理后，不再转发给页面
            if (btn == BTN_LONG_PRESS) {
                btn = BTN_NONE; // 清空按键，避免后续分发
            }
        }

        // 分发普通按键给当前页面
        if (btn != BTN_NONE) {
            page_t *current_page = page_stack_current(ps);
            if (current_page && current_page->comp && current_page->comp->input) {
                current_page->comp->input(btn, current_page->ctx);
            }
        }
    }

    // 更新tick刷新屏幕
    ps->main_tick++;
    page_t *p = page_stack_current(ps);
    if (p && p->comp && p->comp->draw) {
        u8g2_ClearBuffer(ps->u8g2);
        p->comp->draw(ps->u8g2, p->ctx);
        u8g2_SendBuffer(ps->u8g2);
    }
}