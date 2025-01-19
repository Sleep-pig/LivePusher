file(GLOB_RECURSE srcs CMAKE_CONFIGURE_DEPENDS include/*.hpp core/*.cpp)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Multimedia  MultimediaWidgets REQUIRED)


set(ffmpeg_libs_DIR /usr/lib/ffmpeg5.1.6)
set(ffmpeg_headers_DIR /usr/include/ffmpeg5.1.6)
include_directories(${ffmpeg_headers_DIR})
link_directories(${ffmpeg_libs_DIR})

add_executable(widget  ${srcs})
#add_library(logger STATIC ${srcs})

target_link_libraries(widget PRIVATE Qt5::Core Qt5::Widgets Qt5::Multimedia Qt5::MultimediaWidgets)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}) 
target_include_directories(widget PUBLIC include)


target_link_libraries(widget PRIVATE
    avcodec
    avformat
    avutil
    avdevice
    avfilter
    swscale
    swresample
    postproc
)