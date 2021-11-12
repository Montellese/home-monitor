cmake_minimum_required(VERSION 3.10)

project(home-monitor)

# find external dependencies
find_package(Poco REQUIRED Foundation Util XML)
find_package(Threads REQUIRED)
pkg_search_module(libcrafter REQUIRED IMPORTED_TARGET libcrafter crafter)
pkg_search_module(liblog4cxx REQUIRED IMPORTED_TARGET liblog4cxx log4cxx)
pkg_search_module(libpcap REQUIRED IMPORTED_TARGET libpcap pcap)
pkg_search_module(libssh REQUIRED IMPORTED_TARGET libssh ssh)

# collect sources
set(LIB_SRC Configuration.cpp
            Networking.cpp)
set(LIB_HDR Configuration.h
            Machine.h
            Networking.h)
set(MAIN_SRC main.cpp)

# internal library target
add_library(${PROJECT_NAME}_lib STATIC ${LIB_SRC})
target_link_libraries(${PROJECT_NAME}_lib
    PRIVATE
      PkgConfig::libcrafter
      PkgConfig::liblog4cxx
      PkgConfig::libpcap
      PkgConfig::libssh
      Poco::Foundation
      Poco::Util
      Poco::XML
      Threads::Threads
)

# main app target
add_executable(${PROJECT_NAME} ${MAIN_SRC})
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_lib)

# set compiler flags
target_compile_options(${PROJECT_NAME}_lib PRIVATE ${cxxflags})
target_compile_options(${PROJECT_NAME} PRIVATE ${cxxflags})

# install steps
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION /usr/bin/
)
install(FILES ../home-monitor.service
    DESTINATION /etc/systemd/system/
)
install(FILES ../home-monitor.xml.example
    DESTINATION /etc/home-monitor/
)