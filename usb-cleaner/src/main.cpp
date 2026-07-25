/*
 * USB Cleaner v2.0 - Modern GUI Application with Nana
 * Очистка истории USB-устройств и меток флешек
 * 
 * Библиотека: Nana C++ GUI (https://github.com/cnjinhao/nana)
 * 
 * Сборка:
 * 1. Скачайте Nana: https://github.com/cnjinhao/nana/releases
 * 2. Распакуйте в папку /nana рядом с проектом
 * 3. Скомпилируйте:
 *    g++ -std=c++17 -o usb_cleaner.exe src/main.cpp 
 *        -I./nana/include -DNANA_AUTOMATIC_GUI_TESTING
 *        -lcomctl32 -lshell32 -lole32 -luuid -lgdi32 -lcomdlg32
 *        -mwindows -static -DNDEBUG
 */

#define NANA_AUTOMATIC_GUI_TESTING
#include <nana/gui.hpp>
#include <nana/gui/widgets/form.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/progress.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/group.hpp>
#include <nana/gui/widgets/picture.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/timer.hpp>
#include <nana/gui/tooltip.hpp>
#include <nana/gui/filebox.hpp>

// Windows API для системных функций
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>

// Стандартные библиотеки
#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <chrono>
#include <functional>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "gdi32.lib")

using namespace nana;

// ==================== КОНСТАНТЫ ====================

const wchar_t* USB_STOR_KEY = L"SYSTEM\\CurrentControlSet\\Enum\\USBSTOR";
const wchar_t* USB_KEY = L"SYSTEM\\CurrentControlSet\\Enum\\USB";
const wchar_t* MOUNTED_DEVICES_KEY = L"SYSTEM\\MountedDevices";
const char* APP_NAME = "USB Cleaner";
const char* APP_VERSION = "v2.0";

// ==================== СТРУКТУРЫ ====================

struct USBDeviceInfo {
    std::wstring deviceName;
    std::wstring devicePath;
    bool isConnected;
};

struct OperationResult {
    bool success;
    int itemsProcessed;
    std::string message;
    std::vector<std::wstring> details;
};

// ==================== ЛОГГЕР ====================

class Logger {
private:
    std::mutex mtx;
    std::wofstream logFile;
    
public:
    Logger() {
        logFile.open("usb_cleaner.log", std::ios::app);
    }
    
    ~Logger() {
        if (logFile.is_open()) logFile.close();
    }
    
    void log(const std::wstring& message) {
        std::lock_guard<std::mutex> lock(mtx);
        if (logFile.is_open()) {
            SYSTEMTIME st;
            GetLocalTime(&st);
            logFile << L"[" << st.wYear << L"-" << std::setw(2) << std::setfill(L'0') << st.wMonth 
                    << L"-" << std::setw(2) << std::setfill(L'0') << st.wDay << L" " 
                    << std::setw(2) << std::setfill(L'0') << st.wHour << L":" 
                    << std::setw(2) << std::setfill(L'0') << st.wMinute << L":" 
                    << std::setw(2) << std::setfill(L'0') << st.wSecond << L"] " 
                    << message << std::endl;
            logFile.flush();
        }
    }
};

static Logger logger;

// ==================== ПРОВЕРКА АДМИНИСТРАТОРА ====================

bool IsAdministrator() {
    BOOL isAdmin = FALSE;
    PSID adminSid = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NTAuthority;
    
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, 
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminSid)) {
        if (!CheckTokenMembership(NULL, adminSid, &isAdmin)) {
            isAdmin = FALSE;
        }
        FreeSid(adminSid);
    }
    
    return isAdmin == TRUE;
}

void RestartAsAdmin() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.lpVerb = L"runas";
    sei.lpFile = exePath;
    sei.nShow = SW_SHOWNORMAL;
    
    ShellExecuteExW(&sei);
}

// ==================== ФУНКЦИИ РАБОТЫ С USB ====================

std::set<std::wstring> GetCurrentUSBDrives() {
    std::set<std::wstring> drives;
    DWORD drivesMask = GetLogicalDrives();
    
    for (char drive = 'A'; drive <= 'Z'; drive++) {
        if (drivesMask & (1 << (drive - 'A'))) {
            std::wstring path = std::wstring(1, drive) + L":\\";
            UINT type = GetDriveType(path.c_str());
            
            if (type == DRIVE_REMOVABLE || type == DRIVE_FIXED) {
                drives.insert(std::wstring(1, drive) + L":");
            }
        }
    }
    
    return drives;
}

bool IsUSBDeviceConnected(const std::wstring& hardwareId) {
    HDEVINFO hDevInfo = SetupDiGetClassDevs(NULL, L"USB", NULL, DIGCF_PRESENT | DIGCF_ALLCLASSES);
    if (hDevInfo == INVALID_HANDLE_VALUE) return false;
    
    bool found = false;
    SP_DEVINFO_DATA devInfo = {};
    devInfo.cbSize = sizeof(SP_DEVINFO_DATA);
    
    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfo); i++) {
        wchar_t deviceId[MAX_DEVICE_ID_LEN];
        if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfo, SPDRP_HARDWAREID,
            NULL, (PBYTE)deviceId, sizeof(deviceId), NULL)) {
            
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
    HKEY hKey;
    LONG result = RegOpenKeyExW(hRootKey, subKey.c_str(), 0, KEY_READ | KEY_WRITE | KEY_WOW64_64KEY, &hKey);
    
    if (result != ERROR_SUCCESS) return false;
    
    DWORD subKeysCount = 0, maxSubKeyLen = 0;
    RegQueryInfoKeyW(hKey, NULL, NULL, NULL, &subKeysCount, &maxSubKeyLen, NULL,
        NULL, NULL, NULL, NULL, NULL);
    
    if (subKeysCount > 0 && maxSubKeyLen > 0) {
        std::vector<wchar_t> subKeyName(maxSubKeyLen + 1);
        
        for (DWORD i = subKeysCount; i > 0; i--) {
            DWORD nameSize = maxSubKeyLen + 1;
            FILETIME lastWriteTime;
            
            if (RegEnumKeyExW(hKey, i - 1, subKeyName.data(), &nameSize,
                NULL, NULL, NULL, &lastWriteTime) == ERROR_SUCCESS) {
                
                std::wstring childPath = subKey + L"\\" + subKeyName.data();
                DeleteRegistryKeyRecursive(hRootKey, childPath);
            }
        }
    }
    
    RegCloseKey(hKey);
    
    result = RegDeleteKeyExW(hRootKey, subKey.c_str(), KEY_WOW64_64KEY, 0);
    return (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND);
}

OperationResult CleanUSBHistory(std::function<void(int)> progressCallback) {
    OperationResult result{true, 0, "", {}};
    
    logger.log(L"Начало очистки истории USB-устройств...");
    
    HKEY hUsbStorKey;
    LONG openResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, USB_STOR_KEY, 0, 
        KEY_READ | KEY_WRITE | KEY_WOW64_64KEY, &hUsbStorKey);
    
    if (openResult != ERROR_SUCCESS) {
        result.success = false;
        result.message = "Ошибка открытия реестра USBSTOR";
        return result;
    }
    
    // Получаем список устройств
    DWORD subKeysCount = 0, maxSubKeyLen = 0;
    RegQueryInfoKeyW(hUsbStorKey, NULL, NULL, NULL, &subKeysCount,
        &maxSubKeyLen, NULL, NULL, NULL, NULL, NULL, NULL);
    
    int processed = 0;
    
    if (subKeysCount > 0 && maxSubKeyLen > 0) {
        std::vector<wchar_t> deviceClassName(maxSubKeyLen + 1);
        
        for (DWORD i = 0; i < subKeysCount; i++) {
            DWORD nameSize = maxSubKeyLen + 1;
            FILETIME lastWriteTime;
            
            if (RegEnumKeyExW(hUsbStorKey, i, deviceClassName.data(), &nameSize,
                NULL, NULL, NULL, &lastWriteTime) == ERROR_SUCCESS) {
                
                std::wstring classPath = std::wstring(USB_STOR_KEY) + L"\\" + deviceClassName.data();
                
                HKEY hClassKey;
                if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, classPath.c_str(), 0,
                    KEY_READ | KEY_WRITE | KEY_WOW64_64KEY, &hClassKey) == ERROR_SUCCESS) {
                    
                    DWORD instanceCount = 0, maxInstanceLen = 0;
                    RegQueryInfoKeyW(hClassKey, NULL, NULL, NULL, &instanceCount,
                        &maxInstanceLen, NULL, NULL, NULL, NULL, NULL, NULL);
                    
                    if (instanceCount > 0 && maxInstanceLen > 0) {
                        std::vector<wchar_t> instanceName(maxInstanceLen + 1);
                        
                        for (DWORD j = 0; j < instanceCount; j++) {
                            DWORD instNameSize = maxInstanceLen + 1;
                            
                            if (RegEnumKeyExW(hClassKey, j, instanceName.data(), &instNameSize,
                                NULL, NULL, NULL, &lastWriteTime) == ERROR_SUCCESS) {
                                
                                std::wstring fullDeviceId = deviceClassName.data();
                                fullDeviceId += L"&";
                                fullDeviceId += instanceName.data();
                                
                                if (!IsUSBDeviceConnected(fullDeviceId)) {
                                    std::wstring instancePath = classPath + L"\\" + instanceName.data();
                                    
                                    logger.log(L"Удаление: " + fullDeviceId);
                                    
                                    if (DeleteRegistryKeyRecursive(HKEY_LOCAL_MACHINE, instancePath)) {
                                        result.itemsProcessed++;
                                        result.details.push_back(L"Удалено: " + fullDeviceId);
                                    } else {
                                        result.details.push_back(L"Ошибка удаления: " + fullDeviceId);
                                    }
                                } else {
                                    result.details.push_back(L"Пропущено (подключено): " + fullDeviceId);
                                }
                                
                                processed++;
                                if (progressCallback) {
                                    progressCallback((processed * 100) / (int)(subKeysCount * 2));
                                }
                            }
                        }
                    }
                    
                    RegCloseKey(hClassKey);
                }
                
                if (progressCallback) {
                    progressCallback((processed * 100) / (int)(subKeysCount * 2));
                }
            }
        }
    }
    
    RegCloseKey(hUsbStorKey);
    
    // Очищаем MountedDevices
    HKEY hMountedKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, MOUNTED_DEVICES_KEY, 0,
        KEY_READ | KEY_WRITE | KEY_WOW64_64KEY, &hMountedKey) == ERROR_SUCCESS) {
        
        DWORD valuesCount = 0, maxValueNameLen = 0;
        RegQueryInfoKeyW(hMountedKey, NULL, NULL, NULL, NULL, NULL, NULL,
            &valuesCount, &maxValueNameLen, NULL, NULL, NULL);
        
        if (valuesCount > 0 && maxValueNameLen > 0) {
            std::vector<wchar_t> valueName(maxValueNameLen + 1);
            
            for (DWORD i = 0; i < valuesCount; i++) {
                DWORD valueNameSize = maxValueNameLen + 1;
                
                if (RegEnumValueW(hMountedKey, i, valueName.data(), &valueNameSize,
                    NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                    
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
                
                if (progressCallback) {
                    progressCallback((processed * 100) / (int)(subKeysCount * 2));
                }
            }
        }
        
        RegCloseKey(hMountedKey);
    }
    
    result.message = "Очистка завершена успешно";
    logger.log(L"Завершено. Удалено: " + std::to_wstring(result.itemsProcessed));
    
    return result;
}

int RemoveZoneIdentifiers(const std::wstring& rootPath, std::function<void(int)> progressCallback, int totalFiles) {
    static int processed = 0;
    int removedCount = 0;
    
    WIN32_FIND_DATAW findData;
    std::wstring searchPattern = rootPath + L"*.*";
    
    HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return 0;
    
    do {
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0)
            continue;
        
        std::wstring fullPath = rootPath + findData.cFileName;
        
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            fullPath += L"\\";
            removedCount += RemoveZoneIdentifiers(fullPath, progressCallback, totalFiles);
        } else {
            std::wstring zoneFile = fullPath + L":Zone.Identifier";
            DWORD attrs = GetFileAttributesW(zoneFile.c_str());
            
            if (attrs != INVALID_FILE_ATTRIBUTES) {
                SetFileAttributesW(zoneFile.c_str(), FILE_ATTRIBUTE_NORMAL);
                
                if (DeleteFileW(zoneFile.c_str())) {
                    removedCount++;
                    logger.log(L"Удален Zone.Identifier: " + fullPath);
                }
            }
            
            processed++;
            if (progressCallback && totalFiles > 0) {
                progressCallback((processed * 100) / totalFiles);
            }
        }
        
    } while (FindNextFileW(hFind, &findData));
    
    FindClose(hFind);
    return removedCount;
}

OperationResult CleanFlashLabels(std::function<void(int)> progressCallback) {
    OperationResult result{true, 0, "", {}};
    
    logger.log(L"Начало очистки меток флешек...");
    
    int totalRemoved = 0;
    std::set<std::wstring> processedDrives;
    
    DWORD drivesMask = GetLogicalDrives();
    int driveCount = 0;
    
    // Сначала считаем диски
    for (char drive = 'A'; drive <= 'Z'; drive++) {
        if (drivesMask & (1 << (drive - 'A'))) {
            std::wstring drivePath = std::wstring(1, drive) + L":\\";
            UINT type = GetDriveType(drivePath.c_str());
            if (type == DRIVE_REMOVABLE || type == DRIVE_FIXED) {
                driveCount++;
            }
        }
    }
    
    int currentDrive = 0;
    
    for (char drive = 'A'; drive <= 'Z'; drive++) {
        if (!(drivesMask & (1 << (drive - 'A')))) continue;
        
        std::wstring drivePath = std::wstring(1, drive) + L":\\";
        UINT type = GetDriveType(drivePath.c_str());
        
        if (type == DRIVE_REMOVABLE || type == DRIVE_FIXED) {
            currentDrive++;
            
            wchar_t fileSystem[MAX_PATH];
            if (GetVolumeInformationW(drivePath.c_str(), NULL, 0, NULL, NULL, NULL, 
                fileSystem, MAX_PATH)) {
                
                std::wstring driveLetter(1, drive);
                driveLetter += L":";
                
                if (processedDrives.find(driveLetter) == processedDrives.end()) {
                    processedDrives.insert(driveLetter);
                    
                    logger.log(L"Обработка диска: " + driveLetter);
                    
                    int removed = RemoveZoneIdentifiers(drivePath, 
                        [&, currentDrive](int p) {
                            if (progressCallback) {
                                progressCallback(((currentDrive - 1) * 100 + p) / driveCount);
                            }
                        }, 0);
                    
                    totalRemoved += removed;
                    result.details.push_back(L"Диск " + driveLetter + L": удалено " + 
                        std::to_wstring(removed) + L" меток");
                }
            }
            
            if (progressCallback) {
                progressCallback((currentDrive * 100) / driveCount);
            }
        }
    }
    
    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH, NULL, NULL);
    
    result.itemsProcessed = totalRemoved;
    result.message = "Очистка меток завершена успешно";
    logger.log(L"Завершено. Удалено меток: " + std::to_wstring(totalRemoved));
    
    return result;
}

// ==================== ГЛАВНОЕ ОКНО ПРИЛОЖЕНИЯ ====================

class USBCleanerApp {
private:
    form fm;
    group grp_usb, grp_flash;
    btn_clean_usb, btn_clean_flash, btn_exit, btn_about;
    progress prog_usb, prog_flash;
    lbl_status, lbl_title, lbl_version;
    textbox txt_log;
    listbox lst_devices;
    tooltip tip;
    timer tmr_animation;
    
    color bg_color = color(240, 245, 250);
    color accent_color = color(41, 128, 185);
    color success_color = color(39, 174, 96);
    color warning_color = color(243, 156, 18);
    color text_color = color(44, 62, 80);
    
public:
    USBCleanerApp() : fm(API::make_center(520, 580), API::window_caption("USB Cleaner")),
        grp_usb(fm, rectangle(20, 80, 480, 180)),
        grp_flash(fm, rectangle(20, 280, 480, 180)),
        btn_clean_usb(grp_usb, rectangle(150, 120, 180, 40)),
        btn_clean_flash(grp_flash, rectangle(150, 120, 180, 40)),
        btn_exit(fm, rectangle(380, 500, 110, 35)),
        btn_about(fm, rectangle(30, 500, 110, 35)),
        prog_usb(grp_usb, rectangle(30, 90, 420, 25)),
        prog_flash(grp_flash, rectangle(30, 90, 420, 25)),
        lbl_status(fm, rectangle(20, 475, 480, 25)),
        lbl_title(fm, rectangle(20, 15, 480, 50)),
        lbl_version(fm, rectangle(400, 55, 100, 20)),
        txt_log(fm, rectangle(20, 470, 480, 25)),
        lst_devices(grp_usb, rectangle(30, 30, 420, 50)),
        tip(fm),
        tmr_animation(fm, std::chrono::milliseconds(100))
    {
        InitializeUI();
        BindEvents();
    }
    
    void InitializeUI() {
        // Настройка главного окна
        fm.bgcolor(bg_color);
        fm.caption("🔌 USB Cleaner " + std::string(APP_VERSION));
        
        // Заголовок
        lbl_title.fgcolor(accent_color);
        lbl_title.textalign(align::center);
        lbl_title.typeface(font("", 18, {700}));
        lbl_title.caption("USB Cleaner - Управление устройствами");
        
        // Версия
        lbl_version.fgcolor(color(127, 140, 141));
        lbl_version.typeface(font("", 9));
        lbl_version.caption(APP_VERSION);
        
        // Группа USB
        grp_usb.caption("🔌 Очистка истории USB-устройств");
        grp_usb.fgcolor(text_color);
        grp_usb.typeface(font("", 11, {600}));
        
        btn_clean_usb.caption("🧹 Очистить историю USB");
        btn_clean_usb.bgcolor(accent_color);
        btn_clean_usb.fgcolor(colors::white);
        btn_clean_usb.typeface(font("", 11, {600}));
        btn_clean_usb.enable(true);
        tip.show(btn_clean_usb, "Удаляет записи об отключённых устройствах из реестра");
        
        prog_usb.amount(100);
        prog_usb.value(0);
        prog_usb.bgcolor(color(220, 225, 230));
        prog_usb.fgcolor(accent_color);
        
        // Группа Flash
        grp_flash.caption("💾 Очистка меток флешек");
        grp_flash.fgcolor(text_color);
        grp_flash.typeface(font("", 11, {600}));
        
        btn_clean_flash.caption("🗑️ Очистить метки Zone.Identifier");
        btn_clean_flash.bgcolor(success_color);
        btn_clean_flash.fgcolor(colors::white);
        btn_clean_flash.typeface(font("", 11, {600}));
        btn_clean_flash.enable(true);
        tip.show(btn_clean_flash, "Удаляет метки безопасности NTFS на флешках");
        
        prog_flash.amount(100);
        prog_flash.value(0);
        prog_flash.bgcolor(color(220, 225, 230));
        prog_flash.fgcolor(success_color);
        
        // Статус
        lbl_status.fgcolor(text_color);
        lbl_status.typeface(font("", 10));
        lbl_status.caption("✓ Готов к работе");
        
        // Кнопки управления
        btn_exit.caption("✖️ Выход");
        btn_exit.bgcolor(color(231, 76, 60));
        btn_exit.fgcolor(colors::white);
        btn_exit.typeface(font("", 10));
        
        btn_about.caption("ℹ️ О программе");
        btn_about.bgcolor(color(149, 165, 166));
        btn_about.fgcolor(colors::white);
        btn_about.typeface(font("", 10));
        
        // Показываем окно
        fm.show();
        fm.modality();
    }
    
    void BindEvents() {
        // Кнопка очистки USB
        btn_clean_usb.events.click([this]() {
            msgbox mb(fm, "Подтверждение", msgbox::yes_no);
            mb.icon(msgbox::icon_question);
            mb.message("Вы уверены, что хотите очистить историю USB-устройств?\n\n"
                      "Будут удалены записи об ОТКЛЮЧЁННЫХ устройствах.\n"
                      "Текущие подключения сохранятся.");
            
            if (mb() == pick_ok) {
                RunUSBCleanup();
            }
        });
        
        // Кнопка очистки меток
        btn_clean_flash.events.click([this]() {
            msgbox mb(fm, "Подтверждение", msgbox::yes_no);
            mb.icon(msgbox::icon_question);
            mb.message("Вы уверены, что хотите очистить метки на флешках?\n\n"
                      "Будут удалены:\n"
                      "- Zone.Identifier потоки\n"
                      "- Метки безопасности файлов\n"
                      "- Другие альтернативные потоки NTFS");
            
            if (mb() == pick_ok) {
                RunFlashCleanup();
            }
        });
        
        // Кнопка выхода
        btn_exit.events.click([this]() {
            API::close_window(fm);
        });
        
        // Кнопка "О программе"
        btn_about.events.click([this]() {
            ShowAboutDialog();
        });
        
        // Закрытие окна
        fm.events.destroy([this]() {
            API::exit();
        });
    }
    
    void RunUSBCleanup() {
        btn_clean_usb.enable(false);
        btn_clean_flash.enable(false);
        prog_usb.value(0);
        lbl_status.caption("⏳ Выполняется очистка истории USB...");
        lbl_status.fgcolor(warning_color);
        
        // Запуск в отдельном потоке
        std::thread([this]() {
            auto result = CleanUSBHistory([this](int progress) {
                fm.ui_thread([this, progress]() {
                    prog_usb.value(progress);
                });
            });
            
            fm.ui_thread([this, result]() {
                prog_usb.value(100);
                btn_clean_usb.enable(true);
                btn_clean_flash.enable(true);
                
                if (result.success) {
                    lbl_status.caption("✓ Успешно! Удалено записей: " + 
                        std::to_string(result.itemsProcessed));
                    lbl_status.fgcolor(success_color);
                    
                    msgbox m(fm, "Результат", msgbox::ok);
                    m.icon(msgbox::icon_information);
                    m.message("Очистка истории USB завершена!\n\n"
                             "Удалено записей: " + std::to_string(result.itemsProcessed) +
                             "\n\nДетали сохранены в логе.");
                    m();
                } else {
                    lbl_status.caption("✗ Ошибка: " + result.message);
                    lbl_status.fgcolor(color(231, 76, 60));
                }
            });
        }).detach();
    }
    
    void RunFlashCleanup() {
        btn_clean_usb.enable(false);
        btn_clean_flash.enable(false);
        prog_flash.value(0);
        lbl_status.caption("⏳ Выполняется очистка меток...");
        lbl_status.fgcolor(warning_color);
        
        std::thread([this]() {
            auto result = CleanFlashLabels([this](int progress) {
                fm.ui_thread([this, progress]() {
                    prog_flash.value(progress);
                });
            });
            
            fm.ui_thread([this, result]() {
                prog_flash.value(100);
                btn_clean_usb.enable(true);
                btn_clean_flash.enable(true);
                
                if (result.success) {
                    lbl_status.caption("✓ Успешно! Удалено меток: " + 
                        std::to_string(result.itemsProcessed));
                    lbl_status.fgcolor(success_color);
                    
                    msgbox m(fm, "Результат", msgbox::ok);
                    m.icon(msgbox::icon_information);
                    m.message("Очистка меток завершена!\n\n"
                             "Удалено меток: " + std::to_string(result.itemsProcessed) +
                             "\n\nРекомендуется перезагрузить проводник.");
                    m();
                } else {
                    lbl_status.caption("✗ Ошибка: " + result.message);
                    lbl_status.fgcolor(color(231, 76, 60));
                }
            });
        }).detach();
    }
    
    void ShowAboutDialog() {
        form about_fm(API::make_center(400, 350), API::window_caption("О программе"));
        about_fm.bgcolor(bg_color);
        
        label title(about_fm, rectangle(10, 20, 380, 40));
        title.fgcolor(accent_color);
        title.textalign(align::center);
        title.typeface(font("", 20, {700}));
        title.caption("USB Cleaner");
        
        label version(about_fm, rectangle(10, 65, 380, 25));
        version.fgcolor(color(127, 140, 141));
        version.textalign(align::center);
        version.caption(std::string("Версия ") + APP_VERSION);
        
        label desc(about_fm, rectangle(20, 100, 360, 150));
        desc.fgcolor(text_color);
        desc.textalign(align::left);
        desc.typeface(font("", 10));
        desc.caption(
            "Программа для очистки истории подключённых\n"
            "USB-устройств и меток безопасности на флешках.\n\n"
            "Функции:\n"
            "• Очистка реестра от записей об отключённых USB\n"
            "• Удаление Zone.Identifier потоков NTFS\n"
            "• Сохранение данных о текущих подключениях\n\n"
            "Библиотека: Nana C++ GUI\n"
            "Совместимость: Windows 7+"
        );
        
        label copyright(about_fm, rectangle(10, 270, 380, 25));
        copyright.fgcolor(color(127, 140, 141));
        copyright.textalign(align::center);
        copyright.caption("(c) 2026 USB Cleaner - MIT License");
        
        button btn_ok(about_fm, rectangle(140, 305, 120, 35));
        btn_ok.caption("OK");
        btn_ok.bgcolor(accent_color);
        btn_ok.fgcolor(colors::white);
        
        btn_ok.events.click([&about_fm]() {
            API::close_window(about_fm);
        });
        
        about_fm.modality();
    }
    
    void Run() {
        exec();
    }
};

// ==================== SYSTRAY ИНТЕГРАЦИЯ (опционально) ====================

class SystemTrayIcon {
private:
    NOTIFYICONDATA nid = {};
    HWND hWnd = NULL;
    
public:
    bool Create(HWND parentWnd, const wchar_t* tooltip, HICON icon) {
        hWnd = parentWnd;
        
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = hWnd;
        nid.uID = 1;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_APP + 1;
        nid.hIcon = icon ? icon : LoadIcon(NULL, IDI_WINLOGO);
        wcscpy_s(nid.szTip, tooltip);
        
        return Shell_NotifyIcon(NIM_ADD, &nid) != FALSE;
    }
    
    void Remove() {
        Shell_NotifyIcon(NIM_DELETE, &nid);
    }
    
    void ShowBalloon(const wchar_t* title, const wchar_t* text, int timeout = 3000) {
        wcscpy_s(nid.szInfoTitle, title);
        wcscpy_s(nid.szInfo, text);
        nid.uTimeout = timeout;
        nid.uFlags |= NIF_INFO;
        nid.dwInfoFlags = NIIF_INFO;
        Shell_NotifyIcon(NIM_MODIFY, &nid);
    }
    
    void SetTooltip(const wchar_t* tooltip) {
        wcscpy_s(nid.szTip, tooltip);
        Shell_NotifyIcon(NIM_MODIFY, &nid);
    }
};

// ==================== TOCHKA ВХОДА ====================

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Инициализация Nana
    constexpr int ARGV = 0;
    char* argv[] = {nullptr};
    
    // Проверка прав администратора
    if (!IsAdministrator()) {
        // Показываем диалог с запросом прав
        int choice = MessageBoxW(NULL, 
            L"Программа требует прав администратора для работы с реестром.\n\n"
            L"Хотите перезапустить с правами администратора?",
            L"USB Cleaner - Требуются права",
            MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);
        
        if (choice == IDYES) {
            RestartAsAdmin();
        }
        return 0;
    }
    
    logger.log(L"Программа запущена (режим администратора)");
    
    try {
        // Создаём и запускаем приложение
        USBCleanerApp app;
        app.Run();
    } catch (const std::exception& e) {
        logger.log(L"Критическая ошибка: " + std::wstring(e.what(), e.what() + strlen(e.what())));
        MessageBoxA(NULL, e.what(), "Critical Error", MB_ICONERROR);
        return 1;
    }
    
    logger.log(L"Программа закрыта");
    
    return 0;
}
