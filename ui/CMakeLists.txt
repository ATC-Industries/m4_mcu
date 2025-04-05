SET(SOURCES screens/ui_ScreenMain.c
    screens/ui_ScreenSettings.c
    ui_theme_manager.cpp
    ui_themes.cpp
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    ui_events.cpp
    images/ui_img_iconsettings44px_png.c
    images/ui_img_iconchangedriver44px_png.c
    fonts/ui_font_BIO_BOLDITALIC_10.c
    fonts/ui_font_BIO_BOLDITALIC_6.c
    fonts/ui_font_BIO_BOLDITALIC_8.c
    fonts/ui_font_BIO_EXTRABOLD_19.c
    fonts/ui_font_BIO_SEMIBOLD_6.c
    fonts/ui_font_BIO_SEMIBOLD_7.c
    fonts/ui_font_HH13.c
    fonts/ui_font_HH24.c
    fonts/ui_font_HH40.c
    fonts/ui_font_HH8.c)

add_library(ui ${SOURCES})
