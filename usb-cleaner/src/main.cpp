/*
 * USB Cleaner - Tray Application
 * Очистка истории USB-устройств и меток флешек
 * 
 * Компиляция (MinGW-w64):
 * g++ -o usb_cleaner.exe main.cpp -lcomctl32 -lshell32 -lole32 -luuid -mwindows -static
 * 
 * Компиляция (MSVC):
 * cl /EHsc /Fe:usb_cleaner.exe main.cpp comctl32.lib shell32.lib ole32.lib uuid.lib /link /SUBSYSTEM:WINDOWS
 */

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0601  // Windows 7+

#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <shlobj.h>

// Глобальные переменные
HINSTANCE g_hInstance = NULL;
HWND g_hWnd = NULL;
NOTIFYICONDATA g_nid = {};
HMENU g_hMenu = NULL;
UINT g_uTaskbarCreated = 0;

// Идентификаторы меню
#define IDM_CLEAN_USB_HISTORY   1001
#define IDM_CLEAN_FLASH_LABELS  1002
#define IDM_SEPARATOR1          1003
#define IDM_ABOUT               1004
#define IDM_EXIT                1005

// Константы для реестра USB-устройств
const wchar_t* USB_STOR_KEY = L"SYSTEM\\CurrentControlSet\\Enum\\USBSTOR";
const wchar_t* USB_KEY = L"SYSTEM\\CurrentControlSet\\Enum\\USB";
const wchar_t* MOUNTED_DEVICES_KEY = L"SYSTEM\\MountedDevices";

// Структура для информации об USB-устройстве
struct USBDeviceInfo {
    std::wstring deviceName;
    std::wstring devicePath;
    bool isConnected;
};

// Логирование в файл
void LogMessage(const std::wstring& message) {
    std::wofstream logFile(L"usb_cleaner.log", std::ios::app);
    if (logFile.is_open()) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        logFile << L"[" << st.wYear << L"-" << st.wMonth << L"-" << st.wDay 
                << L" " << st.wHour << L":" << st.wMinute << L":" << st.wSecond 
                << L"] " << message << std::endl;
        logFile.close();
    }
}

// Получить список текущих подключенных USB-накопителей
std::set<std::wstring> GetCurrentUSBDrives() {
    std::set<std::wstring> drives;
    DWORD drivesMask = GetLogicalDrives();
    
    for (char drive = 'A'; drive <= 'Z'; drive++) {
        if (drivesMask & (1 << (drive - 'A'))) {
            std::wstring path = std::wstring(1, drive) + L":\\";
            UINT type = GetDriveType(path.c_str());
            
            if (type == DRIVE_REMOVABLE || type == DRIVE_FIXED) {
                // Проверяем, является ли устройство USB
                wchar_t volumeName[MAX_PATH];
                if (GetVolumeInformation(path.c_str(), volumeName, MAX_PATH, 
                    NULL, NULL, NULL, NULL, 0)) {
                    drives.insert(std::wstring(1, drive) + L":");
                }
            }
        }
    }
    
    return drives;
}

// Проверить, подключено ли USB-устройство по hardware ID
bool IsUSBDeviceConnected(const std::wstring& hardwareId) {
    // Получаем информацию о текущих устройствах через SetupAPI
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

// Рекурсивное удаление ключа реестра со всеми подклюами
bool DeleteRegistryKeyRecursive(HKEY hRootKey, const std::wstring& subKey) {
    HKEY hKey;
    LONG result = RegOpenKeyExW(hRootKey, subKey.c_str(), 0, KEY_READ | KEY_WRITE, &hKey);
    
    if (result != ERROR_SUCCESS) return false;
    
    // Сначала удаляем все подключи
    wchar_t className[256];
    DWORD classNameSize = 256;
    DWORD subKeysCount = 0;
    DWORD maxSubKeyLen = 0;
    DWORD maxClassLen = 0;
    DWORD valuesCount = 0;
    DWORD maxValueLen = 0;
    DWORD maxValueDataLen = 0;
    DWORD securityDescriptorLen;
    FILETIME lastWriteTime;
    
    result = RegQueryInfoKeyW(hKey, className, &classNameSize, NULL, &subKeysCount,
        &maxSubKeyLen, &maxClassLen, &valuesCount, &maxValueLen, &maxValueDataLen,
        &securityDescriptorLen, &lastWriteTime);
    
    if (result == ERROR_SUCCESS && subKeysCount > 0) {
        std::vector<wchar_t> subKeyName(maxSubKeyLen + 1);
        
        for (DWORD i = subKeysCount - 1; i < subKeysCount; i--) {
            DWORD subKeyNameSize = maxSubKeyLen + 1;
            result = RegEnumKeyExW(hKey, i, subKeyName.data(), &subKeyNameSize,
                NULL, NULL, NULL, &lastWriteTime);
            
            if (result == ERROR_SUCCESS) {
                std::wstring childPath = subKey + L"\\" + subKeyName.data();
                DeleteRegistryKeyRecursive(hRootKey, childPath);
            }
        }
    }
    
    RegCloseKey(hKey);
    
    // Теперь удаляем сам ключ
    result = RegDeleteKeyExW(hRootKey, subKey.c_str(), KEY_WOW64_64KEY, 0);
    return (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND);
}

// Очистка истории USB-устройств из реестра
int CleanUSBHistory() {
    int deletedCount = 0;
    std::vector<USBDeviceInfo> allDevices;
    
    LogMessage(L"Начало очистки истории USB-устройств...");
    
    // Открываем ключ USBSTOR
    HKEY hUsbStorKey;
    LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, USB_STOR_KEY, 0, 
        KEY_READ | KEY_WRITE | KEY_WOW64_64KEY, &hUsbStorKey);
    
    if (result != ERROR_SUCCESS) {
        LogMessage(L"Ошибка открытия ключа USBSTOR: " + std::to_wstring(result));
        MessageBoxW(g_hWnd, 
            L"Не удалось открыть реестр.\nЗапустите программу от имени администратора!", 
            L"Ошибка", MB_ICONERROR | MB_OK);
        return -1;
    }
    
    // Перечисляем все устройства в USBSTOR
    DWORD subKeysCount = 0;
    DWORD maxSubKeyLen = 0;
    RegQueryInfoKeyW(hUsbStorKey, NULL, NULL, NULL, &subKeysCount,
        &maxSubKeyLen, NULL, NULL, NULL, NULL, NULL, NULL);
    
    if (subKeysCount > 0) {
        std::vector<wchar_t> deviceClassName(maxSubKeyLen + 1);
        
        for (DWORD i = 0; i < subKeysCount; i++) {
            DWORD nameSize = maxSubKeyLen + 1;
            FILETIME lastWriteTime;
            
            if (RegEnumKeyExW(hUsbStorKey, i, deviceClassName.data(), &nameSize,
                NULL, NULL, NULL, &lastWriteTime) == ERROR_SUCCESS) {
                
                std::wstring classPath = std::wstring(USB_STOR_KEY) + L"\\" + deviceClassName.data();
                
                // Открываем класс устройства
                HKEY hClassKey;
                if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, classPath.c_str(), 0,
                    KEY_READ | KEY_WRITE | KEY_WOW64_64KEY, &hClassKey) == ERROR_SUCCESS) {
                    
                    DWORD instanceCount = 0;
                    DWORD maxInstanceLen = 0;
                    RegQueryInfoKeyW(hClassKey, NULL, NULL, NULL, &instanceCount,
                        &maxInstanceLen, NULL, NULL, NULL, NULL, NULL, NULL);
                    
                    std::vector<wchar_t> instanceName(maxInstanceLen + 1);
                    
                    for (DWORD j = 0; j < instanceCount; j++) {
                        DWORD instNameSize = maxInstanceLen + 1;
                        
                        if (RegEnumKeyExW(hClassKey, j, instanceName.data(), &instNameSize,
                            NULL, NULL, NULL, &lastWriteTime) == ERROR_SUCCESS) {
                            
                            std::wstring fullDeviceId = deviceClassName.data();
                            fullDeviceId += L"&";
                            fullDeviceId += instanceName.data();
                            
                            // Проверяем, подключено ли устройство сейчас
                            if (!IsUSBDeviceConnected(fullDeviceId)) {
                                // Устройство не подключено - удаляем запись
                                std::wstring instancePath = classPath + L"\\" + instanceName.data();
                                
                                LogMessage(L"Удаление записи отключенного устройства: " + fullDeviceId);
                                
                                if (DeleteRegistryKeyRecursive(HKEY_LOCAL_MACHINE, instancePath)) {
                                    deletedCount++;
                                } else {
                                    LogMessage(L"Не удалось удалить: " + instancePath);
                                }
                            } else {
                                LogMessage(L"Пропуск подключенного устройства: " + fullDeviceId);
                            }
                        }
                    }
                    
                    RegCloseKey(hClassKey);
                }
            }
        }
    }
    
    RegCloseKey(hUsbStorKey);
    
    // Также очищаем MountedDevices
    HKEY hMountedKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, MOUNTED_DEVICES_KEY, 0,
        KEY_READ | KEY_WRITE | KEY_WOW64_64KEY, &hMountedKey) == ERROR_SUCCESS) {
        
        DWORD valuesCount = 0;
        DWORD maxValueNameLen = 0;
        
        RegQueryInfoKeyW(hMountedKey, NULL, NULL, NULL, NULL, NULL, NULL,
            &valuesCount, &maxValueNameLen, NULL, NULL, NULL);
        
        if (valuesCount > 0) {
            std::vector<wchar_t> valueName(maxValueNameLen + 1);
            
            for (DWORD i = 0; i < valuesCount; i++) {
                DWORD valueNameSize = maxValueNameLen + 1;
                DWORD valueType = 0;
                
                if (RegEnumValueW(hMountedKey, i, valueName.data(), &valueNameSize,
                    NULL, &valueType, NULL, NULL) == ERROR_SUCCESS) {
                    
                    std::wstring valName(valueName.data());
                    
                    // Удаляем записи о несуществующих устройствах
                    if (valName.find(L"\\DosDevices\\") != std::wstring::npos ||
                        valName.find(L"\\??\\") != std::wstring::npos) {
                        
                        // Проверяем существование диска
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
                            // Если диск не существует или не USB - удаляем запись
                            UINT type = GetDriveType(driveLetter.c_str());
                            if (type == DRIVE_NO_ROOT_DIR || type == DRIVE_UNKNOWN) {
                                RegDeleteValueW(hMountedKey, valName.c_str());
                                deletedCount++;
                                LogMessage(L"Удалена запись MountedDevices: " + valName);
                            }
                        }
                    }
                }
            }
        }
        
        RegCloseKey(hMountedKey);
    }
    
    LogMessage(L"Очистка завершено. Удалено записей: " + std::to_wstring(deletedCount));
    return deletedCount;
}

// Рекурсивный поиск файлов с Zone.Identifier и их удаление
int RemoveZoneIdentifiers(const std::wstring& rootPath) {
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
            // Рекурсивно обходим папки
            fullPath += L"\\";
            removedCount += RemoveZoneIdentifiers(fullPath);
        } else {
            // Пытаемся удалить Zone.Identifier
            std::wstring zoneFile = fullPath + L":Zone.Identifier";
            DWORD attrs = GetFileAttributesW(zoneFile.c_str());
            
            if (attrs != INVALID_FILE_ATTRIBUTES) {
                // Сбрасываем атрибуты только для чтения
                SetFileAttributesW(zoneFile.c_str(), FILE_ATTRIBUTE_NORMAL);
                
                if (DeleteFileW(zoneFile.c_str())) {
                    removedCount++;
                    LogMessage(L"Удален Zone.Identifier: " + fullPath);
                }
            }
        }
        
    } while (FindNextFileW(hFind, &findData));
    
    FindClose(hFind);
    return removedCount;
}

// Очистка меток на всех съемных дисках
int CleanFlashLabels() {
    int totalRemoved = 0;
    std::set<std::wstring> processedDrives;
    
    LogMessage(L"Начало очистки меток флешек...");
    
    DWORD drivesMask = GetLogicalDrives();
    
    for (char drive = 'A'; drive <= 'Z'; drive++) {
        if (!(drivesMask & (1 << (drive - 'A')))) continue;
        
        std::wstring drivePath = std::wstring(1, drive) + L":\\";
        UINT type = GetDriveType(drivePath.c_str());
        
        // Обрабатываем только съемные диски и фиксированные (внешние HDD)
        if (type == DRIVE_REMOVABLE || type == DRIVE_FIXED) {
            // Дополнительная проверка на USB
            wchar_t fileSystem[MAX_PATH];
            if (GetVolumeInformationW(drivePath.c_str(), NULL, 0, NULL, NULL, NULL, 
                fileSystem, MAX_PATH)) {
                
                std::wstring driveLetter(1, drive);
                driveLetter += L":";
                
                if (processedDrives.find(driveLetter) == processedDrives.end()) {
                    processedDrives.insert(driveLetter);
                    
                    LogMessage(L"Обработка диска: " + driveLetter);
                    int removed = RemoveZoneIdentifiers(drivePath);
                    totalRemoved += removed;
                    
                    // Также удаляем desktop.ini с метаданными
                    std::wstring desktopIni = drivePath + L"desktop.ini";
                    if (GetFileAttributesW(desktopIni.c_str()) != INVALID_FILE_ATTRIBUTES) {
                        SetFileAttributesW(desktopIni.c_str(), FILE_ATTRIBUTE_NORMAL);
                        // Не удаляем desktop.ini, но можно очистить его содержимое
                    }
                }
            }
        }
    }
    
    // Очищаем кэш иконок для флешек
    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH, NULL, NULL);
    
    LogMessage(L"Очистка меток завершено. Удалено меток: " + std::to_wstring(totalRemoved));
    return totalRemoved;
}

// Создание иконки в трее
BOOL CreateTrayIcon() {
    ZeroMemory(&g_nid, sizeof(NOTIFYICONDATA));
    g_nid.cbSize = sizeof(NOTIFYICONDATA);
    g_nid.hWnd = g_hWnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
    g_nid.uCallbackMessage = WM_APP + 1;
    g_nid.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wcscpy_s(g_nid.szTip, L"USB Cleaner - Очистка истории устройств");
    
    return Shell_NotifyIcon(NIM_ADD, &g_nid);
}

// Удаление иконки из трея
VOID RemoveTrayIcon() {
    Shell_NotifyIcon(NIM_DELETE, &g_nid);
}

// Показать контекстное меню
VOID ShowTrayMenu(HWND hWnd) {
    POINT pt;
    GetCursorPos(&pt);
    
    // Создаем меню
    HMENU hMenu = CreatePopupMenu();
    
    AppendMenuW(hMenu, MF_STRING, IDM_CLEAN_USB_HISTORY, 
        L"🔌 Очистить историю USB-устройств\n   (кроме подключённых)");
    AppendMenuW(hMenu, MF_STRING, IDM_CLEAN_FLASH_LABELS, 
        L"💾 Очистить метки флешек\n   (Zone.Identifier)");
    AppendMenuW(hMenu, MF_SEPARATOR, IDM_SEPARATOR1, NULL);
    AppendMenuW(hMenu, MF_STRING, IDM_ABOUT, L"ℹ️ О программе");
    AppendMenuW(hMenu, MF_STRING, IDM_EXIT, L"✖️ Выход");
    
    // Делаем окно передним
    SetForegroundWindow(hWnd);
    
    // Показываем меню
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
    PostMessage(hWnd, WM_NULL, 0, 0);
    
    DestroyMenu(hMenu);
}

// Обработка командов меню
VOID HandleMenuCommand(WORD commandId) {
    switch (commandId) {
        case IDM_CLEAN_USB_HISTORY: {
            int result = MessageBoxW(g_hWnd,
                L"Вы уверены, что хотите очистить историю подключённых USB-устройств?\n\n"
                L"Будут удалены записи об ОТКЛЮЧЁННЫХ устройствах из реестра.\n"
                L"Текущие подключения сохранятся.",
                L"Подтверждение",
                MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
            
            if (result == IDYES) {
                // Запускаем очистку в отдельном потоке
                CreateThread(NULL, 0, [](LPVOID) -> DWORD {
                    int deleted = CleanUSBHistory();
                    
                    // Показываем уведомление
                    COPYDATASTRUCT cds = {0};
                    std::wstring msg = L"Очистка завершена!\nУдалено записей: " + std::to_wstring(deleted);
                    cds.dwData = 1;
                    cds.lpData = (void*)msg.c_str();
                    cds.cbData = (msg.size() + 1) * sizeof(wchar_t);
                    SendMessage(g_hWnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
                    
                    return 0;
                }, NULL, 0, NULL);
                
                // Временное уведомление
                wcscpy_s(g_nid.szInfoTitle, L"USB Cleaner");
                wcscpy_s(g_nid.szInfo, L"Выполняется очистка истории...");
                g_nid.uTimeout = 3000;
                g_nid.dwInfoFlags = NIIF_INFO;
                Shell_NotifyIcon(NIM_MODIFY, &g_nid);
            }
            break;
        }
        
        case IDM_CLEAN_FLASH_LABELS: {
            int result = MessageBoxW(g_hWnd,
                L"Вы уверены, что хотите очистить метки на флешках?\n\n"
                L"Будут удалены:\n"
                L"- Zone.Identifier потоки (метки безопасности)\n"
                L"- Метки \"Эта программа загружена из Интернета\"\n"
                L"- Другие альтернативные потоки данных NTFS",
                L"Подтверждение",
                MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
            
            if (result == IDYES) {
                CreateThread(NULL, 0, [](LPVOID) -> DWORD {
                    int removed = CleanFlashLabels();
                    
                    std::wstring msg = L"Очистка меток завершена!\nУдалено меток: " + std::to_wstring(removed);
                    COPYDATASTRUCT cds = {0};
                    cds.dwData = 2;
                    cds.lpData = (void*)msg.c_str();
                    cds.cbData = (msg.size() + 1) * sizeof(wchar_t);
                    SendMessage(g_hWnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
                    
                    return 0;
                }, NULL, 0, NULL);
                
                wcscpy_s(g_nid.szInfoTitle, L"USB Cleaner");
                wcscpy_s(g_nid.szInfo, L"Выполняется очистка меток...");
                g_nid.uTimeout = 3000;
                g_nid.dwInfoFlags = NIIF_INFO;
                Shell_NotifyIcon(NIM_MODIFY, &g_nid);
            }
            break;
        }
        
        case IDM_ABOUT:
            MessageBoxW(g_hWnd,
                L"USB Cleaner v1.0\n\n"
                L"Программа для очистки истории USB-устройств\n"
                L"и меток безопасности на флешках.\n\n"
                L"Функции:\n"
                L"• Очистка реестра от записей об отключённых USB\n"
                L"• Удаление Zone.Identifier потоков\n"
                L"• Сохранение данных о текущих подключениях\n\n"
                L"⚠️ Требуются права администратора!\n\n"
                L"(c) 2026 USB Cleaner",
                L"О программе", MB_ICONINFORMATION | MB_OK);
            break;
            
        case IDM_EXIT:
            PostQuitMessage(0);
            break;
    }
}

// Процедура окна (скрытое, только для сообщений)
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            g_uTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
            CreateTrayIcon();
            break;
            
        case WM_APP + 1:  // Сообщение от иконки в трее
            switch (LOWORD(lParam)) {
                case WM_RBUTTONUP:
                case WM_CONTEXTMENU:
                    ShowTrayMenu(hWnd);
                    break;
                    
                case WM_LBUTTONDBLCLK:
                    ShowTrayMenu(hWnd);
                    break;
            }
            break;
            
        case WM_COMMAND:
            HandleMenuCommand(LOWORD(wParam));
            break;
            
        case WM_COPYDATA: {
            PCOPYDATASTRUCT pCds = (PCOPYDATASTRUCT)lParam;
            if (pCds && pCds->lpData) {
                std::wstring* msg = (std::wstring*)pCds->lpData;
                wcscpy_s(g_nid.szInfoTitle, L"USB Cleaner");
                wcsncpy_s(g_nid.szInfo, (const wchar_t*)pCds->lpData, _countof(g_nid.szInfo) - 1);
                g_nid.uTimeout = 5000;
                g_nid.dwInfoFlags = NIIF_INFO;
                Shell_NotifyIcon(NIM_MODIFY, &g_nid);
            }
            break;
        }
        
        default:
            if (message == g_uTaskbarCreated) {
                // Восстановление иконки после перезапуска проводника
                CreateTrayIcon();
            }
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Точка входа
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    g_hInstance = hInstance;
    
    // Проверка прав администратора
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
    
    if (!isAdmin) {
        // Попытка перезапуска с правами администратора
        SHELLEXECUTEINFOW sei = {0};
        sei.cbSize = sizeof(SHELLEXECUTEINFOW);
        sei.lpVerb = L"runas";
        sei.lpFile = L"usb_cleaner.exe";
        sei.nShow = SW_SHOWNORMAL;
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        
        if (!ShellExecuteExW(&sei)) {
            MessageBoxW(NULL, 
                L"Программа требует прав администратора!\n"
                L"Пожалуйста, запустите программу от имени администратора.", 
                L"Ошибка прав", MB_ICONERROR | MB_OK);
            return 1;
        }
        return 0;
    }
    
    // Регистрация класса окна
    WNDCLASSEXW wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName = L"USBCleanerHiddenWindow";
    
    if (!RegisterClassExW(&wcex)) {
        MessageBoxW(NULL, L"Ошибка регистрации окна!", L"Ошибка", MB_ICONERROR);
        return 1;
    }
    
    // Создание скрытого окна
    g_hWnd = CreateWindowExW(
        0,
        L"USBCleanerHiddenWindow",
        L"USB Cleaner",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL
    );
    
    if (!g_hWnd) {
        MessageBoxW(NULL, L"Ошибка создания окна!", L"Ошибка", MB_ICONERROR);
        return 1;
    }
    
    // Инициализация common controls
    INITCOMMONCONTROLSEX icc = {0};
    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);
    
    LogMessage(L"Программа запущена успешно.");
    
    // Цикл обработки сообщений
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Cleanup
    RemoveTrayIcon();
    LogMessage(L"Программа закрыта.");
    
    return (int)msg.wParam;
}
