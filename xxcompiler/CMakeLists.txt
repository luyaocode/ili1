cmake_minimum_required(VERSION 3.10)  # 最低CMake版本要求

project(xxcompiler  # 项目名称
    LANGUAGES C     # 使用C语言
)

# 设置C标准（C99及以上即可）
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 添加可执行目标：将main.c编译为calculator程序
add_executable(${PROJECT_NAME} main.c)

# 可选：设置编译选项（如警告等级）
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror)
endif()
