cmake_minimum_required(VERSION 3.29)
project(nfq_forwarder)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

# optimization
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -flto -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")


find_package(PkgConfig REQUIRED)
pkg_check_modules(MAXMINDDB REQUIRED libmaxminddb)
pkg_check_modules(NFQ REQUIRED libnetfilter_queue)
pkg_check_modules(NFNETLINK REQUIRED libnfnetlink)
pkg_check_modules(NFTNL REQUIRED libnftnl)

add_executable(nfq_forwarder main.cpp)

target_include_directories(nfq_forwarder PRIVATE
        include
        ${NFQ_INCLUDE_DIRS}
        ${NFNETLINK_INCLUDE_DIRS}
        ${NFTNL_INCLUDE_DIRS}
        ${MAXMINDDB_INCLUDE_DIRS}
)

target_link_libraries(nfq_forwarder PRIVATE
        ${NFQ_LIBRARIES}
        ${NFNETLINK_LIBRARIES}
        ${NFTNL_LIBRARIES}
        ${MAXMINDDB_LIBRARIES}
)

target_compile_options(nfq_forwarder PRIVATE ${MAXMINDDB_CFLAGS_OTHER})

add_subdirectory(test)
