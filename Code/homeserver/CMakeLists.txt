cmake_minimum_required(VERSION 3.16)

project(homeserver VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64")

find_package(Qt6 REQUIRED COMPONENTS Quick HttpServer Sql)
find_package(OpenSSL REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Quick HttpServer)

qt_standard_project_setup(REQUIRES 6.10)

# Content of downloaded design from Figma to Qt or similar
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/importedcontent/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/importedcontent)
endif()


qt_add_executable(apphomeserver
    main.cpp
    database.h
    database.cpp
)

qt_add_qml_module(apphomeserver
    URI homeserver
    QML_FILES
        Main.qml
        RESOURCES TEST.md
        SOURCES passwordhasher.h passwordhasher.cpp
        SOURCES demoUser.h
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(apphomeserver PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.apphomeserver
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(apphomeserver
    PRIVATE Qt6::Quick
    PRIVATE Qt6::HttpServer
    PRIVATE Qt6::Sql
    PRIVATE OpenSSL::Crypto
)

install(TARGETS apphomeserver
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
