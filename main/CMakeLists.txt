idf_component_register(
    SRCS "main.c" "board.c" "tach_model.c" "tach_ui.c" "tach_font_rpm.c"
         "tach_startup.c" "glastron_logo.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_lcd esp_timer esp_system lvgl)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
