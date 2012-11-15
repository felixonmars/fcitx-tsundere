set(PO_LANGS zh_CN)
foreach(lang ${PO_LANGS})
  fcitx_translate_add_po_file("${lang}" "${lang}.po")
endforeach()
fcitx_translate_set_pot_target(pot fcitx-tsundere fcitx-tsundere.pot
  BUGADDR felixonmars@gmail.com)
