cmake_minimum_required(VERSION 3.16)

project(Q3_Client VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Network Sql)

qt_standard_project_setup(REQUIRES 6.10)

# Content of downloaded design from Figma to Qt or similar
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/importedcontent/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/importedcontent)
endif()

qt_add_executable(appQ3_Client
    main.cpp
)

qt_add_qml_module(appQ3_Client
    URI Q3_Client
    QML_FILES
        Main.qml
        SOURCES clientservice.h clientservice.cpp
        SOURCES clientdatabase.h clientdatabase.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appQ3_Client PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appQ3_Client
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appQ3_Client
    PRIVATE Qt6::Quick # QML/UI
    PRIVATE Qt6::Network # lets the client send HTTP Requests
    PRIVATE Qt6::Sql # local SQLite message storage
)

# Unit tests for the local database: cmake -DQ3_CLIENT_BUILD_TESTS=ON, then ctest
option(Q3_CLIENT_BUILD_TESTS "Build the client database unit tests" OFF)
if (Q3_CLIENT_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(tst_clientdatabase
        tests/tst_clientdatabase.cpp
        clientdatabase.h clientdatabase.cpp
    )
    target_include_directories(tst_clientdatabase PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(tst_clientdatabase PRIVATE Qt6::Sql Qt6::Test)
    add_test(NAME tst_clientdatabase COMMAND tst_clientdatabase)
endif()

install(TARGETS appQ3_Client
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
