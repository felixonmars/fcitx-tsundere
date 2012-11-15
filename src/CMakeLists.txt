include_directories(${PROJECT_SOURCE_DIR}/src)

set(FCITX_TSUNDERE_SOURCES
  tsundere.c
  )
fcitx_add_addon_full(tsundere DESC
  HEADERS tsundere.h
  SOURCES ${FCITX_TSUNDERE_SOURCES})
