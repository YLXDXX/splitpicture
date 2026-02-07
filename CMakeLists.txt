cmake_minimum_required(VERSION 3.15)

project(splitpicture VERSION 0.1 LANGUAGES CXX DESCRIPTION "Split Picture")

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(QT_VERSION_MAJOR 6)
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent )
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Concurrent)

# find_package(OpenCV REQUIRED) #不需要包含整个为 OpenCV，按需取用
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)

# 检查是否找到 OpenCV
if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV not found")
else()
    message(STATUS "OpenCV found: ${OpenCV_VERSION}")
    message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
endif()


# 安装路径
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    # 仅当用户未指定时修改默认值
    if(UNIX AND NOT APPLE)
        set(CMAKE_INSTALL_PREFIX "~/.local" CACHE PATH "..." FORCE)
    elseif(APPLE)
        set(CMAKE_INSTALL_PREFIX "~/Applications/${PROJECT_NAME}" CACHE PATH "..." FORCE)
    elseif(WIN32)
        set(CMAKE_INSTALL_PREFIX "C:/Program Files/${PROJECT_NAME}" CACHE PATH "..." FORCE)
    endif()
endif()




# 引入GNU安装目录标准
include(GNUInstallDirs)

# 设置项目特定的目录结构
set(PROJECT_ICON_NAME icons)  # 图标文件的基本名称
set(PROJECT_DESKTOP_FILE ${PROJECT_NAME}.desktop)



include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${OpenCV_INCLUDE_DIRS})

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
)

qt_add_executable(splitpicture
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)


# 目标属性
set_target_properties(splitpicture PROPERTIES
    OUTPUT_NAME splitpicture
)

target_link_libraries(splitpicture PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Concurrent
    ${OpenCV_LIBS})


install(TARGETS splitpicture
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# 安装桌面文件
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_DESKTOP_FILE}
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

# 简化图标安装（使用循环）
set(ICON_SIZES 16x16 32x32 48x48 64x64 72x72 96x96 128x128 256x256)

foreach(SIZE ${ICON_SIZES})
    # 检查图标文件是否存在
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/icons/hicolor/${SIZE}/${PROJECT_NAME}.png)
        install(FILES icons/hicolor/${SIZE}/${PROJECT_NAME}.png
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/${SIZE}/apps
            RENAME ${PROJECT_NAME}.png
        )
    endif()
endforeach()

# 安装SVG矢量图标
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/icons/hicolor/scalable/${PROJECT_NAME}.svg
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
    RENAME ${PROJECT_NAME}.svg
)

# ========== 安装后的处理脚本 ==========
# 更新图标缓存（gtk-update-icon-cache）
install(CODE "
    message(STATUS \"Updating icon cache...\")
    execute_process(
        COMMAND gtk-update-icon-cache -f -t ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor
    )
")

# 更新桌面数据库（update-desktop-database）
install(CODE "
    message(STATUS \"Updating desktop database...\")
    execute_process(
        COMMAND update-desktop-database ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/applications
    )
")



if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(splitpicture)
endif()

