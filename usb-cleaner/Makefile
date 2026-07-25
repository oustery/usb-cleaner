# USB Cleaner - Makefile
# Сборка для Windows (MinGW-w64)

CXX = g++
TARGET = usb_cleaner.exe
SRC = src/main.cpp
OBJ = build/main.o

# Флаги компиляции
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS = -lcomctl32 -lshell32 -lole32 -luuid -mwindows -static

.PHONY: all clean install run debug

all: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)
	@echo "Сборка завершена: $(TARGET)"

debug: CXXFLAGS = -std=c++17 -Wall -Wextra -g -DDEBUG
debug: clean $(TARGET)
	@echo "Debug сборка завершена"

clean:
	rm -f $(TARGET) *.log build/*.o

run: $(TARGET)
	./$(TARGET)

# Установка в системную папку (требует прав администратора)
install: $(TARGET)
	copy $(TARGET) "%ProgramFiles%\\USB Cleaner\\"
	@echo "Установлено в Program Files"

# Создание дистрибутива
dist: $(TARGET)
	@mkdir -p dist
	copy $(TARGET) dist\\
	copy README.md dist\\
	@echo "Дистрибутив создан в папке dist/"
