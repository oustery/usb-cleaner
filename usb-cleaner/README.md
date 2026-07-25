# 🔌 USB Cleaner v2.0

[![Build Status](https://github.com/super-cow/usb-cleaner/actions/workflows/build.yml/badge.svg)](https://github.com/super-cow/usb-cleaner/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-blue.svg)](https://github.com/super-cow/usb-cleaner)

**Современное приложение для очистки истории USB-устройств и меток безопасности на флешках.**

Построено на **[Nana C++ GUI](https://github.com/cnjinhao/nana)** — современной, лёгкой библиотеке для создания красивых кроссплатформенных интерфейсов.

---

## ✨ Возможности

### 🔌 Очистка истории USB-устройств
- ✅ Удаляет записи об **отключённых** устройствах из реестра Windows
- ✅ **Сохраняет** информацию о текущих подключениях
- ✅ Очищает ключи:
  - `HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Enum\USBSTOR`
  - `HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Enum\USB`
  - `HKEY_LOCAL_MACHINE\SYSTEM\MountedDevices`

### 💾 Очистка меток флешек (Zone.Identifier)
- ✅ Удаляет **альтернативные потоки NTFS** (Zone.Identifier)
- ✅ Убирает метки "Эта программа загружена из Интернета"
- ✅ Рекурсивная обработка всех папок и подпапок
- ✅ Поддержка нескольких съёмных дисков одновременно

### 🎨 Современный интерфейс
- ✅ Красивый GUI на **Nana C++ GUI**
- ✅ Прогресс-бары в реальном времени
- ✅ Цветовая индикация статуса операций
- ✅ Всплывающие подсказки (tooltips)
- ✅ Адаптивный дизайн

## 📸 Скриншоты

```
┌─────────────────────────────────────────────┐
│  🔌 USB Cleaner v2.0                    [_][×]│
│                                             │
│  ── 🔌 Очистка истории USB-устройств ──     │
│  [═════════════════════════════] 100%       │
│           [ 🧹 Очистить историю USB ]        │
│                                             │
│  ── 💾 Очистка меток флешек ──              │
│  [═════════════════════════════]   0%       │
│        [ 🗑️ Очистить метки Zone.Identifier ] │
│                                             │
│  ✓ Готов к работе                           │
│                    [ℹ️ О программе] [✖️ Выход] │
└─────────────────────────────────────────────┘
```

## 🚀 Быстрый старт

### Требования
- **Windows 7 / 8 / 10 / 11** (64-bit)
- **Права администратора** (обязательно!)
- **Компилятор**: MinGW-w64 или MSVC 2019+

### Сборка из исходников

#### 1. Клонирование репозитория
```bash
git clone https://github.com/super-cow/usb-cleaner.git
cd usb-cleaner
git checkout dev  # Для разработки
```

#### 2. Загрузка Nana GUI
```bash
# Windows (PowerShell)
mkdir external\nana
cd external\nana
Invoke-WebRequest -Uri "https://github.com/cnjinhao/nana/releases/download/v1.7.4/nana-1.7.4.zip" -OutFile "nana.zip"
Expand-Archive -Path "nana.zip" -DestinationPath "."
```

#### 3. Сборка с CMake
```bash
# Создание директории сборки
mkdir build && cd build

# Конфигурация (MinGW)
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

# Или конфигурация (MSVC)
cmake .. -G "Visual Studio 17 2022" -A x64

# Сборка
cmake --build . --config Release --parallel
```

#### 4. Быстрая сборка (MinGW)
```bash
g++ -std=c++17 -o usb_cleaner.exe src/main.cpp \
    -I./external/nana/include \
    -DNANA_AUTOMATIC_GUI_TESTING \
    -lcomctl32 -lshell32 -lole32 -luuid -lgdi32 \
    -lcomdlg32 -lsetupapi \
    -mwindows -static -DNDEBUG -O2
```

## 📁 Структура проекта

```
usb-cleaner/
├── src/
│   └── main.cpp              # Основной код приложения (~700 строк)
├── external/
│   └── nana/                 # Библиотека Nana GUI (загружается отдельно)
├── .github/
│   └── workflows/
│       └── build.yml         # GitHub Actions CI/CD
├── cmake/
│   └── version.h.in          # Шаблон версии
├── CMakeLists.txt            # CMake конфигурация
├── Makefile                  # Альтернативная сборка (MinGW)
├── README.md                 # Документация
├── LICENSE                   # MIT License
└── .gitignore                # Правила Git
```

## 🔄 Ветки репозитория

| Ветка | Назначение |
|-------|------------|
| `master` | Стабильная версия, готовая к релизу |
| `dev` | Разработка новых функций |

## 🔄 GitHub Actions (CI/CD)

Проект включает автоматизированные сборки:

### Автоматически при push:
- ✅ **Windows MinGW-w64** — сборка с GCC
- ✅ **Windows MSVC** — сборка с Visual Studio 2022
- ✅ **Linux** — проверка компиляции (опционально)

### При создании тега (`v*`):
- ✅ **Создание релиза** на GitHub
- ✅ **Публикация артефактов** (.zip файлы)
- ✅ **Генерация changelog** из коммитов

### Пример создания релиза:
```bash
# Создание тега релиза
git tag v2.0.0
git push origin v2.0.0

# Или beta-версия
git tag v2.0.0-beta.1
git push origin v2.0.0-beta.1
```

## 🛠️ Использование

1. **Запустите программу от имени администратора**
   - Правый клик → "Запуск от имени администратора"

2. **Выберите действие:**
   - 🧹 **Очистить историю USB** — удаляет записи об отключённых устройствах
   - 🗑️ **Очистить метки флешек** — удаляет Zone.Identifier со всех файлов

3. **Следите за прогрессом** в реальном времени

4. **Просмотрите результаты** во всплывающем диалоге

## ⚙️ Технические детали

### Архитектура
```
┌─────────────────────────────────────┐
│          Nana C++ GUI Layer         │
│  (Формы, кнопки, прогресс-бары)     │
├─────────────────────────────────────┤
│         Business Logic Layer        │
│  (CleanUSBHistory, CleanFlashLabels)│
├─────────────────────────────────────┤
│        Windows API Layer            │
│  (Registry, SetupAPI, NTFS ADS)     │
└─────────────────────────────────────┘
```

### Ключевые технологии
- **Nana C++ GUI** — современный UI фреймворк
- **Windows Registry API** — работа с реестром
- **SetupAPI** — обнаружение USB устройств
- **NTFS Alternate Data Streams** — Zone.Identifier

## 🐛 Troubleshooting

| Проблема | Решение |
|----------|---------|
| "Требуются права администратора" | Запустите от имени администратора |
| Ошибка компиляции Nana | Проверьте путь `external/nana/include` |
| Не очищаются метки | Файловая система должна быть NTFS |
| Программа не запускается | Установите Visual C++ Redistributable |

## 🤝 Вклад в развитие

1. Fork проекта
2. Создайте ветку функции (`git checkout -b feature/amazing-feature`)
3. Commit изменения (`git commit -m 'Add amazing feature'`)
4. Push в ветку (`git push origin feature/amazing-feature`)
5. Откройте Pull Request в `dev` ветку

## 📄 Лицензия

Этот проект лицензирован под **MIT License** — см. файл [LICENSE](LICENSE).

## 👥 Авторы

- **super-cow** — Initial work and ongoing maintenance

## 🙏 Благодарности

- [Nana C++ GUI](https://github.com/cnjinhao/nana) — за отличную библиотеку
- [Microsoft](https://docs.microsoft.com) — за документацию WinAPI
- Сообществу Stack Overflow — за помощь с решениями

---

**Версия**: 2.0  
**Статус**: ✅ Active Development  
**Последнее обновление**: 2026
