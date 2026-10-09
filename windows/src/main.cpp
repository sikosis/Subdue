#include <windows.h>
#include <commctrl.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>
#include <shellapi.h>
#include <tlhelp32.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <cwchar>
#include <cwctype>
#include <iterator>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "resource.h"

namespace {

constexpr wchar_t kWindowClass[] = L"SubdueMainWindow";
constexpr wchar_t kWindowTitle[] = L"Subdue";
constexpr UINT kTrayCallback = WM_APP + 1;
constexpr UINT_PTR kTickTimer = 1;
constexpr UINT_PTR kHideTimer = 2;
constexpr UINT kTrayId = 1;

enum ControlId {
    IDC_ACTION = 1001,
    IDC_APP,
    IDC_REFRESH,
    IDC_TIMING,
    IDC_VALUE,
    IDC_FORCE,
    IDC_SCHEDULE,
    IDC_STATUS
};

enum TrayCommand {
    IDM_SHOW = 2001,
    IDM_CANCEL,
    IDM_EXIT
};

struct AppTarget {
    DWORD processId{};
    std::wstring name;
    std::wstring path;
};

enum class TimingMode { after, at, topOfHour };
enum class ActionMode { subdue, smother };

struct Options {
    std::wstring app;
    bool mute = false;
    std::optional<TimingMode> timing;
    int minutes{};
    int hour{};
    int minute{};
    bool force = true;
    bool foreground = false;
    bool help = false;
    std::wstring error;
};

struct Schedule {
    ActionMode action = ActionMode::subdue;
    AppTarget target;
    std::chrono::system_clock::time_point fireTime;
    bool force = true;
    bool gracefulSent = false;
    std::chrono::steady_clock::time_point forceDeadline;
    HANDLE process = nullptr;
};

HINSTANCE gInstance{};
HWND gWindow{};
HWND gActionCombo{};
HWND gAppLabel{};
HWND gAppCombo{};
HWND gRefreshButton{};
HWND gTimingCombo{};
HWND gValueEdit{};
HWND gForceCheck{};
HWND gStatus{};
HWND gScheduleButton{};
HFONT gFont{};
HICON gIcon{};
NOTIFYICONDATAW gTray{};
UINT gTaskbarCreated{};
std::vector<AppTarget> gApps;
std::optional<Schedule> gSchedule;
bool gExitRequested = false;

std::wstring lower(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t c) {
        return static_cast<wchar_t>(std::towlower(c));
    });
    return value;
}

std::wstring fileNameWithoutExtension(const std::wstring& path) {
    auto slash = path.find_last_of(L"\\/");
    auto name = slash == std::wstring::npos ? path : path.substr(slash + 1);
    auto dot = name.find_last_of(L'.');
    return dot == std::wstring::npos ? name : name.substr(0, dot);
}

std::wstring processPath(DWORD pid) {
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) return {};
    wchar_t path[32768]{};
    DWORD length = static_cast<DWORD>(std::size(path));
    const BOOL ok = QueryFullProcessImageNameW(process, 0, path, &length);
    CloseHandle(process);
    return ok ? std::wstring(path, length) : std::wstring();
}

BOOL CALLBACK collectWindow(HWND window, LPARAM parameter) {
    if (!IsWindowVisible(window) || GetWindow(window, GW_OWNER) != nullptr) return TRUE;
    DWORD pid{};
    GetWindowThreadProcessId(window, &pid);
    if (!pid || pid == GetCurrentProcessId()) return TRUE;
    auto* targets = reinterpret_cast<std::unordered_map<DWORD, AppTarget>*>(parameter);
    if (targets->contains(pid)) return TRUE;

    const auto path = processPath(pid);
    if (path.empty()) return TRUE;
    wchar_t title[512]{};
    GetWindowTextW(window, title, static_cast<int>(std::size(title)));
    auto name = fileNameWithoutExtension(path);
    if (name.empty()) name = title;
    targets->emplace(pid, AppTarget{pid, std::move(name), path});
    return TRUE;
}

std::vector<AppTarget> runningWindowApps() {
    std::unordered_map<DWORD, AppTarget> targets;
    EnumWindows(collectWindow, reinterpret_cast<LPARAM>(&targets));
    std::vector<AppTarget> result;
    result.reserve(targets.size());
    for (auto& [_, target] : targets) result.push_back(std::move(target));
    std::sort(result.begin(), result.end(), [](const auto& lhs, const auto& rhs) {
        return lower(lhs.name) < lower(rhs.name);
    });
    return result;
}

std::optional<AppTarget> findProcess(const std::wstring& requested) {
    if (requested.empty()) return std::nullopt;
    const auto needle = lower(requested);
    const auto requestedName = lower(fileNameWithoutExtension(requested));
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return std::nullopt;

    PROCESSENTRY32W entry{sizeof(entry)};
    std::optional<AppTarget> found;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (entry.th32ProcessID == GetCurrentProcessId()) continue;
            const auto path = processPath(entry.th32ProcessID);
            const auto exeName = lower(fileNameWithoutExtension(entry.szExeFile));
            if (exeName == requestedName || lower(entry.szExeFile) == needle || (!path.empty() && lower(path) == needle)) {
                found = AppTarget{entry.th32ProcessID, fileNameWithoutExtension(entry.szExeFile), path};
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return found;
}

void applyFont(HWND control) {
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(gFont), TRUE);
}

HWND makeControl(const wchar_t* klass, const wchar_t* text, DWORD style,
                 int x, int y, int width, int height, int id = 0) {
    HWND control = CreateWindowExW(0, klass, text, WS_CHILD | WS_VISIBLE | style,
                                   x, y, width, height, gWindow,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), gInstance, nullptr);
    applyFont(control);
    return control;
}

void setStatus(const std::wstring& text) {
    SetWindowTextW(gStatus, text.c_str());
}

std::wstring scheduleName(const Schedule& schedule) {
    return schedule.action == ActionMode::smother ? L"System audio" : schedule.target.name;
}

void populateApps() {
    const int previous = static_cast<int>(SendMessageW(gAppCombo, CB_GETCURSEL, 0, 0));
    DWORD previousPid = previous >= 0 && previous < static_cast<int>(gApps.size()) ? gApps[previous].processId : 0;
    gApps = runningWindowApps();
    SendMessageW(gAppCombo, CB_RESETCONTENT, 0, 0);
    int selection = -1;
    for (size_t index = 0; index < gApps.size(); ++index) {
        SendMessageW(gAppCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(gApps[index].name.c_str()));
        if (gApps[index].processId == previousPid) selection = static_cast<int>(index);
    }
    if (selection >= 0) SendMessageW(gAppCombo, CB_SETCURSEL, selection, 0);
}

std::wstring countdownText(std::chrono::system_clock::time_point fireTime) {
    auto remaining = std::chrono::duration_cast<std::chrono::seconds>(fireTime - std::chrono::system_clock::now()).count();
    if (remaining < 0) remaining = 0;
    const auto hours = remaining / 3600;
    const auto minutes = (remaining % 3600) / 60;
    const auto seconds = remaining % 60;
    wchar_t buffer[32]{};
    if (hours > 0) swprintf_s(buffer, L"%lld:%02lld:%02lld", hours, minutes, seconds);
    else swprintf_s(buffer, L"%02lld:%02lld", minutes, seconds);
    return buffer;
}

void installTrayIcon() {
    gTray = {};
    gTray.cbSize = sizeof(gTray);
    gTray.hWnd = gWindow;
    gTray.uID = kTrayId;
    gTray.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    gTray.uCallbackMessage = kTrayCallback;
    gTray.hIcon = gIcon;
    wcscpy_s(gTray.szTip, L"Subdue");
    Shell_NotifyIconW(NIM_ADD, &gTray);
    gTray.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &gTray);
}

void updateTray() {
    std::wstring tip = L"Subdue";
    if (gSchedule) tip += L" — " + scheduleName(*gSchedule) + L" — " + countdownText(gSchedule->fireTime);
    gTray.uFlags = NIF_TIP | NIF_ICON;
    gTray.hIcon = gIcon;
    wcsncpy_s(gTray.szTip, tip.c_str(), _TRUNCATE);
    Shell_NotifyIconW(NIM_MODIFY, &gTray);
}

void notify(const std::wstring& title, const std::wstring& message) {
    gTray.uFlags = NIF_INFO;
    wcsncpy_s(gTray.szInfoTitle, title.c_str(), _TRUNCATE);
    wcsncpy_s(gTray.szInfo, message.c_str(), _TRUNCATE);
    gTray.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIconW(NIM_MODIFY, &gTray);
}

void showMainWindow() {
    ShowWindow(gWindow, SW_SHOWNORMAL);
    SetForegroundWindow(gWindow);
}

BOOL CALLBACK closeTargetWindow(HWND window, LPARAM pidValue) {
    DWORD pid{};
    GetWindowThreadProcessId(window, &pid);
    if (pid == static_cast<DWORD>(pidValue)) PostMessageW(window, WM_CLOSE, 0, 0);
    return TRUE;
}

void clearSchedule() {
    if (gSchedule && gSchedule->process) CloseHandle(gSchedule->process);
    gSchedule.reset();
    updateTray();
}

void cancelSchedule() {
    KillTimer(gWindow, kHideTimer);
    clearSchedule();
    setStatus(L"Schedule cancelled.");
}

void hideWindowAfterConfirmation() {
    SetTimer(gWindow, kHideTimer, 5000, nullptr);
}

void completeSchedule(const std::wstring& message) {
    setStatus(message);
    notify(L"Subdue", message);
    clearSchedule();
}

std::optional<std::chrono::system_clock::time_point> calculateFireTime(TimingMode mode, int first, int second = 0) {
    const auto now = std::chrono::system_clock::now();
    if (mode == TimingMode::after) {
        if (first <= 0) return std::nullopt;
        return now + std::chrono::minutes(first);
    }
    const std::time_t current = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
    localtime_s(&local, &current);
    if (mode == TimingMode::topOfHour) {
        local.tm_hour += 1;
        local.tm_min = 0;
        local.tm_sec = 0;
    } else {
        if (first < 0 || first > 23 || second < 0 || second > 59) return std::nullopt;
        local.tm_hour = first;
        local.tm_min = second;
        local.tm_sec = 0;
        auto candidate = std::chrono::system_clock::from_time_t(std::mktime(&local));
        if (candidate <= now) local.tm_mday += 1;
    }
    return std::chrono::system_clock::from_time_t(std::mktime(&local));
}

bool muteDefaultAudio() {
    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDevice* device = nullptr;
    IAudioEndpointVolume* endpointVolume = nullptr;

    HRESULT result = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                      IID_PPV_ARGS(&enumerator));
    if (SUCCEEDED(result)) result = enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &device);
    if (SUCCEEDED(result)) {
        result = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr,
                                  reinterpret_cast<void**>(&endpointVolume));
    }
    if (SUCCEEDED(result)) result = endpointVolume->SetMute(TRUE, nullptr);

    if (endpointVolume) endpointVolume->Release();
    if (device) device->Release();
    if (enumerator) enumerator->Release();
    return SUCCEEDED(result);
}

bool armSubdueSchedule(const AppTarget& target, std::chrono::system_clock::time_point fireTime, bool force) {
    HANDLE process = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, target.processId);
    if (!process) {
        setStatus(L"Unable to access that application. It may require administrator privileges.");
        return false;
    }
    clearSchedule();
    gSchedule = Schedule{ActionMode::subdue, target, fireTime, force, false, {}, process};
    setStatus(target.name + L" is scheduled. Remaining: " + countdownText(fireTime));
    updateTray();
    hideWindowAfterConfirmation();
    return true;
}

bool armSmotherSchedule(std::chrono::system_clock::time_point fireTime) {
    clearSchedule();
    gSchedule = Schedule{ActionMode::smother, {}, fireTime, false, false, {}, nullptr};
    setStatus(L"System audio will be muted. Remaining: " + countdownText(fireTime));
    updateTray();
    hideWindowAfterConfirmation();
    return true;
}

void tickSchedule() {
    if (!gSchedule) return;
    auto& schedule = *gSchedule;
    if (schedule.action == ActionMode::smother) {
        if (std::chrono::system_clock::now() >= schedule.fireTime) {
            completeSchedule(muteDefaultAudio() ? L"System audio was muted."
                                                : L"Could not mute the default audio output.");
            return;
        }
        setStatus(L"System audio will be muted. Remaining: " + countdownText(schedule.fireTime));
        updateTray();
        return;
    }
    if (WaitForSingleObject(schedule.process, 0) == WAIT_OBJECT_0) {
        completeSchedule(schedule.target.name + L" is no longer running.");
        return;
    }
    if (!schedule.gracefulSent && std::chrono::system_clock::now() >= schedule.fireTime) {
        EnumWindows(closeTargetWindow, static_cast<LPARAM>(schedule.target.processId));
        schedule.gracefulSent = true;
        schedule.forceDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        setStatus(L"Asked " + schedule.target.name + L" to close.");
    } else if (schedule.gracefulSent && std::chrono::steady_clock::now() >= schedule.forceDeadline) {
        if (schedule.force) {
            if (TerminateProcess(schedule.process, 0)) completeSchedule(L"Force-closed " + schedule.target.name + L".");
            else completeSchedule(L"Could not force-close " + schedule.target.name + L".");
        } else {
            completeSchedule(L"Asked " + schedule.target.name + L" to close.");
        }
        return;
    }
    if (!schedule.gracefulSent) setStatus(schedule.target.name + L" is scheduled. Remaining: " + countdownText(schedule.fireTime));
    updateTray();
}

std::optional<std::pair<int, int>> parseClock(const std::wstring& value) {
    const auto colon = value.find(L':');
    if (colon == std::wstring::npos) return std::nullopt;
    try {
        const int hour = std::stoi(value.substr(0, colon));
        const int minute = std::stoi(value.substr(colon + 1));
        if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return std::nullopt;
        return std::pair{hour, minute};
    } catch (...) { return std::nullopt; }
}

void scheduleFromControls() {
    const auto action = static_cast<ActionMode>(SendMessageW(gActionCombo, CB_GETCURSEL, 0, 0));
    const int modeIndex = static_cast<int>(SendMessageW(gTimingCombo, CB_GETCURSEL, 0, 0));
    wchar_t value[64]{};
    GetWindowTextW(gValueEdit, value, static_cast<int>(std::size(value)));
    std::optional<std::chrono::system_clock::time_point> fireTime;
    if (modeIndex == 0) {
        try { fireTime = calculateFireTime(TimingMode::after, std::stoi(value)); } catch (...) {}
    } else if (modeIndex == 1) {
        if (auto clock = parseClock(value)) fireTime = calculateFireTime(TimingMode::at, clock->first, clock->second);
    } else {
        fireTime = calculateFireTime(TimingMode::topOfHour, 0);
    }
    if (!fireTime) {
        setStatus(modeIndex == 1 ? L"Enter a valid 24-hour time such as 23:15." : L"Enter a positive number of minutes.");
        return;
    }
    if (action == ActionMode::smother) {
        armSmotherSchedule(*fireTime);
        return;
    }
    const int appIndex = static_cast<int>(SendMessageW(gAppCombo, CB_GETCURSEL, 0, 0));
    if (appIndex < 0 || appIndex >= static_cast<int>(gApps.size())) {
        setStatus(L"Choose an application first.");
        return;
    }
    armSubdueSchedule(gApps[appIndex], *fireTime, SendMessageW(gForceCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
}

void updateTimingInput() {
    const int mode = static_cast<int>(SendMessageW(gTimingCombo, CB_GETCURSEL, 0, 0));
    EnableWindow(gValueEdit, mode != 2);
    SetWindowTextW(gValueEdit, mode == 0 ? L"30" : mode == 1 ? L"23:15" : L"");
}

void updateActionControls() {
    const bool subdue = SendMessageW(gActionCombo, CB_GETCURSEL, 0, 0) == 0;
    EnableWindow(gAppLabel, subdue);
    EnableWindow(gAppCombo, subdue);
    EnableWindow(gRefreshButton, subdue);
    EnableWindow(gForceCheck, subdue);
    SetWindowTextW(gScheduleButton, subdue ? L"Schedule Subdue" : L"Schedule Smother");
    setStatus(subdue ? L"Choose an application and a time."
                     : L"Choose when to mute the default audio output.");
}

void trayMenu() {
    POINT point{};
    GetCursorPos(&point);
    HMENU menu = CreatePopupMenu();
    if (gSchedule) {
        const std::wstring status = scheduleName(*gSchedule) + L" — " + countdownText(gSchedule->fireTime);
        AppendMenuW(menu, MF_STRING | MF_DISABLED, 0, status.c_str());
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    }
    AppendMenuW(menu, MF_STRING, IDM_SHOW, L"Show Subdue");
    AppendMenuW(menu, MF_STRING | (gSchedule ? 0 : MF_GRAYED), IDM_CANCEL, L"Cancel Schedule");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDM_EXIT, L"Exit Subdue");
    SetForegroundWindow(gWindow);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, point.x, point.y, 0, gWindow, nullptr);
    DestroyMenu(menu);
}

Options parseOptions() {
    Options options;
    int count{};
    LPWSTR* arguments = CommandLineToArgvW(GetCommandLineW(), &count);
    for (int index = 1; index < count; ++index) {
        const std::wstring argument = arguments[index];
        auto next = [&]() -> std::wstring {
            if (++index >= count) return {};
            return arguments[index];
        };
        if (argument == L"--app") options.app = next();
        else if (argument == L"--mute") options.mute = true;
        else if (argument == L"--after") {
            const auto value = next();
            try { options.minutes = std::stoi(value); options.timing = TimingMode::after; }
            catch (...) { options.error = L"--after requires a positive number of minutes"; }
        } else if (argument == L"--at") {
            const auto value = next();
            if (auto clock = parseClock(value)) {
                options.hour = clock->first; options.minute = clock->second; options.timing = TimingMode::at;
            } else options.error = L"--at requires a valid HH:mm time";
        } else if (argument == L"--top" || argument == L"--top-of-hour") options.timing = TimingMode::topOfHour;
        else if (argument == L"--force") options.force = true;
        else if (argument == L"--no-force") options.force = false;
        else if (argument == L"--foreground") options.foreground = true;
        else if (argument == L"--help" || argument == L"-h") options.help = true;
        else options.error = L"Unknown option: " + argument;
    }
    LocalFree(arguments);
    return options;
}

void handleOptions(const Options& options) {
    constexpr wchar_t usage[] =
        L"Subdue (--app NAME | --mute) (--after MINUTES | --at HH:mm | --top-of-hour) [--no-force] [--foreground]\n\n"
        L"--mute schedules Smother for the default audio output.\n"
        L"Force-close after 10 seconds is enabled by default for app schedules.";
    if (options.help || !options.error.empty()) {
        MessageBoxW(gWindow, options.error.empty() ? usage : (options.error + L"\n\n" + usage).c_str(),
                    L"Subdue command line", options.error.empty() ? MB_OK : MB_OK | MB_ICONERROR);
        if (options.help) return;
    }
    if (!options.timing || !options.error.empty()) return;
    if (options.mute && !options.app.empty()) {
        setStatus(L"Supply either --app or --mute, not both.");
        showMainWindow();
        return;
    }
    if (!options.mute && options.app.empty()) return;
    std::optional<std::chrono::system_clock::time_point> fireTime;
    if (*options.timing == TimingMode::after) fireTime = calculateFireTime(*options.timing, options.minutes);
    else if (*options.timing == TimingMode::at) fireTime = calculateFireTime(*options.timing, options.hour, options.minute);
    else fireTime = calculateFireTime(*options.timing, 0);
    bool armed = false;
    if (fireTime && options.mute) {
        armed = armSmotherSchedule(*fireTime);
    } else if (fireTime) {
        const auto target = findProcess(options.app);
        if (!target) {
            setStatus(L"Could not find a running application matching “" + options.app + L"”.");
        } else {
            armed = armSubdueSchedule(*target, *fireTime, options.force);
        }
    }
    if (!armed || options.foreground) showMainWindow();
}

LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == gTaskbarCreated) { installTrayIcon(); updateTray(); return 0; }
    switch (message) {
    case WM_CREATE: {
        gWindow = window;
        gFont = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        makeControl(L"STATIC", L"Subdue", SS_LEFT, 24, 20, 300, 38);
        HWND heading = GetWindow(window, GW_CHILD);
        HFONT titleFont = CreateFontW(-28, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                      DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        SendMessageW(heading, WM_SETFONT, reinterpret_cast<WPARAM>(titleFont), TRUE);
        SetPropW(window, L"SubdueTitleFont", titleFont);
        makeControl(L"STATIC", L"Quiet an application or your audio on your schedule.", SS_LEFT, 24, 57, 440, 24);
        makeControl(L"STATIC", L"Action", SS_LEFT, 24, 98, 100, 24);
        gActionCombo = makeControl(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST, 128, 94, 270, 100, IDC_ACTION);
        SendMessageW(gActionCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Subdue an application"));
        SendMessageW(gActionCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Smother system audio"));
        SendMessageW(gActionCombo, CB_SETCURSEL, 0, 0);
        gAppLabel = makeControl(L"STATIC", L"Application", SS_LEFT, 24, 142, 100, 24);
        gAppCombo = makeControl(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL, 128, 138, 270, 300, IDC_APP);
        gRefreshButton = makeControl(L"BUTTON", L"Refresh", BS_PUSHBUTTON, 406, 138, 70, 28, IDC_REFRESH);
        makeControl(L"STATIC", L"When", SS_LEFT, 24, 186, 100, 24);
        gTimingCombo = makeControl(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST, 128, 182, 150, 200, IDC_TIMING);
        SendMessageW(gTimingCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"After (minutes)"));
        SendMessageW(gTimingCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"At time"));
        SendMessageW(gTimingCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Top of hour"));
        SendMessageW(gTimingCombo, CB_SETCURSEL, 0, 0);
        gValueEdit = makeControl(L"EDIT", L"30", WS_BORDER | ES_AUTOHSCROLL, 290, 182, 108, 28, IDC_VALUE);
        gForceCheck = makeControl(L"BUTTON", L"Force close if the application does not exit after 10 seconds",
                                  BS_AUTOCHECKBOX, 24, 232, 452, 28, IDC_FORCE);
        SendMessageW(gForceCheck, BM_SETCHECK, BST_CHECKED, 0);
        gScheduleButton = makeControl(L"BUTTON", L"Schedule Subdue", BS_DEFPUSHBUTTON, 336, 278, 140, 32, IDC_SCHEDULE);
        gStatus = makeControl(L"STATIC", L"Choose an application and a time.", SS_LEFT, 24, 328, 452, 48, IDC_STATUS);
        populateApps();
        installTrayIcon();
        SetTimer(window, kTickTimer, 1000, nullptr);
        return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_REFRESH: populateApps(); break;
        case IDC_ACTION: if (HIWORD(wParam) == CBN_SELCHANGE) updateActionControls(); break;
        case IDC_TIMING: if (HIWORD(wParam) == CBN_SELCHANGE) updateTimingInput(); break;
        case IDC_SCHEDULE: scheduleFromControls(); break;
        case IDM_SHOW: showMainWindow(); break;
        case IDM_CANCEL: cancelSchedule(); break;
        case IDM_EXIT: gExitRequested = true; DestroyWindow(window); break;
        }
        return 0;
    case WM_TIMER:
        if (wParam == kTickTimer) tickSchedule();
        else if (wParam == kHideTimer) {
            KillTimer(window, kHideTimer);
            if (gSchedule) ShowWindow(window, SW_HIDE);
        }
        return 0;
    case kTrayCallback:
        switch (LOWORD(lParam)) {
        case WM_LBUTTONDBLCLK: case NIN_SELECT: showMainWindow(); break;
        case WM_CONTEXTMENU: trayMenu(); break;
        }
        return 0;
    case WM_CLOSE:
        if (!gExitRequested) { ShowWindow(window, SW_HIDE); return 0; }
        break;
    case WM_DESTROY: {
        KillTimer(window, kTickTimer);
        KillTimer(window, kHideTimer);
        Shell_NotifyIconW(NIM_DELETE, &gTray);
        clearSchedule();
        if (auto titleFont = reinterpret_cast<HFONT>(GetPropW(window, L"SubdueTitleFont"))) DeleteObject(titleFont);
        if (gFont) DeleteObject(gFont);
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

} // namespace

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int showCommand) {
    gInstance = instance;
    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&controls);
    gIcon = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_SUBDUE), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
    gTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");

    WNDCLASSEXW klass{sizeof(klass)};
    klass.style = CS_HREDRAW | CS_VREDRAW;
    klass.lpfnWndProc = windowProc;
    klass.hInstance = instance;
    klass.hIcon = gIcon;
    klass.hIconSm = gIcon;
    klass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    klass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    klass.lpszClassName = kWindowClass;
    RegisterClassExW(&klass);

    RECT desired{0, 0, 500, 410};
    AdjustWindowRectExForDpi(&desired, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                             FALSE, 0, GetDpiForSystem());
    HWND window = CreateWindowExW(0, kWindowClass, kWindowTitle,
                                  WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                                  CW_USEDEFAULT, CW_USEDEFAULT, desired.right - desired.left, desired.bottom - desired.top,
                                  nullptr, nullptr, instance, nullptr);
    if (!window) {
        if (SUCCEEDED(comResult)) CoUninitialize();
        return 1;
    }

    const Options options = parseOptions();
    if ((!options.mute && options.app.empty()) || !options.timing || !options.error.empty() || options.help) {
        ShowWindow(window, showCommand);
        UpdateWindow(window);
    }
    handleOptions(options);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    if (SUCCEEDED(comResult)) CoUninitialize();
    return static_cast<int>(message.wParam);
}
