include_directories(${PROJECT_SOURCE_DIR}/src)

set(FCITX_TSUNDERE_SOURCES
    tsundere.c
)

fcitx_add_addon(fcitx-tsundere ${FCITX_TSUNDERE_SOURCES})

fcitx_add_configdesc_file(fcitx-tsundere.desc)
fcitx_add_addon_conf_file(fcitx-tsundere.conf)
fcitx_add_addon_header(tsundere)
