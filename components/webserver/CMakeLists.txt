FILE(GLOB_RECURSE app_sources src/*.*)
FILE(GLOB_RECURSE web_serve srv/*.*)

idf_component_register(
  SRCS ${app_sources}
  INCLUDE_DIRS "include"
  EMBED_FILES ${web_serve}
  REQUIRES esp_http_server json softap attack
)