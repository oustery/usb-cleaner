# USB Cleaner v2.0 - Makefile
# Сборка для Windows (MinGW-w64) и Linux
# 
# Использование:
#   make          - Сборка Release версии
#   make debug    - Сборка Debug версии
#   make clean    - Очистка
#   make install  - Установка
#   make dist     - Создание дистрибутива
#   make help     - Показать доступные цели

# ==================== КОНФИГУРАЦИЯ ====================

CXX ?= g++
TARGET = usb_cleaner.exe
SRC = src/main.cpp
OBJ_DIR = build
OBJ = $(OBJ_DIR)/main.o

# Версия проекта
VERSION_MAJOR = 2
VERSION_MINOR = 0
VERSION_PATCH = 0
VERSION = $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)

# Пути к Nana GUI
NANA_DIR = external/nana
NANA_INCLUDE = $(NANA_DIR)/include
NANA_LIB = $(NANA_DIR)/lib

# Флаги компиляции
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -DNDEBUG
CXXFLAGS_DEBUG = -std=c++17 -Wall -Wextra -g -DDEBUG

# Флаги линковки
LDFLAGS = -lcomctl32 -lshell32 -lole32 -luuid -lgdi32 -lcomdlg32 -lsetupapi
LDFLAGS_STATIC = -mwindows -static
LDFLAGS_DEBUG = -mwindows

# Определения препроцессора
DEFINES = -DAPP_VERSION=\"$(VERSION)\" -DAPP_NAME=\"USB Cleaner\"
DEFINES_DEBUG = $(DEFINES) -DDEBUG

# ==================== ЦЕЛИ СБОРКИ ====================

.PHONY: all clean debug install run dist help check-deps download-nana

all: check-deps $(TARGET)
	@echo ""
	@echo "✅ Сборка завершена: $(TARGET)"
	@echo "   Версия: $(VERSION)"
	@echo "   Запустите: ./$(TARGET)"

debug: CXXFLAGS = $(CXXFLAGS_DEBUG)
debug: LDFLAGS += $(LDFLAGS_DEBUG)
debug: DEFINES = $(DEFINES_DEBUG)
debug: clean check-deps $(TARGET)
	@echo "🔧 Debug сборка завершена"

$(TARGET): $(SRC)
	@echo "📦 Компиляция..."
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(DEFINES) \
		-I$(NANA_INCLUDE) \
		-DNANA_AUTOMATIC_GUI_TESTING \
		-o $@ $< $(LDFLAGS) $(LDFLAGS_STATIC)
	@echo "✓ Компиляция успешна"

# ==================== ВСПОМОГАТЕЛЬНЫЕ ЦЕЛИ ====================

clean:
	@echo "🧹 Очистка..."
	rm -f $(TARGET)
	rm -rf $(OBJ_DIR)
	rm -f *.log
	rm -rf release/*
	@echo "✓ Очистка завершена"

run: $(TARGET)
	@echo "🚀 Запуск программы..."
	./$(TARGET)

install: $(TARGET)
	@echo "📥 Установка..."
ifeq ($(OS),Windows_NT)
	mkdir -p "$(ProgramFiles)/USB Cleaner"
	cp $(TARGET) "$(ProgramFiles)/USB Cleaner/"
	cp README.md "$(ProgramFiles)/USB Cleaner/"
	@echo "✓ Установлено в Program Files"
else
	mkdir -p /usr/local/bin
	mkdir -p /usr/local/share/doc/usb-cleaner
	cp $(TARGET) /usr/local/bin/
	cp README.md CHANGELOG.md LICENSE /usr/local/share/doc/usb-cleaner/
	@echo "✓ Установлено в /usr/local"
endif

dist: clean $(TARGET)
	@echo "📦 Создание дистрибутива..."
	mkdir -p release/usb-cleaner-$(VERSION)-windows
	cp $(TARGET) release/usb-cleaner-$(VERSION)-windows/
	cp README.md CHANGELOG.md LICENSE CONTRIBUTING.md setup.bat release/usb-cleaner-$(VERSION)-windows/
	cd release && zip -r usb-cleaner-$(VERSION)-windows.zip usb-cleaner-$(VERSION)-windows/
	@echo "✓ Дистрибутив создан: release/usb-cleaner-$(VERSION)-windows.zip"

check-deps:
ifndef NANA_SKIP_CHECK
	@if [ ! -d "$(NANA_INCLUDE)" ]; then \
		echo ""; \
		echo "⚠️  Nana GUI library not found!"; \
		echo ""; \
		echo "Please run setup.bat or:"; \
		echo "  1. Download: https://github.com/cnjinhao/nana/releases/download/v1.7.4/nana-1.7.4.zip"; \
		echo "  2. Extract to: $(NANA_DIR)/"; \
		echo ""; \
		echo "Or skip this check with: make NANA_SKIP_CHECK=1"; \
		echo ""; \
		exit 1; \
	fi
endif

download-nana:
	@echo "⬇️  Downloading Nana GUI library..."
	@mkdir -p $(NANA_DIR)
	@cd $(NANA_DIR) && \
	if [ ! -f "nana-1.7.4.zip" ]; then \
		wget -q --show-progress https://github.com/cnjinhao/nana/releases/download/v1.7.4/nana-1.7.4.zip -O nana.zip; \
	fi && \
	unzip -qo nana.zip && \
	mv nana/* . 2>/dev/null || true && \
	rm -rf nana nana.zip 2>/dev/null || true
	@echo "✓ Nana downloaded and extracted"

setup: download-nana all
	@echo "✅ Setup complete!"

help:
	@echo ""
	@echo "╔═══════════════════════════════════════════════════╗"
	@echo "║       USB Cleaner v$(VERSION) - Makefile Help        ║"
	@echo "╠═══════════════════════════════════════════════════╣"
	@echo "║                                                   ║"
	@echo "║  Основные цели:                                   ║"
	@echo "║    make              Сборка Release версии        ║"
	@echo "║    make debug         Сборка Debug версии          ║"
	@echo "║    make clean         Очистка файлов сборки        ║"
	@echo "║    make install       Установка программы          ║"
	@echo "║    make run           Сборка и запуск              ║"
	@echo "║    make dist          Создание дистрибутива (.zip) ║"
	@echo "║                                                   ║"
	@echo "║  Зависимости:                                     ║"
	@echo "║    make download-nana Загрузить Nana библиотеку    ║"
	@echo "║    make setup         Полная установка             ║"
	@echo "║    make check-deps    Проверка зависимостей        ║"
	@echo "║                                                   ║"
	@echo "║  Переменные:                                      ║"
	@echo "║    CXX=...          Компилятор (по умолчанию g++) ║"
	@echo "║    NANA_SKIP_CHECK=1 Пропустить проверку Nana     ║"
	@echo "║                                                   ║"
	@echo "╚═══════════════════════════════════════════════════╝"
	@echo ""

# ==================== ДЕБАГ ИНФОРМАЦИЯ ====================

info:
	@echo "Информация о сборке:"
	@echo "  Компилятор:    $(CXX)"
	@echo "  Версия:        $(VERSION)"
	@echo "  Цель:          $(TARGET)"
	@echo "  Исходники:     $(SRC)"
	@echo "  Nana Include:  $(NANA_INCLUDE)"
	@echo "  CXXFLAGS:      $(CXXFLAGS)"
	@echo "  LDFLAGS:       $(LDFLAGS)"

# ==================== ТЕСТЫ (заглушки для будущего) ====================

test: $(TARGET)
	@echo "🧪 Запуск тестов..."
	@echo "⚠️  Автоматические тесты пока не реализованы"
	@echo "   Проверьте работу вручную: make run"

lint:
	@echo "🔍 Проверка кода..."
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all --std=c++17 src/main.cpp; \
	else \
		echo "cppcheck не установлен. Установите: apt install cppcheck"; \
	fi

format:
	@echo "🎨 Форматирование кода..."
	@if command -v clang-format >/dev/null 2>&1; then \
		clang-format -i src/main.cpp; \
		echo "✓ Код отформатирован"; \
	else \
		echo "clang-format не установлен"; \
	fi
