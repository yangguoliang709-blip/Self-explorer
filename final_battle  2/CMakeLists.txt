cmake_minimum_required(VERSION 3.19)
project(final_battle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)  # 必须开启，自动处理qrc资源

# 尝试查找 Qt6 及其组件
find_package(Qt6 6.11 REQUIRED COMPONENTS Core Widgets)

# 尝试查找 Multimedia 模块，但不强制要求
find_package(Qt6 6.11 COMPONENTS Multimedia)

# 检查是否找到 Multimedia 模块
if(Qt6Multimedia_FOUND)
    message(STATUS "Qt6 Multimedia found, enabling audio support")
else()
    message(STATUS "Qt6 Multimedia not found, audio support will be disabled")
endif()

qt_add_executable(final_battle
    WIN32 MACOSX_BUNDLE
    main.cpp
    widget.cpp
    widget.h
    resources.qrc  # 资源文件必须在这里
)

target_link_libraries(final_battle PRIVATE
    Qt6::Core
    Qt6::Widgets
)

# 仅在找到 Multimedia 模块时链接
if(Qt6Multimedia_FOUND)
    target_link_libraries(final_battle PRIVATE Qt6::Multimedia)
    # 传递 Qt6Multimedia_FOUND 宏给 C++ 编译器
    target_compile_definitions(final_battle PRIVATE Qt6Multimedia_FOUND)
endif()

# 部署脚本（可选，不影响编译）
qt_generate_deploy_app_script(
    TARGET final_battle
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(TARGETS final_battle
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
