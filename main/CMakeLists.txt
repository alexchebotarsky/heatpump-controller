file(GLOB_RECURSE SOURCES "*.cpp")
idf_component_register(
  SRCS ${SOURCES}
  INCLUDE_DIRS "."
  PRIV_REQUIRES esp_wifi nvs_flash mqtt json
)
