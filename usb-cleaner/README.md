# 🔌 USB Cleaner v2.0

[![Build Status](https://github.com/oustery/usb-cleaner/actions/workflows/build.yml/badge.svg)](https://github.com/oustery/usb-cleaner/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-blue.svg)](https://github.com/oustery/usb-cleaner)
[![Version](https://img.shields.io/badge/version-2.0.0-green.svg)](https://github.com/oustery/usb-cleaner/releases/latest)

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
- ✅ Диалоги подтверждения и результатов

## 📸 Скриншоты

```
┌─────────────────────────────────────────────────────┐
│  🔌 USB Cleaner v2.0.0                         [_][×]│
│                                                     │
│     USB Cleaner — Управление устройствами           │
│                        v2.0.0                       │
│                                                     │
│  ── 🔌 Очистка истории USB-устройств ──            │
│  [═══════════════════════════════════]   0%         │
│                                                    │
│            [ 🧹 Очистить историю USB ]              │
│                                                     │
│  ── 💾 Очистка меток флешек (Zone.Identifier) ──    │
│  [═══════════════════════════════════]   0%         │
│                                                    │
│            [ 🗑️ Очистить метки NTFS ]               │
│                                                     │
│  ✓ Готов к работе — запустите от имени админа...    │
│                                                    │
│          [ℹ️ О программе]        [✖️ Выход]          │
└─────────────────────────────────────────────────────┘
```

## 🚀 Быстрый старт

### Требования
| Компонент | Версия |
|-----------|--------|
| **Windows** | 7 / 8 / 10 / 11 (64-bit) |
| **Права** | Администратор (обязательно!) |
| **Компилятор** | MinGW-w64 11+ или MSVC 2019+ |
| **CMake** | 3.15+ (опционально) |

### ⚡ Быстрая установка (Windows)

1. **Клонируйте репозиторий:**
   ```bash
   git clone https://github.com/oustery/usb-cleaner.git
   cd usb-cleaner
   ```

2. **Запустите установщик:**
   ```bash
   setup.bat
   ```
   Этот скрипт автоматически:
   - Скачает Nana GUI библиотеку
   - Настроит проект
   - Подготовит к сборке

3. **Соберите проект:**
   ```bash
   cd build
   cmake --build . --config Release --parallel
   ```

4. **Запустите:**
   ```bash
   # Правый клик → "Запуск от имени администратора"
   usb_cleaner.exe
   ```

### 🔧 Ручная сборка

#### С использованием CMake

```bash
# Клонирование
git clone https://github.com/oustery/usb-cleaner.git
cd usb-cleaner

# Загрузка Nana (вручную)
# Скачать: https://github.com/cnjinhao/nana/releases/download/v1.7.4/nana-1.7.4.zip
# Распаковать в: external/nama/

# Конфигурация (MinGW)
mkdir build && cd build
cmake .. -G "MinGW Makefiles" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DUSE_NANA=ON ^
  -DNANA_INSTALL_DIR="../external/nana"

# Сборка
cmake --build . --config Release --parallel

# Или с MSVC
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release --parallel
```

#### С использованием MinGW напрямую

```bash
g++ -std=c++17 -o usb_cleaner.exe src/main.cpp \
    -I./external/nana/include \
    -DNANA_AUTOMATIC_GUI_TESTING \
    -lcomctl32 -lshell32 -lole32 -luuid \
    -lgdi32 -lcomdlg32 -lsetupapi \
    -mwindows -static -DNDEBUG -O2
```

#### С использованием MSVC

```cmd
cl /EHsc /std:c++17 /Fe:usb_cleaner.exe src/main.cpp ^
    /I external\nana\include ^
    /D NANA_AUTOMATIC_GUI_TESTING ^
    comctl32.lib shell32.lib ole32.lib uuid.lib ^
    gdi32.lib comdlg32.lib setupapi.lib ^
    /link /SUBSYSTEM:WINDOWS
```

## 📁 Структура проекта

```
usb-cleaner/
├── src/
│   └── main.cpp                 # Основной код (~800 строк)
├── external/
│   └── nana/                    # Библиотека Nana GUI (загружается через setup.bat)
├── .github/
│   └── workflows/
│       └── build.yml            # GitHub Actions CI/CD
├── cmake/                       # Дополнительные CMake модули
├── CMakeLists.txt               # Основная конфигурация сборки
├── Makefile                     # Альтернативная сборка (MinGW)
├── setup.bat                    # Автоматическая установка зависимостей
├── README.md                    # Документация (этот файл)
├── CHANGELOG.md                 # История изменений
├── CONTRIBUTING.md              # Руководство для контрибьюторов
├── LICENSE                      # MIT License
└── .gitignore                   # Правила Git
```

## 🔄 Ветки репозитория

| Ветка | Назначение | Статус |
|-------|------------|--------|
| `master` | Стабильные релизы | ✅ Production-ready |
| `dev` | Разработка новых функций | 🚧 In Development |

## 🔄 GitHub Actions (CI/CD)

Проект включает полностью автоматизированный пайплайн:

### Автоматически при push:
```yaml
✅ Windows MinGW-w64 — сборка с GCC 13.x
✅ Windows MSVC 2022 — сборка с Visual Studio
⚠️ Linux GCC — проверка компиляции (опционально)
```

### При создании тега (`v*`):
```yaml
📦 Создание GitHub Release
📋 Автоматический changelog
🎁 Публикация артефактов (.zip)
🏷️ Превью-релиз для beta версий
```

### Как создать релиз:

```bash
# Стабильный релиз
git tag v2.0.0
git push origin v2.0.0

# Beta версия
git tag v2.1.0-beta.1
git push origin v2.1.0-beta.1

# RC версия
git tag v2.1.0-rc.1
git push origin v2.1.0-rc.1
```

## 🛠️ Использование

### Запуск программы

1. **Запустите от имени администратора**
   - Правый клик по `usb_cleaner.exe`
   - Выберите "Запуск от имени администратора"

2. **Выберите действие:**

| Кнопка | Функция | Время выполнения |
|--------|---------|------------------|
| 🧹 **Очистить историю USB** | Удаляет записи об отключённых устройствах | 5-30 сек |
| 🗑️ **Очистить метки NTFS** | Удаляет Zone.Identifier на флешках | 10-60 сек |

3. **Следите за прогрессом** в реальном времени

4. **Просмотрите результаты** во всплывающем диалоге

### Что происходит при очистке:

#### USB История:
```
1. Открывается HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Enum\USBSTOR
2. Перечисляются все классы устройств
3. Для каждого экземпляра проверяется подключение через SetupAPI
4. Отключённые устройства удаляются из реестра
5. Очищается MountedDevices от устаревших записей
```

#### Метки флешек:
```
1. Находятся все съёмные диски (A:-Z:)
2. Для каждого диска рекурсивно обходятся файлы
3. Проверяется наличие :Zone.Identifier потока
4. Поток удаляется (если существует)
5. Обновляется кэш проводника
```

## ⚙️ Технические детали

### Архитектура приложения

```
┌─────────────────────────────────────────────────────┐
│                  Presentation Layer                  │
│  (Nana C++ GUI: Forms, Buttons, Progress Bars)      │
├─────────────────────────────────────────────────────┤
│                Business Logic Layer                  │
│  (CleanUSBHistory, CleanFlashLabels, OperationResult)│
├─────────────────────────────────────────────────────┤
│             System Integration Layer                 │
│  (Registry API, SetupAPI, NTFS ADS, ShellAPI)       │
├─────────────────────────────────────────────────────┤
│              Infrastructure Layer                    │
│  (Logger, AdminChecker, Error Handling)             │
└─────────────────────────────────────────────────────┘
```

### Ключевые технологии

| Технология | Назначение |
|------------|------------|
| **Nana C++ GUI** | Кроссплатформенный UI фреймворк |
| **Win32 Registry API** | Работа с системным реестром |
| **SetupAPI** | Обнаружение и перечисление USB устройств |
| **NTFS ADS** | Работа с альтернативными потоками данных |
| **ShellAPI** | Интеграция с проводником Windows |
| **C++17** | Современный стандарт C++ |
| **CMake** | Система сборки |

### Цветовая схема интерфейса

| Элемент | Цвет | Hex |
|---------|------|-----|
| Фон | Светло-серый | `#F0F5FA` |
| Акцент | Синий | `#2980B9` |
| Успех | Зелёный | `#27AE60` |
| Предупреждение | Жёлтый | `#F39C12` |
| Ошибка | Красный | `#E74C3C` |
| Текст основной | Тёмно-серый | `#2C3E50` |
| Текст вторичный | Серый | `#7F8C8D` |

## 🐛 Troubleshooting

| Проблема | Причина | Решение |
|----------|---------|---------|
| "Требуются права администратора" | Нет прав на реестр | Запустите от имени администратора |
| Ошибка компиляции Nana | Не найден include path | Проверьте `external/nana/include` |
| Не очищаются метки | Файловая система не NTFS | Используйте NTFS форматирование |
| Программа не запускается | Отсутствуют DLL | Установите VC++ Redistributable |
| Иконка не отображается | Отсутствует ресурс | Добавьте .ico файл в ресурсы |
| Progress bar не работает | Потоки блокируются | Проверьте `fm.ui_thread()` вызовы |

### Получение помощи

1. Проверьте лог-файл: `usb_cleaner.log`
2. Посмотрите [известные проблемы](https://github.com/oustery/usb-cleaner/issues)
3. Создайте [новый issue](https://github.com/oustery/usb-cleaner/issues/new) с подробным описанием

## 🤝 Вклад в развитие

Мы приветствуем вклад сообщества! См. [CONTRIBUTING.md](CONTRIBUTING.md) для деталей.

### Быстрый старт для разработчиков:

```bash
# Fork и клонирование
git clone https://github.com/YOUR_USERNAME/usb-cleaner.git
cd usb-cleaner

# Создание ветки функции
git checkout -b feature/amazing-feature

# Внесение изменений
# ... код ...

# Commit и push
git commit -m "feat: add amazing feature"
git push origin feature/amazing-feature

# Создание Pull Request (target: dev branch)
# https://github.com/oustery/usb-cleaner/compare/dev...YOUR_USERNAME:feature/amazing-feature
```

## 📄 Лицензия

Этот проект лицензирован под **MIT License** — см. файл [LICENSE](LICENSE).

```
MIT License

Copyright (c) 2026 USB Cleaner Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
```

## 👥 Авторы

- **super-cow** ([@oustery](https://github.com/oustery)) — Initial work and ongoing maintenance

## 🙏 Благодарности

- **[Nana C++ GUI](https://github.com/cnjinhao/nana)** — За отличную библиотеку GUI
- **[Microsoft](https://docs.microsoft.com)** — За документацию WinAPI
- **Stack Overflow Community** — За помощь с решениями
- **All Contributors** — За улучшения проекта

## 📊 Статистика проекта

| Метрика | Значение |
|---------|----------|
| Язык | C++17 |
| Строк кода | ~800 |
| Файлов | 15+ |
| Лицензия | MIT |
| Первая версия | 2026-01-24 |
| Последнее обновление | 2026-01-25 |

## 📞 Контакты

- **GitHub Issues**: [Создать issue](https://github.com/oustery/usb-cleaner/issues/new)
- **GitHub Discussions**: [Обсуждения](https://github.com/oustery/usb-cleaner/discussions)
- **Email**: oustery@mail.ru

---

<div align="center">

**Версия**: 2.0.0  
**Статус**: ✅ Stable & Active Development  
**Последнее обновление**: 2026-01-25  
**Совместимость**: Windows 7+

[📥 Download Latest Release](https://github.com/oustery/usb-cleaner/releases/latest) • 
[📖 Documentation](README.md) • 
[🤝 Contributing](CONTRIBUTING.md) • 
[📋 Changelog](CHANGELOG.md)

Made with ❤️ by super-cow

</div>
