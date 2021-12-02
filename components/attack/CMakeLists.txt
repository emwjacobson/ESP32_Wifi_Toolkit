FILE(GLOB_RECURSE app_sources src/*.*)

idf_component_register(
  SRCS ${app_sources}
  INCLUDE_DIRS "include"
)

target_link_libraries(${COMPONENT_LIB} -Wl,-zmuldefs)