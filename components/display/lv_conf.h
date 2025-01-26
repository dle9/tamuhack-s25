#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH     16
#define LV_HOR_RES_MAX     240
#define LV_VER_RES_MAX     320

#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

// Memory configuration
#define LV_MEM_CUSTOM      1
#define LV_MEM_SIZE        (32U * 1024U)  // 32KB of memory for LVGL

// Recommended settings
#define LV_TICK_CUSTOM     1
#define LV_USE_DEFAULT_THEME 1
#define LV_THEME_DEFAULT_DARK 1

// Use the following to modify how LVGL ticks
#define LV_TICK_CUSTOM_SYS_TIME_INTERVAL 1

#endif /* LV_CONF_H */