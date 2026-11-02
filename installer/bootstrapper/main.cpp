#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <shlobj.h>
#include <shellapi.h>
#include <commdlg.h>
#include <objbase.h>
#include <filesystem>
#include <string>
#include <vector>
#include <cstdio>
#include "payload_config.hpp"

namespace fs = std::filesystem;
namespace {
struct InternetHandle {
    HINTERNET value = nullptr;
    ~InternetHandle() { if (value) WinHttpCloseHandle(value); }
};
struct TempPath {
    fs::path path;
    ~TempPath() { std::error_code ec; fs::remove_all(path, ec); }
};
std::wstring errorMessage(const wchar_t *message) {
    return std::wstring(message) + L" (Windows error " + std::to_wstring(GetLastError()) + L")";
}
bool download(const std::wstring &url, const fs::path &path, std::wstring &error) {
    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    parts.dwHostNameLength = static_cast<DWORD>(-1);
    parts.dwUrlPathLength = static_cast<DWORD>(-1);
    parts.dwExtraInfoLength = static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &parts) || parts.nScheme != INTERNET_SCHEME_HTTPS) {
        error = L"Invalid HTTPS download URL."; return false;
    }
    std::wstring host(parts.lpszHostName, parts.dwHostNameLength);
    std::wstring target(parts.lpszUrlPath, parts.dwUrlPathLength);
    if (parts.dwExtraInfoLength) target.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);
    InternetHandle session{WinHttpOpen(L"NULLSTACK setup/0.1", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                       WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0)};
    if (!session.value) { error = errorMessage(L"Could not initialize network connection"); return false; }
    WinHttpSetTimeouts(session.value, 30000, 30000, 30000, 30000);
    InternetHandle connection{WinHttpConnect(session.value, host.c_str(), parts.nPort, 0)};
    if (!connection.value) { error = errorMessage(L"Could not connect"); return false; }
    InternetHandle request{WinHttpOpenRequest(connection.value, L"GET", target.c_str(), nullptr,
                                              WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE)};
    if (!request.value || !WinHttpSendRequest(request.value, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                              WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(request.value, nullptr)) {
        error = errorMessage(L"Download failed; check your internet connection or proxy"); return false;
    }
    DWORD status = 0, length = sizeof(status);
    if (!WinHttpQueryHeaders(request.value, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &status, &length, WINHTTP_NO_HEADER_INDEX) || status != 200) {
        error = L"The download server returned HTTP " + std::to_wstring(status) + L"."; return false;
    }
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) { error = errorMessage(L"Cannot create temporary download"); return false; }
    bool ok = true;
    unsigned long long total = 0;
    std::vector<char> buffer(65536);
    for (;;) {
        DWORD received = 0;
        if (!WinHttpReadData(request.value, buffer.data(), static_cast<DWORD>(buffer.size()), &received)) {
            error = errorMessage(L"Download interrupted"); ok = false; break;
        }
        if (!received) break;
        total += received;
        DWORD written = 0;
        if (total > 50ULL * 1024 * 1024 || !WriteFile(file, buffer.data(), received, &written, nullptr) || written != received) {
            error = L"Download is too large or could not be written (check disk space)."; ok = false; break;
        }
    }
    CloseHandle(file);
    return ok;
}
bool verify(const fs::path &path, std::wstring &error) {
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    HANDLE file = INVALID_HANDLE_VALUE;
    bool ok = false;
    std::vector<unsigned char> object;
    unsigned char digest[32]{};
    DWORD objectSize = 0, got = 0;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0) goto done;
    if (BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectSize), sizeof(objectSize), &got, 0) < 0) goto done;
    object.resize(objectSize);
    if (BCryptCreateHash(alg, &hash, object.data(), objectSize, nullptr, 0, 0) < 0) goto done;
    file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) goto done;
    for (;;) {
        unsigned char buf[65536]; DWORD read = 0;
        if (!ReadFile(file, buf, sizeof(buf), &read, nullptr)) goto done;
        if (!read) break;
        if (BCryptHashData(hash, buf, read, 0) < 0) goto done;
    }
    if (BCryptFinishHash(hash, digest, sizeof(digest), 0) < 0) goto done;
    {
        const char *hex = "0123456789abcdef";
        std::string actual;
        for (unsigned char byte : digest) { actual += hex[byte >> 4]; actual += hex[byte & 15]; }
        ok = actual == NULLSTACK_PAYLOAD_SHA256;
    }
done:
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    if (hash) BCryptDestroyHash(hash);
    if (alg) BCryptCloseAlgorithmProvider(alg, 0);
    if (!ok) error = L"Package integrity verification failed. Nothing was installed.";
    return ok;
}
fs::path knownFolder(REFKNOWNFOLDERID id) {
    PWSTR raw = nullptr;
    if (FAILED(SHGetKnownFolderPath(id, KF_FLAG_DEFAULT, nullptr, &raw))) return {};
    fs::path path(raw); CoTaskMemFree(raw); return path;
}
bool extract(const fs::path &zip, const fs::path &destination, std::wstring &error) {
    wchar_t systemDir[MAX_PATH];
    if (!GetSystemDirectoryW(systemDir, MAX_PATH)) { error = errorMessage(L"Cannot locate system directory"); return false; }
    fs::path tar = fs::path(systemDir) / L"tar.exe";
    // Paths are supplied as separate quoted arguments; Windows paths cannot contain quotes.
    std::wstring cmd = L"\"" + tar.wstring() + L"\" -xf \"" + zip.wstring() + L"\" -C \"" + destination.wstring() + L"\"";
    STARTUPINFOW si{}; si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(tar.c_str(), cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        error = L"Could not start Windows tar.exe. Windows 10 version 1803 or later is required."; return false;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 1; GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
    if (code != 0) { error = L"Could not extract the verified game package."; return false; }
    return true;
}
bool shortcut(const fs::path &link, const fs::path &target) {
    IShellLinkW *shell = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shell)))) return false;
    bool ok = SUCCEEDED(shell->SetPath(target.c_str())) && SUCCEEDED(shell->SetWorkingDirectory(target.parent_path().c_str())) &&
              SUCCEEDED(shell->SetDescription(L"NULLSTACK: Back Room"));
    IPersistFile *persist = nullptr;
    if (ok && SUCCEEDED(shell->QueryInterface(IID_PPV_ARGS(&persist)))) {
        ok = SUCCEEDED(persist->Save(link.c_str(), TRUE)); persist->Release();
    } else ok = false;
    shell->Release(); return ok;
}
int run() {
    int argc = 0;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return 1;
    bool silent = false, shortcuts = true;
    fs::path install, localArchive;
    for (int i = 1; i < argc; ++i) {
        std::wstring arg(argv[i]);
        if (arg == L"/S") silent = true;
        else if (arg == L"/no-shortcut") shortcuts = false;
        else if (arg.rfind(L"/D=", 0) == 0 && arg.size() > 3) install = arg.substr(3);
        else if (arg.rfind(L"/archive=", 0) == 0 && arg.size() > 9) localArchive = arg.substr(9);
        else { LocalFree(argv); MessageBoxW(nullptr, L"Unknown option. Use /S, /no-shortcut, or /D=<path>.", L"NULLSTACK Setup", MB_ICONERROR); return 2; }
    }
    LocalFree(argv);
    const auto fail = [silent](const std::wstring &message) {
        if (!silent) MessageBoxW(nullptr, message.c_str(), L"NULLSTACK Setup", MB_OK | MB_ICONERROR);
        return 1;
    };
    if (install.empty()) {
        const auto base = knownFolder(FOLDERID_LocalAppData);
        if (base.empty()) return fail(L"Cannot locate your local application data folder.");
        install = base / L"Programs" / L"Nullstack";
    }
    if (!silent && MessageBoxW(nullptr, (L"Install NULLSTACK: Back Room to:\n\n" + install.wstring() +
          L"\n\nSetup downloads and verifies the early-access game. Continue?").c_str(), L"NULLSTACK Setup",
          MB_YESNO | MB_ICONQUESTION) != IDYES) return 0;
    wchar_t tempDir[MAX_PATH], tempName[MAX_PATH];
    if (!GetTempPathW(MAX_PATH, tempDir) || !GetTempFileNameW(tempDir, L"nst", 0, tempName))
        return fail(errorMessage(L"Cannot create temporary directory"));
    TempPath temp{tempName};
    DeleteFileW(temp.path.c_str());
    std::error_code ec;
    if (!fs::create_directory(temp.path, ec)) return fail(L"Cannot create temporary directory.");
    fs::path archive = localArchive.empty() ? temp.path / L"payload.zip" : localArchive;
    std::wstring error;
    if (localArchive.empty() && !download(NULLSTACK_PAYLOAD_URL, archive, error)) {
        if (silent) return fail(error);
        const auto prompt = error + L"\n\nIf the release is private, download the matching Windows ZIP in your browser. Select it now?";
        if (MessageBoxW(nullptr, prompt.c_str(), L"NULLSTACK Setup", MB_YESNO | MB_ICONWARNING) != IDYES) return 1;
        wchar_t selected[32768]{};
        OPENFILENAMEW dialog{};
        dialog.lStructSize = sizeof(dialog);
        dialog.lpstrFilter = L"ZIP archive\0*.zip\0All files\0*.*\0\0";
        dialog.lpstrFile = selected;
        dialog.nMaxFile = static_cast<DWORD>(sizeof(selected) / sizeof(selected[0]));
        dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        dialog.lpstrTitle = L"Select the matching NULLSTACK release ZIP";
        if (!GetOpenFileNameW(&dialog)) return fail(L"No game package selected.");
        archive = selected;
    }
    if (!verify(archive, error) || !extract(archive, temp.path, error)) return fail(error);
    fs::create_directories(install, ec);
    if (ec) return fail(L"Cannot create installation directory. Check permissions and disk space.");
    for (const auto *name : {L"Nullstack.exe", L"README.md", L"LICENSE", L"EARLY_ACCESS.md", L"THIRD_PARTY.md", L"sqlite-amalgamation-BSD.txt"}) {
        fs::copy_file(temp.path / name, install / name, fs::copy_options::overwrite_existing, ec);
        if (ec) return fail(L"Could not install game files. Close the game and check permissions and disk space.");
    }
    bool linksOk = true;
    if (shortcuts) {
        const HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (SUCCEEDED(initialized)) {
            const auto desktop = knownFolder(FOLDERID_Desktop);
            const auto menu = knownFolder(FOLDERID_Programs);
            linksOk = !desktop.empty() && shortcut(desktop / L"NULLSTACK.lnk", install / L"Nullstack.exe");
            linksOk = !menu.empty() && shortcut(menu / L"NULLSTACK.lnk", install / L"Nullstack.exe") && linksOk;
            CoUninitialize();
        } else linksOk = false;
    }
    if (!linksOk) return fail(L"Game files were installed, but one or more shortcuts could not be created.");
    if (!silent && MessageBoxW(nullptr, L"NULLSTACK is installed. Launch the game now?", L"NULLSTACK Setup",
                                MB_YESNO | MB_ICONINFORMATION) == IDYES) {
        if (reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", (install / L"Nullstack.exe").c_str(), nullptr,
                                                    install.c_str(), SW_SHOWNORMAL)) <= 32)
            return fail(L"Installed successfully, but the game could not be launched.");
    }
    return 0;
}
} // namespace
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) { return run(); }
