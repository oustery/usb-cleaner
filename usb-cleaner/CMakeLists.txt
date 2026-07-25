cmake_minimum_required(VERSION 3.15)
project(USBCleaner VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Опции проекта
option(USE_NANA "Use Nana GUI library" ON)
option(BUILD_TESTS "Build test targets" OFF)
option(STATIC_LINKING "Link statically" ON)

# Поиск библиотеки Nana (если используется)
if(USE_NANA)
    # Загрузка внешнего проекта Nana
    include(FetchContent)
    
    FetchContent_Declare(
        nana
        GIT_REPOSITORY https://github.com/cnjinhao/nana.git
        GIT_TAG        v1.7.4
        SOURCE_DIR     ${CMAKE_BINARY_DIR}/nana
    )
    
    # Для Windows используем предустановленные бинарники
    if(WIN32)
        set(NANA_INSTALL_DIR "${CMAKE_SOURCE_DIR}/external/nana" CACHE PATH "Path to Nana installation")
        
        if(EXISTS "${NANA_INSTALL_DIR}/include")
            message(STATUS "Using local Nana installation: ${NANA_INSTALL_DIR}")
        else()
            message(WARNING "Nana not found at ${NANA_INSTALL_DIR}")
            message(STATUS "Please download Nana from: https://github.com/cnjinhao/nana/releases")
            message(STATUS "And extract to: ${NANA_INSTALL_DIR}")
        endif()
    endif()
endif()

# Исходные файлы
set(SOURCES
    src/main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

# Настройки компилятора
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /EHsc /DNDEBUG)
    if(STATIC_LINKING)
        set_target_properties(${PROJECT_NAME} PROPERTIES 
            LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
    endif()
elseif(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -O2)
endif()

# Включаемые директории
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

if(USE_NANA AND EXISTS "${NANA_INSTALL_DIR}/include")
    target_include_directories(${PROJECT_NAME} PRIVATE
        "${NANA_INSTALL_DIR}/include"
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE NANA_AUTOMATIC_GUI_TESTING)
endif()

# Библиотеки
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        comctl32
        shell32
        ole32
        uuid
        gdi32
        comdlg32
        setupapi
    )
    
    if(USE_NANA AND EXISTS "${NANA_INSTALL_DIR}/lib")
        target_link_libraries(${PROJECT_NAME} PRIVATE
            "${NANA_INSTALL_DIR}/lib/nana.lib"
        )
    endif()
    
    # GUI приложение (без консоли)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Установка
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Информация о версии
configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/version.h.in"
    "${CMAKE_BINARY_DIR}/generated/version.h"
)

message(STATUS "")
message(STATUS "========== USB Cleaner v${PROJECT_VERSION} ==========")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Nana GUI: ${USE_NANA}")
message(STATIC_LINKING "Static linking: ${STATIC_LINKING}")
message(STATUS "=====================================================")
message(STATUS "")
