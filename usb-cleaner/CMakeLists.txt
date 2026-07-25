cmake_minimum_required(VERSION 3.15)
project(USBCleaner VERSION 2.0.0 LANGUAGES CXX DESCRIPTION "USB Cleaner - Modern USB device history and flash label cleaner")

# ==================== ОПЦИИ СБОРКИ ====================

option(USE_NANA "Use Nana GUI library" ON)
option(BUILD_TESTS "Build test targets" OFF)
option(STATIC_LINKING "Link statically for portable builds" ON)
option(BUILD_PORTABLE "Create portable build without external dependencies" ON)

# ==================== НАСТРОЙКИ КОМПИЛЯТОРА ====================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Экспорт команд компиляции
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Настройки для разных компиляторов
if(MSVC)
    # MSVC настройки
    add_compile_options(
        /W4           # Максимальный уровень предупреждений
        /WX-          # Предупреждения не как ошибки (можно включить)
        /EHsc         # Exception handling
        /utf-8        # UTF-8 source encoding
        /MP           # Multi-processor compilation
    )
    
    # Debug/Release флаги
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi /D_DEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG /GL")
    
elseif(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    # GCC/Clang настройки
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
    )
    
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG -flto")
endif()

# ==================== БИБЛИОТЕКА NANA GUI ====================

if(USE_NAMA)
    message(STATUS "")
    message(STATUS "🔍 Searching for Nana GUI library...")
    
    # Поиск в стандартных locations
    find_path(NANA_INCLUDE_DIR 
        NAMES nana/gui.hpp
        PATHS 
            ${CMAKE_SOURCE_DIR}/external/nana/include
            ${CMAKE_INSTALL_PREFIX}/include
            /usr/include
            /usr/local/include
            $ENV{NANA_ROOT}/include
        DOC "Nana GUI include directory"
    )
    
    find_library(NANA_LIBRARY
        NAMES nana libnana
        PATHS
            ${CMAKE_SOURCE_DIR}/external/nana/lib
            ${CMAKE_INSTALL_PREFIX}/lib
            /usr/lib
            /usr/local/lib
            $ENV{NANA_ROOT}/lib
        DOC "Nana GUI library file"
    )
    
    if(NANA_INCLUDE_DIR AND NANA_LIBRARY)
        message(STATUS "✅ Nana found:")
        message(STATUS "   Include: ${NANA_INCLUDE_DIR}")
        message(STATUS "   Library: ${NANA_LIBRARY}")
        
        add_compile_definitions(NANA_AUTOMATIC_GUI_TESTING)
    else()
        message(WARNING "⚠️  Nana not found!")
        message(STATUS "   Please run setup.bat or download manually:")
        message(STATUS "   https://github.com/cnjinhao/nana/releases/download/v1.7.4/nana-1.7.4.zip")
        message(STATUS "   Extract to: ${CMAKE_SOURCE_DIR}/external/nana/")
        
        if(USE_NANA)
            message(FATAL_ERROR "Nana is required but not found. Set USE_NANA=OFF or install Nana.")
        endif()
    endif()
endif()

# ==================== ИСХОДНЫЕ ФАЙЛЫ ====================

set(SOURCES
    src/main.cpp
)

# Заголовочные файлы (для IDE)
set(HEADERS
    src/main.cpp  # Single-file architecture currently
)

# ==================== ЦЕЛЕВЫЕ ИСПОЛНЯЕМЫЙ ФАЙЛ ====================

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Включаемые директории
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${NANA_INCLUDE_DIR}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

# Библиотеки Windows
if(WIN32)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            comctl32       # Common controls
            shell32        # Shell API
            ole32          # OLE/COM
            uuid           # GUID generation
            gdi32          # Graphics
            comdlg32       # Common dialogs
            setupapi       # Device setup API
            ${NANA_LIBRARY}
    )
endif()

# Определения препроцессора
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        $<$<CONFIG:Debug>:DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
        APP_VERSION="${PROJECT_VERSION}"
        APP_NAME="${PROJECT_NAME}"
)

# Настройки для Windows
if(WIN32)
    # GUI приложение без консольного окна
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
        OUTPUT_NAME "${PROJECT_NAME}"
        PREFIX ""          # Без 'lib' префикса на Windows
    )
    
    # Статическая линковка runtime для portable сборок
    if(STATIC_LINKING OR BUILD_PORTABLE)
        if(MSVC)
            set_target_properties(${PROJECT_NAME} PROPERTIES
                LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTSTATIC"
                MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
            )
        else()
            target_link_options(${PROJECT_NAME} PRIVATE -static)
        endif()
    endif()
    
    # Внедрение манифеста (требование прав администратора при необходимости)
    if(BUILD_PORTABLE)
        # Можно добавить манифест здесь
    endif()
endif()

# Установка файлов
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Установка дополнительных файлов
install(FILES README.md CHANGELOG.md LICENSE CONTRIBUTING.md
    DESTINATION share/doc/${PROJECT_NAME}
)

# Генерация конфигурационного файла версии
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in"
    "${CMAKE_BINARY_DIR}/generated/version.h"
    @ONLY
)

# ==================== ТЕСТЫ (опционально) ====================

if(BUILD_TESTS)
    enable_testing()
    
    # Здесь можно добавить тесты
    # add_subdirectory(tests)
endif()

# ==================== УПАКОВКА (CPack) ====================

set(CPACK_PACKAGE_VENDOR "USB Cleaner Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern USB device history and flash label cleaner")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
setCPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "USB Cleaner")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "USB Cleaner")
    set(CPACK_NSIS_PACKAGE_NAME "USB Cleaner")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "usb_cleaner.exe")
    set(CPACK_NSIS_MODIFY_PATH ON)
endif()

include(CPack)

# ==================== ИНФОРМАЦИЯ О СБОРКЕ ====================

message(STATUS "")
message(STATUS "═══════════════════════════════════════════════")
message(STATUS "  USB Cleaner v${PROJECT_VERSION} Configuration")
message(STATUS "═══════════════════════════════════════════════")
message(STATUS "")
message(Status "  Build Type:      ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler:        ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  System:          ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
message(STATUS "  Install Prefix:  ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "  Options:")
message(STATUS "    Use Nana:          ${USE_NANA}")
message(STATUS "    Build Tests:       ${BUILD_TESTS}")
message(STATUS "    Static Linking:    ${STATIC_LINKING}")
message(STATUS "    Portable Build:    ${BUILD_PORTABLE}")
message(STATUS "")
message(STATUS "  Libraries:")
message(STATUS "    Nana Include:     ${NANA_INCLUDE_DIR}")
message(STATUS "    Nana Library:     ${NANA_LIBRARY}")
message(STATUS "")
message(STATUS "═══════════════════════════════════════════════")
message(STATUS "")

# Проверка критических зависимостей
if(USE_NANA AND NOT NANA_INCLUDE_DIR)
    message(FATAL_ERROR "
╔══════════════════════════════════════════════════════╗
║  ⚠️  CRITICAL ERROR: Nana GUI library not found!    ║
╠══════════════════════════════════════════════════════╣
║                                                      ║
║  To fix this issue:                                  ║
║  1. Run setup.bat (Windows)                          ║
║  2. Or download Nana manually:                       ║
║     https://github.com/cnjinhao/nana/releases        ║
║  3. Extract to: external/nana/                       ║
║                                                      ║
║  Or disable Nana with: -DUSE_NANA=OFF               ║
╚══════════════════════════════════════════════════════╝
")
endif()
