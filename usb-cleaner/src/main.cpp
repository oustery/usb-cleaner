/*
 * USB Cleaner v2.0 - Modern GUI Application with Nana
 * Очистка истории USB-устройств и меток флешек
 * 
 * Библиотека: Nana C++ GUI (https://github.com/cnjinhao/nana)
 * 
 * Сборка:
 * 1. Запустите setup.bat (скачает Nana и настроит проект)
 * 2. Или вручную:
 *    - Скачайте Nana: https://github.com/cnjinhao/nana/releases/download/v1.7.4/nana-1.7.4.zip
 *    - Распакуйте в external/nana/
 *    - Соберите: cmake --build build --config Release
 */

// Отключаем автоматические тесты Nana для production сборки
#ifdef _DEBUG
#define NANA_AUTOMATIC_GUI_TESTING
#endif

#include <nana/gui.hpp>
#include <nana/gui/widgets/form.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/progress.hpp>
#include <nana/gui/widgets/group.hpp>
#include <nana/gui/widgets/tooltip.hpp>

// Windows API для системных функций
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <setupapi.h>
#include <devguid.h>

// Стандартные библиотеки
#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <chrono>
#include <functional>
#include <iomanip>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "setupapi.lib")

using namespace nana;

// ==================== КОНСТАНТЫ ====================

namespace Constants {
    const wchar_t* USB_STOR_KEY = L"SYSTEM\\CurrentControlSet\\Enum\\USBSTOR";
    const wchar_t* USB_KEY = L"SYSTEM\\CurrentControlSet\\Enum\\USB";
    const wchar_t* MOUNTED_DEVICES_KEY = L"SYSTEM\\MountedDevices";
    
    const char* APP_NAME = "USB Cleaner";
    const char* APP_VERSION = "2.0.0";
    const char* GITHUB_URL = "https://github.com/oustery/usb-cleaner";
}

// ==================== ЦВЕТОВАЯ СХЕМА ====================

namespace Colors {
    // Основные цвета
    const color BACKGROUND(240, 245, 250);      // Светло-серый фон
    const color ACCENT(41, 128, 185);            // Синий акцент
    const color SUCCESS(39, 174, 96);            // Зелёный (успех)
    const color WARNING(243, 156, 18);           // Жёлтый (предупреждение)
    const color ERROR(231, 76, 60);              // Красный (ошибка)
    const color TEXT_PRIMARY(44, 62, 80);        // Тёмный текст
    const color TEXT_SECONDARY(127, 140, 141);   // Серый текст
    
    // Цвета кнопок
    const color BTN_PRIMARY(52, 152, 219);      // Основная кнопка
    const color BTN_SUCCESS(46, 204, 113);      // Кнопка успеха
    const color BTN_DANGER(231, 76, 60);        // Кнопка выхода
    const color BTN_DEFAULT(149, 165, 166);     // Обычная кнопка
}

// ==================== ЛОГГЕР ====================

class Logger {
private:
    std::mutex mtx;
    std::wofstream logFile;
    bool enabled = true;
    
public:
    explicit Logger(const std::wstring& filename = L"usb_cleaner.log") 
        : logFile(filename, std::ios::app) {
        if (!logFile.is_open()) {
            enabled = false;
        }
    }
    
    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }
    
    void log(const std::wstring& message) {
        if (!enabled) return;
        
        std::lock_guard<std::mutex> lock(mtx);
        
        SYSTEMTIME st;
        GetLocalTime(&st);
        
        logFile << L"["
                << st.wYear << L"-"
                << std::setw(2) << std::setfill(L'0') << st.wMonth << L"-"
                << std::setw(2) << std::setfill(L'0') << st.wDay << L" "
                << std::setw(2) << std::setfill(L'0') << st.wHour << L":"
                << std::setw(2) << std::setfill(L'0') << st.wMinute << L":"
                << std::setw(2) << std::setfill(L'0') << st.wSecond << L"] "
                << message << std::endl;
        
        logFile.flush();
    }
    
    void info(const std::wstring& msg) { log(L"[INFO] " + msg); }
    void warning(const std::wstring& msg) { log(L"[WARN] " + msg); }
    void error(const std::wstring& msg) { log(L"[ERROR] " + msg); }
};

static Logger logger;

// ==================== ПРОВЕРКА ПРАВ АДМИНИСТРАТОРА ====================

class AdminChecker {
public:
    static bool IsAdministrator() {
        BOOL isAdmin = FALSE;
        PSID adminSid = nullptr;
        SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NTAuthority;
        
        BOOL result = AllocateAndInitializeSid(
            &ntAuthority, 
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &adminSid
        );
        
        if (result && adminSid) {
            CheckTokenMembership(nullptr, adminSid, &isAdmin);
            FreeSid(adminSid);
        }
        
        return isAdmin == TRUE;
    }
    
    static void RestartAsAdmin() {
        wchar_t exePath[MAX_PATH] = {0};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpVerb = L"runas";
        sei.lpFile = exePath;
        sei.nShow = SW_SHOWNORMAL;
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        
        ShellExecuteExW(&sei);
    }
};

// ==================== РЕЗУЛЬТАТ ОПЕРАЦИИ ====================

struct OperationResult {
    bool success = true;
    int itemsProcessed = 0;
    std::string message;
    std::vector<std::wstring> details;
    std::chrono::milliseconds duration{0};
    
    static OperationResult Ok(const std::string& msg, int count = 0) {
        return {true, count, msg, {}, {}};
    }
    
    static OperationResult Error(const std::string& msg) {
        return {false, 0, msg, {}, {}};
    }
};

// ==================== ФУНКЦИИ РАБОТЫ С USB ====================

namespace USBUtils {
    
std::set<std::wstring> GetCurrentUSBDrives() {
    std::set<std::wstring> drives;
    DWORD drivesMask = GetLogicalDrives();
    
    for (char drive = 'A'; drive <= 'Z'; drive++) {
        if (!(drivesMask & (1 << (drive - 'A')))) continue;
        
        std::wstring path(1, static_cast<wchar_t>(drive));
        path += L":\\";
        
        UINT type = GetDriveType(path.c_str());
        if (type == DRIVE_REMOVABLE || type == DRIVE_FIXED) {
            drives.insert(std::wstring(1, static_cast<wchar_t>(drive)) + L":");
        }
    }
    
    return drives;
}

bool IsDeviceConnected(const std::wstring& hardwareId) {
    HDEVINFO hDevInfo = SetupDiGetClassDevs(
        nullptr, 
        L"USB", 
        nullptr, 
        DIGCF_PRESENT | DIGCF_ALLCLASSES
    );
    
    if (hDevInfo == INVALID_HANDLE_VALUE) return false;
    
    bool found = false;
    SP_DEVINFO_DATA devInfo{};
    devInfo.cbSize = sizeof(SP_DEVINFO_DATA);
    
    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfo); i++) {
        wchar_t deviceId[MAX_DEVICE_ID_LEN] = {0};
        
        if (SetupDiGetDeviceRegistryPropertyW(
            hDevInfo, 
            &devInfo, 
            SPDRP_HARDWAREID,
            nullptr, 
            reinterpret_cast<BYTE*>(deviceId), 
            sizeof(deviceId), 
            nullptr)) {
            
            std::wstring idStr(deviceId);
            if (idStr.find(hardwareId) != std::wstring::npos) {
                found = true;
                break;
            }
        }
    }
    
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return found;
}

bool DeleteRegistryKeyRecursive(HKEY hRootKey, const std::wstring& subKey) {
    HKEY hKey = nullptr;
    LONG result = RegOpenKeyExW(
        hRootKey, 
        subKey.c_str(), 
        0, 
        KEY_READ | KEY_WRITE | KEY_WOW64_64KEY, 
        &hKey
    );
    
    if (result != ERROR_SUCCESS) return false;
    
    DWORD subKeysCount = 0, maxSubKeyLen = 0;
    RegQueryInfoKeyW(
        hKey, 
        nullptr, nullptr, nullptr, 
        &subKeysCount, 
        &maxSubKeyLen, 
        nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr
    );
    
    if (subKeysCount > 0 && maxSubKeyLen > 0) {
        std::vector<wchar_t> subKeyName(maxSubKeyLen + 1);
        
        for (DWORD i = subKeysCount; i > 0; i--) {
            DWORD nameSize = maxSubKeyLen + 1;
            
            if (RegEnumKeyExW(
                hKey, 
                i - 1, 
                subKeyName.data(), 
                &nameSize,
                nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
                
                DeleteRegistryKeyRecursive(
                    hRootKey, 
                    subKey + L"\\" + subKeyName.data()
                );
            }
        }
    }
    
    RegCloseKey(hKey);
    
    result = RegDeleteKeyExW(
        hRootKey, 
        subKey.c_str(), 
        KEY_WOW64_64KEY, 
        0
    );
    
    return (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND);
}

OperationResult CleanUSBHistory(std::function<void(int)> progressCallback) {
    auto startTime = std::chrono::steady_clock::now();
    OperationResult result;
    
    logger.info(L"Начало очистки истории USB-устройств...");
    
    HKEY hUsbStorKey = nullptr;
    LONG openResult = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE, 
        Constants::USB_STOR_KEY, 
        0, 
        KEY_READ | KEY_WRITE | KEY_WOW64_64KEY, 
        &hUsbStorKey
    );
    
    if (openResult != ERROR_SUCCESS) {
        logger.error(L"Ошибка открытия реестра USBSTOR");
        return OperationResult::Error("Не удалось открыть реестр. Проверьте права администратора.");
    }
    
    DWORD subKeysCount = 0, maxSubKeyLen = 0;
    RegQueryInfoKeyW(
        hUsbStorKey, 
        nullptr, nullptr, nullptr, 
        &subKeysCount, 
        &maxSubKeyLen, 
        nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr
    );
    
    int processed = 0;
    int totalItems = static_cast<int>(subKeysCount) * 2; // Ориентировочно
    
    if (subKeysCount > 0 && maxSubKeyLen > 0) {
        std::vector<wchar_t> deviceClassName(maxSubKeyLen + 1);
        
        for (DWORD i = 0; i < subKeysCount; i++) {
            DWORD nameSize = maxSubKeyLen + 1;
            FILETIME lastWriteTime{};
            
            if (RegEnumKeyExW(
                hUsbStorKey, 
                i, 
                deviceClassName.data(), 
                &nameSize,
                nullptr, nullptr, nullptr, &lastWriteTime) == ERROR_SUCCESS) {
                
                std::wstring classPath = std::wstring(Constants::USB_STOR_KEY) + L"\\" + deviceClassName.data();
                
                HKEY hClassKey = nullptr;
                if (RegOpenKeyExW(
                    HKEY_LOCAL_MACHINE, 
                    classPath.c_str(), 
                    0,
                    KEY_READ | KEY_WRITE | KEY_WOW64_64KEY, 
                    &hClassKey) == ERROR_SUCCESS) {
                    
                    DWORD instanceCount = 0, maxInstanceLen = 0;
                    RegQueryInfoKeyW(
                        hClassKey, 
                        nullptr, nullptr, nullptr, 
                        &instanceCount,
                        &maxInstanceLen, 
                        nullptr,
                        nullptr, nullptr, nullptr, nullptr, nullptr
                    );
                    
                    if (instanceCount > 0 && maxInstanceLen > 0) {
                        std::vector<wchar_t> instanceName(maxInstanceLen + 1);
                        
                        for (DWORD j = 0; j < instanceCount; j++) {
                            DWORD instNameSize = maxInstanceLen + 1;
                            
                            if (RegEnumKeyExW(
                                hClassKey, 
                                j, 
                                instanceName.data(), 
                                &instNameSize,
                                nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
                                
                                std::wstring fullDeviceId = deviceClassName.data();
                                fullDeviceId += L"&";
                                fullDeviceId += instanceName.data();
                                
                                if (!IsDeviceConnected(fullDeviceId)) {
                                    std::wstring instancePath = classPath + L"\\" + instanceName.data();
                                    
                                    logger.info(L"Удаление отключенного устройства: " + fullDeviceId);
                                    
                                    if (DeleteRegistryKeyRecursive(HKEY_LOCAL_MACHINE, instancePath)) {
                                        result.itemsProcessed++;
                                        result.details.push_back(L"✓ Удалено: " + fullDeviceId);
                                    } else {
                                        result.details.push_back(L"✗ Ошибка: " + fullDeviceId);
                                    }
                                } else {
                                    result.details.push_back(L"⊙ Пропущено (подключено): " + fullDeviceId);
                                }
                                
                                processed++;
                                if (progressCallback && totalItems > 0) {
                                    progressCallback((processed * 100) / totalItems);
                                }
                            }
                        }
                    }
                    
                    RegCloseKey(hClassKey);
                }
            }
            
            if (progressCallback && totalItems > 0) {
                progressCallback((processed * 100) / totalItems);
            }
        }
    }
    
    RegCloseKey(hUsbStorKey);
    
    // Очищаем MountedDevices
    HKEY hMountedKey = nullptr;
    if (RegOpenKeyExW(
        HKEY_LOCAL_MACHINE, 
        Constants::MOUNTED_DEVICES_KEY, 
        0,
        KEY_READ | KEY_WRITE | KEY_WOW64_64KEY, 
        &hMountedKey) == ERROR_SUCCESS) {
        
        DWORD valuesCount = 0, maxValueNameLen = 0;
        RegQueryInfoKeyW(
            hMountedKey, 
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            &valuesCount, &maxValueNameLen, nullptr, nullptr, nullptr
        );
        
        if (valuesCount > 0 && maxValueNameLen > 0) {
            std::vector<wchar_t> valueName(maxValueNameLen + 1);
            
            for (DWORD i = 0; i < valuesCount; i++) {
                DWORD valueNameSize = maxValueNameLen + 1;
                
                if (RegEnumValueW(
                    hMountedKey, 
                    i, 
                    valueName.data(), 
                    &valueNameSize,
                    nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
                    
                    std::wstring valName(valueName.data());
                    
                    if (valName.find(L"\\DosDevices\\") != std::wstring::npos ||
                        valName.find(L"\\??\\") != std::wstring::npos) {
                        
                        std::wstring driveLetter;
                        size_t pos = valName.find(L"\\DosDevices\\");
                        if (pos != std::wstring::npos) {
                            driveLetter = valName.substr(pos + 12);
                        } else {
                            pos = valName.find(L"\\??\\");
                            if (pos != std::wstring::npos) {
                                driveLetter = valName.substr(pos + 4);
                            }
                        }
                        
                        if (!driveLetter.empty()) {
                            UINT type = GetDriveType(driveLetter.c_str());
                            if (type == DRIVE_NO_ROOT_DIR || type == DRIVE_UNKNOWN) {
                                RegDeleteValueW(hMountedKey, valName.c_str());
                                result.itemsProcessed++;
                                processed++;
                            }
                        }
                    }
                }
                
                if (progressCallback && totalItems > 0) {
                    progressCallback((processed * 100) / totalItems);
                }
            }
        }
        
        RegCloseKey(hMountedKey);
    }
    
    auto endTime = std::chrono::steady_clock::now();
    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    result.message = "Очистка истории USB завершена успешно";
    result.success = true;
    
    logger.info(L"Очистка завершена. Удалено записей: " + std::to_wstring(result.itemsProcessed));
    
    return result;
}

int RemoveZoneIdentifiers(const std::wstring& rootPath, 
                          std::function<void(int)> progressCallback,
                          int& processedCount,
                          int totalFiles) {
    int removedCount = 0;
    
    WIN32_FIND_DATAW findData{};
    std::wstring searchPattern = rootPath + L"*.*";
    
    HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return 0;
    
    do {
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0)
            continue;
        
        std::wstring fullPath = rootPath + findData.cFileName;
        
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            fullPath += L"\\";
            removedCount += RemoveZoneIdentifiers(fullPath, progressCallback, processedCount, totalFiles);
        } else {
            // Проверяем Zone.Identifier stream
            std::wstring zoneFile = fullPath + L":Zone.Identifier";
            DWORD attrs = GetFileAttributesW(zoneFile.c_str());
            
            if (attrs != INVALID_FILE_ATTRIBUTES) {
                SetFileAttributesW(zoneFile.c_str(), FILE_ATTRIBUTE_NORMAL);
                
                if (DeleteFileW(zoneFile.c_str())) {
                    removedCount++;
                    logger.info(L"Удален Zone.Identifier: " + fullPath);
                } else {
                    logger.warning(L"Не удалось удалить: " + zoneFile);
                }
            }
            
            processedCount++;
            if (progressCallback && totalFiles > 0) {
                progressCallback((processedCount * 100) / totalFiles);
            }
        }
        
    } while (FindNextFileW(hFind, &findData));
    
    FindClose(hFind);
    return removedCount;
}

int CountFilesInPath(const std::wstring& rootPath) {
    int count = 0;
    WIN32_FIND_DATAW findData{};
    std::wstring searchPattern = rootPath + L"*.*";
    
    HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return 0;
    
    do {
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0)
            continue;
        
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            std::wstring subPath = rootPath + findData.cFileName + L"\\";
            count += CountFilesInPath(subPath);
        } else {
            count++;
        }
    } while (FindNextFileW(hFind, &findData));
    
    FindClose(hFind);
    return count;
}

OperationResult CleanFlashLabels(std::function<void(int)> progressCallback) {
    auto startTime = std::chrono::steady_clock::now();
    OperationResult result;
    
    logger.info(L"Начало очистки меток флешек...");
    
    int totalRemoved = 0;
    std::set<std::wstring> processedDrives;
    
    DWORD drivesMask = GetLogicalDrives();
    std::vector<wchar_t> usbDrives;
    
    // Сначала собираем список USB дисков
    for (char drive = 'A'; drive <= 'Z'; drive++) {
        if (drivesMask & (1 << (drive - 'A'))) {
            std::wstring drivePath(1, static_cast<wchar_t>(drive));
            drivePath += L":\\";
            
            UINT type = GetDriveType(drivePath.c_str());
            if (type == DRIVE_REMOVABLE || type == DRIVE_FIXED) {
                usbDrives.push_back(drive);
            }
        }
    }
    
    int currentDrive = 0;
    int totalDrives = static_cast<int>(usbDrives.size());
    
    for (wchar_t drive : usbDrives) {
        currentDrive++;
        
        std::wstring drivePath(1, drive);
        drivePath += L":\\";
        
        wchar_t fileSystem[MAX_PATH] = {0};
        if (GetVolumeInformationW(
            drivePath.c_str(), 
            nullptr, 0, 
            nullptr, nullptr, nullptr, 
            fileSystem, 
            MAX_PATH)) {
            
            std::wstring driveLetter(1, drive);
            driveLetter += L":";
            
            if (processedDrives.find(driveLetter) == processedDrives.end()) {
                processedDrives.insert(driveLetter);
                
                logger.info(L"Обработка диска: " + driveLetter);
                
                // Считаем файлы для прогресса
                int fileCount = CountFilesInPath(drivePath);
                int processedCount = 0;
                
                int removed = RemoveZoneIdentifiers(
                    drivePath, 
                    [&, currentDrive, totalDrives](int p) {
                        if (progressCallback) {
                            int baseProgress = ((currentDrive - 1) * 100) / (totalDrives > 0 ? totalDrives : 1);
                            progressCallback(baseProgress + (p / (totalDrives > 0 ? totalDrives : 1)));
                        }
                    },
                    processedCount,
                    fileCount
                );
                
                totalRemoved += removed;
                
                std::wostringstream oss;
                oss << L"Диск " << driveLetter << L": удалено " << removed << L" меток";
                result.details.push_back(oss.str());
            }
        }
        
        if (progressCallback && totalDrives > 0) {
            progressCallback((currentDrive * 100) / totalDrives);
        }
    }
    
    // Обновляем кэш проводника
    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH, nullptr, nullptr);
    
    auto endTime = std::chrono::steady_clock::now();
    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    result.itemsProcessed = totalRemoved;
    result.message = "Очистка меток завершена успешно";
    result.success = true;
    
    logger.info(L"Очистка меток завершена. Удалено: " + std::to_wstring(totalRemoved));
    
    return result;
}

} // namespace USBUtils

// ==================== ГЛАВНОЕ ОКНО ПРИЛОЖЕНИЯ ====================

class USBCleanerApp {
private:
    form fm;
    
    // Группы
    group grp_usb, grp_flash;
    
    // Кнопки
    btn_clean_usb, btn_clean_flash, btn_exit, btn_about;
    
    // Прогресс-бары
    prog_usb, prog_flash;
    
    // Метки
    lbl_status, lbl_title, lbl_version;
    
    // Подсказки
    tooltip tip;
    
    // Состояние
    std::atomic<bool> isProcessing{false};
    
public:
    USBCleanerApp() 
        : fm(API::make_center(540, 600), API::window_caption("USB Cleaner"))
        , grp_usb(fm, rectangle(20, 90, 500, 190))
        , grp_flash(fm, rectangle(20, 300, 500, 190))
        , btn_clean_usb(grp_usb, rectangle(160, 130, 200, 45))
        , btn_clean_flash(grp_flash, rectangle(160, 130, 200, 45))
        , btn_exit(fm, rectangle(390, 520, 130, 40))
        , btn_about(fm, rectangle(20, 520, 130, 40))
        , prog_usb(grp_usb, rectangle(30, 95, 440, 28))
        , prog_flash(grp_flash, rectangle(30, 95, 440, 28))
        , lbl_status(fm, rectangle(20, 495, 500, 25))
        , lbl_title(fm, rectangle(20, 15, 500, 55))
        , lbl_version(fm, rectangle(420, 60, 100, 22))
        , tip(fm)
    {
        InitializeUI();
        BindEvents();
    }
    
    void InitializeUI() {
        // Главное окно
        fm.bgcolor(Colors::BACKGROUND);
        fm.caption(std::string("🔌 USB Cleaner v") + Constants::APP_VERSION);
        
        // Заголовок
        create_title();
        
        // Версия
        lbl_version.fgcolor(Colors::TEXT_SECONDARY);
        lbl_version.typeface(font("", 9));
        lbl_version.caption(std::string("v") + Constants::APP_VERSION);
        
        // Группа USB
        setup_group(grp_usb, "🔌 Очистка истории USB-устройств", Colors::TEXT_PRIMARY);
        
        // Кнопка очистки USB
        setup_button(btn_clean_usb, "🧹 Очистить историю USB", Colors::BTN_PRIMARY);
        tip.show(btn_clean_usb, "Удаляет записи об отключённых устройствах из системного реестра\nСохраняет информацию о текущих подключениях");
        
        // Прогресс-бар USB
        setup_progress(prog_usb);
        
        // Группа Flash
        setup_group(grp_flash, "💾 Очистка меток флешек (Zone.Identifier)", Colors::TEXT_PRIMARY);
        
        // Кнопка очистки меток
        setup_button(btn_clean_flash, "🗑️ Очистить метки NTFS", Colors::BTN_SUCCESS);
        tip.show(btn_clean_flash, "Удаляет альтернативные потоки NTFS (Zone.Identifier)\nУбирает метки безопасности файлов на флешках");
        
        // Прогресс-бар Flash
        setup_progress(prog_flash);
        
        // Статус
        lbl_status.fgcolor(Colors::SUCCESS);
        lbl_status.typeface(font("", 10));
        lbl_status.caption("✓ Готов к работе — запустите от имени администратора");
        
        // Кнопки управления
        setup_button(btn_exit, "✖️ Выход", Colors::BTN_DANGER);
        setup_button(btn_about, "ℹ️ О программе", Colors::BTN_DEFAULT);
        
        // Показываем окно
        fm.show();
        fm.modality();
    }
    
    void BindEvents() {
        // Очистка USB
        btn_clean_usb.events.click([this]() {
            if (isProcessing) return;
            
            auto choice = msgbox(fm, "Подтверждение операции", msgbox::yes_no);
            choice.icon(msgbox::icon_question);
            choice.message(
                "Вы уверены, что хотите очистить историю USB-устройств?\n\n"
                "Эта операция:\n"
                "• Удалит записи об ОТКЛЮЧЁННЫХ устройствах из реестра\n"
                "• Сохранит информацию о текущих подключениях\n"
                "• Очистит MountedDevices от устаревших записей\n\n"
                "⚠️ Действие необратимо!"
            );
            
            if (choice() == pick_ok) {
                StartUSBCleanup();
            }
        });
        
        // Очистка меток
        btn_clean_flash.events.click([this]() {
            if (isProcessing) return;
            
            auto choice = msgbox(fm, "Подтверждение операции", msgbox::yes_no);
            choice.icon(msgbox::icon_question);
            choice.message(
                "Вы уверены, что хотите очистить метки на флешках?\n\n"
                "Будут удалены:\n"
                "• Zone.Identifier потоки (метки безопасности)\n"
                "• Метки \"Эта программа загружена из Интернета\"\n"
                "• Другие альтернативные потоки данных NTFS\n\n"
                "⚠️ Операция может занять время при большом количестве файлов!"
            );
            
            if (choice() == pick_ok) {
                StartFlashCleanup();
            }
        });
        
        // Выход
        btn_exit.events.click([this]() {
            if (isProcessing) {
                auto confirm = msgbox(fm, "Подтверждение", msgbox::yes_no);
                confirm.icon(msgbox::icon_warning);
                confirm.message("Операция ещё выполняется.\nВы действительно хотите выйти?");
                if (confirm() != pick_ok) return;
            }
            API::close_window(fm);
        });
        
        // О программе
        btn_about.events.click([this]() {
            ShowAboutDialog();
        });
        
        // Закрытие окна
        fm.events.destroy([]() {
            API::exit();
        });
    }
    
    void StartUSBCleanup() {
        SetProcessingState(true);
        prog_usb.value(0);
        UpdateStatus("⏳ Выполняется очистка истории USB...", Colors::WARNING);
        
        std::thread([this]() {
            auto result = USBUtils::CleanUSBHistory([this](int progress) {
                fm.ui_thread([this, progress]() {
                    prog_usb.value(progress);
                });
            });
            
            fm.ui_thread([this, result]() {
                prog_usb.value(100);
                SetProcessingState(false);
                
                if (result.success) {
                    std::ostringstream msg;
                    msg << "✓ Успешно! Удалено записей: " << result.itemsProcessed 
                        << " (время: " << result.duration.count() << "мс)";
                    UpdateStatus(msg.str(), Colors::SUCCESS);
                    
                    ShowResultDialog("Результат очистки USB", result);
                } else {
                    UpdateStatus("✗ Ошибка: " + result.message, Colors::ERROR);
                }
            });
        }).detach();
    }
    
    void StartFlashCleanup() {
        SetProcessingState(true);
        prog_flash.value(0);
        UpdateStatus("⏳ Выполняется очистка меток на флешках...", Colors::WARNING);
        
        std::thread([this]() {
            auto result = USBUtils::CleanFlashLabels([this](int progress) {
                fm.ui_thread([this, progress]() {
                    prog_flash.value(progress);
                });
            });
            
            fm.ui_thread([this, result]() {
                prog_flash.value(100);
                SetProcessingState(false);
                
                if (result.success) {
                    std::ostringstream msg;
                    msg << "✓ Успешно! Удалено меток: " << result.itemsProcessed 
                        << " (время: " << result.duration.count() << "мс)";
                    UpdateStatus(msg.str(), Colors::SUCCESS);
                    
                    ShowResultDialog("Результат очистки меток", result);
                    
                    // Предложение перезапустить проводник
                    auto restart = msgbox(fm, "Перезапуск проводника", msgbox::yes_no);
                    restart.icon(msgbox::icon_information);
                    restart.message(
                        "Рекомендуется перезагрузить проводник Windows\n"
                        "для полного обновления кэша иконок.\n\n"
                        "Перезапустить сейчас?"
                    );
                    if (restart() == pick_ok) {
                        ShellExecuteW(nullptr, nullptr, L"explorer.exe", nullptr, nullptr, SW_HIDE);
                    }
                } else {
                    UpdateStatus("✗ Ошибка: " + result.message, Colors::ERROR);
                }
            });
        }).detach();
    }
    
    void ShowAboutDialog() {
        form about_fm(API::make_center(420, 400), API::window_caption("О программе"));
        about_fm.bgcolor(Colors::BACKGROUND);
        
        // Заголовок
        label title(about_fm, rectangle(10, 20, 400, 50));
        title.fgcolor(Colors::ACCENT);
        title.textalign(align::center);
        title.typeface(font("", 24, {700}));
        title.caption("🔌 USB Cleaner");
        
        // Версия
        label version(about_fm, rectangle(10, 75, 400, 28));
        version.fgcolor(Colors::TEXT_SECONDARY);
        version.textalign(align::center);
        version.typeface(font("", 12));
        version.caption(std::string("Версия ") + Constants::APP_VERSION);
        
        // Описание
        label desc(about_fm, rectangle(25, 115, 370, 180));
        desc.fgcolor(Colors::TEXT_PRIMARY);
        desc.textalign(align::left);
        desc.typeface(font("", 10));
        desc.caption(
            "Современное приложение для очистки истории\n"
            "подключённых USB-устройств и меток безопасности\n"
            "на флешках.\n\n"
            "Возможности:\n"
            "• Очистка реестра от записей об отключённых USB\n"
            "• Удаление Zone.Identifier потоков NTFS\n"
            "• Сохранение данных о текущих подключениях\n"
            "• Современный интерфейс на Nana C++ GUI\n"
            "• Прогресс операций в реальном времени\n\n"
            "Совместимость: Windows 7 / 8 / 10 / 11"
        );
        
        // Авторство
        label copyright(about_fm, rectangle(10, 310, 400, 30));
        copyright.fgcolor(Colors::TEXT_SECONDARY);
        copyright.textalign(align::center);
        copyright.typeface(font("", 9));
        copyright.caption("(c) 2026 USB Cleaner Contributors");
        
        // Лицензия
        label license(about_fm, rectangle(10, 340, 400, 25));
        license.fgcolor(Colors::ACCENT);
        license.textalign(align::center);
        license.typeface(font("", 9));
        license.caption("MIT License — Open Source");
        
        // Ссылка на GitHub
        label github(about_fm, rectangle(10, 365, 400, 25));
        github.fgcolor(Colors::ACCENT);
        github.textalign(align::center);
        github.typeface(font("", 9, {400}, true)); // underline
        github.caption(Constants::GITHUB_URL);
        
        // Кнопка OK
        button btn_ok(about_fm, rectangle(145, 330, 130, 38));
        btn_ok.caption("OK");
        btn_ok.bgcolor(Colors::ACCENT);
        btn_ok.fgcolor(colors::white);
        btn_ok.typeface(font("", 11, {600}));
        
        btn_ok.events.click([&about_fm]() {
            API::close_window(about_fm);
        });
        
        about_fm.modality();
    }
    
    void ShowResultDialog(const std::string& title, const OperationResult& result) {
        form result_fm(API::make_center(450, 350), API::window_caption(title));
        result_fm.bgcolor(Colors::BACKGROUND);
        
        // Заголовок результата
        label res_title(result_fm, rectangle(10, 15, 430, 35));
        res_title.fgcolor(result.success ? Colors::SUCCESS : Colors::ERROR);
        res_title.textalign(align::center);
        res_title.typeface(font("", 16, {600}));
        res_title.caption(result.success ? "✓ Операция выполнена успешно" : "✗ Ошибка выполнения");
        
        // Статистика
        label stats(result_fm, rectangle(20, 60, 410, 80));
        stats.fgcolor(Colors::TEXT_PRIMARY);
        stats.typeface(font("", 11));
        
        std::ostringstream stats_text;
        stats_text << "Обработано элементов: " << result.itemsProcessed << "\n"
                  << "Время выполнения: " << result.duration.count() << " мс\n"
                  << "Статус: " << (result.success ? "Успех" : "Ошибка");
        stats.caption(stats_text.str());
        
        // Детали (если есть)
        if (!result.details.empty()) {
            label details_label(result_fm, rectangle(20, 150, 410, 20));
            details_label.fgcolor(Colors::TEXT_SECONDARY);
            details_label.typeface(font("", 10, {600}));
            details_label.caption("Детали операции:");
            
            // Показываем только первые 5 деталей
            std::wostringstream details_text;
            size_t showCount = std::min(result.details.size(), size_t(5));
            for (size_t i = 0; i < showCount; i++) {
                details_text << result.details[i] << L"\n";
            }
            if (result.details.size() > 5) {
                details_text << L"... и ещё " << (result.details.size() - 5) << L" записей";
            }
            
            label details(result_fm, rectangle(20, 175, 410, 100));
            details.fgcolor(Colors::TEXT_PRIMARY);
            details.typeface(font("", 9));
            details.caption(details_text.str());
        }
        
        // Кнопка закрытия
        button btn_close(result_fm, rectangle(155, 290, 140, 38));
        btn_close.caption("Закрыть");
        btn_close.bgcolor(Colors::ACCENT);
        btn_close.fgcolor(colors::white);
        btn_close.typeface(font("", 11, {600}));
        
        btn_close.events.click([&result_fm]() {
            API::close_window(result_fm);
        });
        
        result_fm.modality();
    }
    
    void Run() {
        exec();
    }

private:
    // Вспомогательные методы для создания UI
    
    void create_title() {
        lbl_title.fgcolor(Colors::ACCENT);
        lbl_title.textalign(align::center);
        lbl_title.typeface(font("", 20, {700}));
        lbl_title.caption("USB Cleaner — Управление устройствами");
    }
    
    void setup_group(group& grp, const std::string& caption, const color& fg_color) {
        grp.caption(caption);
        grp.fgcolor(fg_color);
        grp.typeface(font("", 12, {600}));
    }
    
    void setup_button(button& btn, const std::string& caption, const color& bg_color) {
        btn.caption(caption);
        btn.bgcolor(bg_color);
        btn.fgcolor(colors::white);
        btn.typeface(font("", 11, {600}));
        btn.effect(bground::effect::bground_transparent(0.1, [bg_color](graphics& grp) {
            grp.rectangle(true, bg_color);
        }));
    }
    
    void setup_progress(progress& prog) {
        prog.amount(100);
        prog.value(0);
        prog.bgcolor(color(220, 225, 230));
        prog.fgcolor(Colors::ACCENT);
    }
    
    void SetProcessingState(bool processing) {
        isProcessing = processing;
        btn_clean_usb.enable(!processing);
        btn_clean_flash.enable(!processing);
    }
    
    void UpdateStatus(const std::string& text, const color& text_color) {
        lbl_status.caption(text);
        lbl_status.fgcolor(text_color);
    }
};

// ==================== TOCHKA ВХОДА ====================

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Проверка прав администратора
    if (!AdminChecker::IsAdministrator()) {
        int choice = MessageBoxW(
            nullptr, 
            L"Программа требует прав администратора для работы с системным реестром.\n\n"
            L"Хотите перезапустить программу с правами администратора?",
            L"USB Cleaner — Требуются права",
            MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1
        );
        
        if (choice == IDYES) {
            AdminChecker::RestartAsAdmin();
        }
        return 0;
    }
    
    logger.info(L"Программа запущена (режим администратора)");
    
    try {
        USBCleanerApp app;
        app.Run();
    } catch (const std::exception& e) {
        logger.error(L"Критическая ошибка: " + 
                     std::wstring(e.what(), e.what() + strlen(e.what())));
        
        MessageBoxA(
            nullptr, 
            (std::string("Критическая ошибка: ") + e.what()).c_str(),
            "USB Cleaner — Critical Error",
            MB_ICONERROR
        );
        return 1;
    } catch (...) {
        logger.error(L"Неизвестная критическая ошибка");
        MessageBoxA(
            nullptr, 
            "Произошла неизвестная критическая ошибка.",
            "USB Cleaner — Critical Error",
            MB_ICONERROR
        );
        return 1;
    }
    
    logger.info(L"Программа успешно закрыта");
    
    return 0;
}
