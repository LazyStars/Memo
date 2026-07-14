cmake_minimum_required(VERSION 3.12)

project(memo  VERSION 1.0.3)

include(${EXTRAL_PKG_DIR}/MyDepsConfig.cmake)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 COMPONENTS Core Gui Widgets Network PrintSupport Sql REQUIRED)
find_package(QLogCollector REQUIRED COMPONENTS Server)
find_package(QWindowKit COMPONENTS Core Widgets REQUIRED)

#添加宏定义
add_compile_definitions(CUR_PROJECT_PATH="${CMAKE_CURRENT_SOURCE_DIR}")
add_compile_definitions(APP_VERSION="v${PROJECT_VERSION}")

include(${CMAKE_CURRENT_LIST_DIR}/generate_rc.cmake)
if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    generate_rc(RC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/res
            NAME Memo
            VERSION ${PROJECT_VERSION}  #产品版本
            COMPANY_NAME YeXiaobai    #公司名称
            FILE_DESCRIPTION Memo       #文件说明
            ICON_PATH D:/Memo/src/res/logo.ico
    )
    message("RC file generated at: ${RC_FILE}")
endif()

add_executable(${PROJECT_NAME} WIN32
        main.cpp

        app.cpp
        app.h
        app.ui
        app.qrc
        ${RC_FILE}

        dialogs/edittextdialog.h
        dialogs/edittextdialog.cpp

        database/memogroup.h
        database/memorecord.h
        database/memoreminder.h
        database/memodatabase.h
        database/memodatabase.cpp
        database/memorepository.h
        database/memorepository.cpp

        tools/tray/mysystemtrayicon.h
        tools/tray/mysystemtrayicon.cpp

        tools/scrollbar/silkyscrollbar.h
        tools/scrollbar/silkyscrollbar.cpp

        utils/treeview/memotreenode.h
        utils/treeview/memotreemodel.h
        utils/treeview/memotreemodel.cpp
        utils/treeview/memotreedelegate.h
        utils/treeview/memotreedelegate.cpp
        utils/treeview/memotreecontroller.h
        utils/treeview/memotreecontroller.cpp
        utils/treeview/item/recorditem.h
        utils/treeview/item/recorditem.cpp
        utils/treeview/item/groupitem.h
        utils/treeview/item/groupitem.cpp

        utils/menu/memomenu.h
        utils/menu/memomenu.cpp

        utils/reminder/memoremindercontroller.h
        utils/reminder/memoremindercontroller.cpp
)

target_link_libraries(${PROJECT_NAME}
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Network
        Qt5::PrintSupport
        Qt5::Sql

        QLogCollector::Server
        QWindowKit::Widgets
)
