cmake_minimum_required(VERSION 3.15)
project(USBCleaner VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Исходные файлы
add_executable(usb_cleaner
    src/main.cpp
)

# Библиотеки Windows
target_link_libraries(usb_cleaner
    PRIVATE
    comctl32
    shell32
    ole32
    uuid
)

# Для GUI-приложения (без консольного окна)
if(WIN32)
    set_target_properties(usb_cleaner PROPERTIES
        WIN32_EXECUTABLE TRUE
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    )
endif()

# Оптимизация Release сборки
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(usb_cleaner PRIVATE /O2)
endif()
