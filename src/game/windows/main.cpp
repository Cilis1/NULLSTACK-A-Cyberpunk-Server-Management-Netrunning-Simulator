#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <filesystem>
#include <system_error>
#include "save/game_save.hpp"
#include "core/game.hpp"
#include "core/thermal.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace {
constexpr int FIRST_ACTION = 100;
constexpr int NEW_RUN = 200;
constexpr int SAVE_RUN = 201;
constexpr int LOAD_RUN = 202;
constexpr COLORREF BACKGROUND = RGB(14, 19, 23);
constexpr COLORREF TEXT = RGB(213, 222, 218);
constexpr COLORREF ACCENT = RGB(255, 155, 58);
constexpr int MIN_WIDTH = 790;
constexpr int MIN_HEIGHT = 760;

struct Action {
    const wchar_t *label;
    ns::GameAction value;
};
constexpr std::array<Action, 8> ACTIONS{{
    {L"HOST CONTRACTS", ns::GameAction::Host},
    {L"RUN INTRUSION", ns::GameAction::Intrude},
    {L"BUY SERVER  /  300", ns::GameAction::BuyServer},
    {L"ADD COOLING  /  180", ns::GameAction::BuyCooling},
    {L"UPGRADE POWER  /  220", ns::GameAction::UpgradePower},
    {L"REPAIR  /  100", ns::GameAction::Repair},
    {L"LAY LOW", ns::GameAction::LayLow},
    {L"ARCHIVE CONTRACT", ns::GameAction::Archive},
}};

struct WindowState {
    ns::Game game{42};
    std::uint64_t seed = 42;
    HWND title{}, subtitle{}, status{}, goal{}, message{}, rules{}, restart{}, save{}, load{};
    std::array<HWND, ACTIONS.size()> buttons{};
    HFONT font{}, titleFont{};
    HBRUSH background{};
};

HWND label(HWND parent, const wchar_t *text, int id = 0) {
    return CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE | SS_LEFT,
                           0, 0, 0, 0, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                           GetModuleHandleW(nullptr), nullptr);
}
HWND button(HWND parent, const wchar_t *text, int id) {
    return CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                           0, 0, 0, 0, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                           GetModuleHandleW(nullptr), nullptr);
}
void setFont(HWND control, HFONT font) { SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE); }
void position(HWND control, int x, int y, int width, int height) {
    MoveWindow(control, x, y, width, height, TRUE);
}
std::wstring widen(std::string_view text) { return std::wstring(text.begin(), text.end()); }
void update(WindowState &ui, std::wstring_view message) {
    const auto &s = ui.game.state();
    const int overhead = 25 + s.servers * 10 + s.cooling * 5;
    const auto stats = L"DAY " + std::to_wstring(s.day) + L"     CREDITS " + std::to_wstring(s.credits) +
        L"     REPUTATION " + std::to_wstring(s.reputation) + L"     HEAT " + std::to_wstring(s.heat) + L" / 100\n\n" +
        L"SERVERS " + std::to_wstring(s.servers) + L" / 8     POWER " + std::to_wstring(s.power) +
        L"     COOLING " + std::to_wstring(s.cooling) + L"     CONDITION " + std::to_wstring(s.condition) +
        L"%\n\nRACK " + std::to_wstring(s.temperature) + L" C   THROUGHPUT " + std::to_wstring(ns::thermalThroughput(s.temperature)) +
        L"%   BASE COST " + std::to_wstring(overhead) + L" / DAY";
    SetWindowTextW(ui.status, stats.c_str());
    std::wstring result(message);
    if (s.outcome == ns::GameOutcome::Won) result += L"\n\nVICTORY // Your back room is a real operation.";
    if (s.outcome == ns::GameOutcome::Raided) result += L"\n\nRAID // The door was cheaper than the warrant.";
    if (s.outcome == ns::GameOutcome::Bankrupt) result += L"\n\nBANKRUPT // The landlord has changed the locks.";
    SetWindowTextW(ui.message, result.c_str());
    for (HWND control : ui.buttons) EnableWindow(control, s.outcome == ns::GameOutcome::Playing);
}
std::filesystem::path savePath() {
    PWSTR raw = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &raw))) return {};
    std::filesystem::path path(raw);
    CoTaskMemFree(raw);
    return path / L"Nullstack" / L"saves" / L"back-room.sqlite";
}
void layout(HWND window, WindowState &ui) {
    RECT area{}; GetClientRect(window, &area);
    const int width = area.right;
    const int left = 32;
    const int gap = 12;
    const int col = (width - 2 * left - 2 * gap) / 3;
    position(ui.title, left, 22, width - 64, 38);
    position(ui.subtitle, left, 61, width - 64, 28);
    position(ui.status, left, 104, width - 64, 98);
    position(ui.goal, left, 207, width - 64, 45);
    for (std::size_t i = 0; i < ui.buttons.size(); ++i) {
        const int x = left + static_cast<int>(i % 3) * (col + gap);
        const int y = 266 + static_cast<int>(i / 3) * 48;
        position(ui.buttons[i], x, y, col, 37);
    }
    position(ui.restart, left + 2 * (col + gap), 362, col, 37);
    position(ui.save, left, 410, col, 37);
    position(ui.load, left + (col + gap), 410, col, 37);
    position(ui.message, left, 467, width - 64, 66);
    position(ui.rules, left, 546, width - 64, 125);
}
LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto *ui = reinterpret_cast<WindowState *>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto *create = reinterpret_cast<CREATESTRUCTW *>(lParam);
        ui = static_cast<WindowState *>(create->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ui));
    }
    if (!ui) return DefWindowProcW(window, message, wParam, lParam);
    switch (message) {
    case WM_CREATE:
        ui->background = CreateSolidBrush(BACKGROUND);
        ui->font = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH, L"Consolas");
        ui->titleFont = CreateFontW(-30, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH, L"Consolas");
        ui->title = label(window, L"NULLSTACK  //  BACK ROOM");
        ui->subtitle = label(window, L"A NOODLE SHOP. A RACK. A VERY PATIENT LANDLORD.");
        ui->status = label(window, L"");
        ui->goal = label(window, L"OBJECTIVE  /  2500 credits + 4 servers + 5 reputation\nAvoid bankruptcy and keep heat below 100.");
        ui->message = label(window, L"");
        ui->rules = label(window, L"HOW IT WORKS\nEach action costs a day and overhead. Hosting costs 8 extra per server; hot racks throttle above 45 C and wear faster. Archive contracts require two servers, two reputation, 50% condition. Intrusions raise legal heat; lay low reduces it. Upgrade power before hosting with more servers.\n\nSave and load use one manual slot; your run is not saved automatically.");
        for (std::size_t i = 0; i < ACTIONS.size(); ++i)
            ui->buttons[i] = button(window, ACTIONS[i].label, FIRST_ACTION + static_cast<int>(i));
        ui->restart = button(window, L"NEW RUN", NEW_RUN);
        ui->save = button(window, L"SAVE RUN", SAVE_RUN);
        ui->load = button(window, L"LOAD RUN", LOAD_RUN);
        setFont(ui->title, ui->titleFont);
        for (HWND h : {ui->subtitle, ui->status, ui->goal, ui->message, ui->rules, ui->restart, ui->save, ui->load}) setFont(h, ui->font);
        for (HWND h : ui->buttons) setFont(h, ui->font);
        update(*ui, L"SYSTEM READY // Choose an action to begin.");
        layout(window, *ui);
        return 0;
    case WM_GETMINMAXINFO: {
        auto *info = reinterpret_cast<MINMAXINFO *>(lParam);
        info->ptMinTrackSize.x = MIN_WIDTH;
        info->ptMinTrackSize.y = MIN_HEIGHT;
        return 0;
    }
    case WM_SIZE: layout(window, *ui); return 0;
    case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, reinterpret_cast<HWND>(lParam) == ui->title ? ACCENT : TEXT);
        SetBkColor(dc, BACKGROUND);
        return reinterpret_cast<LRESULT>(ui->background);
    }
    case WM_ERASEBKGND: {
        RECT rect{}; GetClientRect(window, &rect);
        FillRect(reinterpret_cast<HDC>(wParam), &rect, ui->background);
        return 1;
    }
    case WM_COMMAND: {
        const int id = LOWORD(wParam);
        if (id >= FIRST_ACTION && id < FIRST_ACTION + static_cast<int>(ACTIONS.size())) {
            const auto result = ui->game.act(ACTIONS[static_cast<std::size_t>(id - FIRST_ACTION)].value);
            update(*ui, widen(result.message));
        } else if (id == NEW_RUN) {
            ui->game = ns::Game(++ui->seed);
            update(*ui, L"NEW RUN // The rack hums back to life.");
        } else if (id == SAVE_RUN || id == LOAD_RUN) {
            const auto path = savePath();
            if (path.empty()) {
                update(*ui, L"SAVE ERROR // Cannot locate your application data folder.");
            } else if (id == SAVE_RUN) {
                std::error_code ec;
                std::filesystem::create_directories(path.parent_path(), ec);
                if (ec) update(*ui, L"SAVE ERROR // Cannot create the save directory.");
                else {
                    const auto result = ns::save::writeGame(path, ui->game);
                    update(*ui, result.ok ? L"RUN SAVED // Manual slot updated." : L"SAVE ERROR // " + widen(result.message));
                }
            } else {
                const auto result = ns::save::loadGame(path, ui->game);
                update(*ui, result.ok ? L"RUN LOADED // Manual slot restored." : L"LOAD ERROR // " + widen(result.message));
            }
        }
        return 0;
    }
    case WM_DESTROY:
        if (ui->font) DeleteObject(ui->font);
        if (ui->titleFont) DeleteObject(ui->titleFont);
        if (ui->background) DeleteObject(ui->background);
        PostQuitMessage(0); return 0;
    default: return DefWindowProcW(window, message, wParam, lParam);
    }
}
} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR commandLine, int show) {
    const bool smoke = std::wstring_view(commandLine) == L"--smoke-test";
    WNDCLASSW wc{};
    wc.lpfnWndProc = windowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"NullstackBackRoom";
    if (!RegisterClassW(&wc)) return 1;
    WindowState ui;
    HWND window = CreateWindowExW(WS_EX_CONTROLPARENT, wc.lpszClassName, L"NULLSTACK // Back Room",
                                  WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 920, 800,
                                  nullptr, nullptr, instance, &ui);
    if (!window) return 1;
    if (smoke) {
        SendMessageW(window, WM_COMMAND, FIRST_ACTION, 0);
        const bool ok = ui.game.state().day == 1 && ui.game.state().reputation == 1;
        SendMessageW(window, WM_COMMAND, NEW_RUN, 0);
        const bool restarted = ui.game.state().day == 0;
        DestroyWindow(window);
        return ok && restarted ? 0 : 1;
    }
    ShowWindow(window, show);
    UpdateWindow(window);
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(window, &msg)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    }
    return static_cast<int>(msg.wParam);
}
