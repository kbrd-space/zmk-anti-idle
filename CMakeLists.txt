if ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources_ifdef(CONFIG_ZMK_ANTI_IDLE app PRIVATE src/anti_idle.c)

  zephyr_include_directories(include)
endif()
