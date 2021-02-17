# Cmake 最低版本号要求
cmake_minimum_required(VERSION 2.8)

# 项目信息
project(LinuxIPv6Demo)

# 指定生成的目标的目录
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 指定生成的目标
add_executable(IPv6ServerClientDemo.bin ipv6serverclient.cpp)

