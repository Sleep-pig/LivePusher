# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "sources/ui/CMakeFiles/widget_autogen.dir/AutogenUsed.txt"
  "sources/ui/CMakeFiles/widget_autogen.dir/ParseCache.txt"
  "sources/ui/widget_autogen"
  )
endif()
