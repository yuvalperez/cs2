#pragma once
#include "skStr.h"
#include "auth.hpp"
#include "utils.hpp"
#include <locale>
#include <codecvt>
#include <iostream>
inline void xor_encrypt_decrypt(std::string& data, const std::string& key) {
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] ^= key[i % key.size()];
    }
}
inline void write_encrypted_json(const std::string& filename, const std::string& data, const std::string& key) {
    std::string encrypted_data = data;
    xor_encrypt_decrypt(encrypted_data, key);

    std::ofstream file(filename, std::ios::binary);
    if (file.is_open()) {
        file.write(encrypted_data.c_str(), encrypted_data.size());
        file.close();
    }
    else {
        std::cerr << "Unable to open file for writing: " << filename << std::endl;
    }
}

inline std::string read_encrypted_json(const std::string& filename, const std::string& key) {
    std::ifstream file(filename, std::ios::binary);
    std::string data;

    if (file.is_open()) {
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        data.resize(size);
        file.read(&data[0], size);
        file.close();

        xor_encrypt_decrypt(data, key);
    }
    else {
        std::cerr << "Unable to open file for reading: " << filename << std::endl;
    }

    return data;
}
// Function to convert std::string to std::wstring
inline std::wstring s2ws(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}
std::string ws2s(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

inline std::wstring tm_to_readable_time(tm ctx) {
    wchar_t buffer[80];
    wcsftime(buffer, sizeof(buffer), L"%a %m/%d/%y %H:%M:%S %Z", &ctx);
    return std::wstring(buffer);
}

inline std::time_t string_to_timet(const std::string& timestamp) {
    auto cv = strtol(timestamp.c_str(), NULL, 10);
    return static_cast<time_t>(cv);
}

inline std::tm timet_to_tm(std::time_t timestamp) {
    std::tm context;
    localtime_s(&context, &timestamp);
    return context;
}
inline void initialize_login_system() {
    using namespace KeyAuth;

    std::string name = skCrypt("cs2 external").decrypt();
    std::string ownerid = skCrypt("1lOI36NcCr").decrypt();
    std::string secret = skCrypt("7fa6bedbe2dc13f01277c7114b7754662d4f5fc52f9dfde2bfe3066f55ab26c0").decrypt();
    std::string version = skCrypt("1.0").decrypt();
    std::string url = skCrypt("https://keyauth.win/api/1.2/").decrypt();
    std::string path = skCrypt("").decrypt();

    // Initialize the KeyAuth API
    api KeyAuthApp(name, ownerid, secret, version, url, path);

    // Clear sensitive data from memory
    name.clear(); ownerid.clear(); secret.clear(); version.clear(); url.clear();

    std::wstring consoleTitle = s2ws(skCrypt("Loader - Built at:  ").decrypt()) + s2ws(__DATE__) + L" " + s2ws(__TIME__);
    SetConsoleTitleW(consoleTitle.c_str());
    std::wcout << L"\n\n Connecting...";
    KeyAuthApp.init();
    if (!KeyAuthApp.response.success)
    {
        std::wcout << L"\n Status: " << s2ws(KeyAuthApp.response.message);
        Sleep(1000);
        exit(1);
    }

    const std::string encryption_key = "!whipid8338383545345(*^%$#$djdsawa;ecKKk";

    if (std::filesystem::exists("test.json"))
    {
        std::string encrypted_data = read_encrypted_json("test.json", encryption_key);
        std::ofstream temp_file("temp.json");
        temp_file << encrypted_data;
        temp_file.close();

        if (!CheckIfJsonKeyExists("temp.json", "username"))
        {
            std::string key = ReadFromJson("temp.json", "license");
            KeyAuthApp.license(key);
            if (!KeyAuthApp.response.success)
            {
                std::remove("test.json");
                std::remove("temp.json");
                std::wcout << L"\n Status: " << s2ws(KeyAuthApp.response.message);
                Sleep(1500);
                exit(1);
            }
            std::wcout << L"\n\n Successfully Automatically Logged In\n";
        }
        else
        {
            std::string username = ReadFromJson("temp.json", "username");
            std::string password = ReadFromJson("temp.json", "password");
            KeyAuthApp.login(username, password);
            if (!KeyAuthApp.response.success)
            {
                std::remove("test.json");
                std::remove("temp.json");
                std::wcout << L"\n Status: " << s2ws(KeyAuthApp.response.message);
                Sleep(1500);
                exit(1);
            }
            std::wcout << L"\n\n Successfully Automatically Logged In\n";
        }
        std::remove("temp.json");
    }
    else
    {
        std::wcout << L"\n\n [1] Login\n [2] Register\n [3] License key only\n\n Choose option: ";

        int option;
        std::wstring username;
        std::wstring password;
        std::wstring license_key;

        std::wcin >> option;
        switch (option)
        {
        case 1:
            std::wcout << L"\n\n Enter username: ";
            std::wcin >> username;
            std::wcout << L"\n Enter password: ";
            std::wcin >> password;
            KeyAuthApp.login(ws2s(username), ws2s(password));
            break;
        case 2:
            std::wcout << L"\n\n Enter username: ";
            std::wcin >> username;
            std::wcout << L"\n Enter password: ";
            std::wcin >> password;
            std::wcout << L"\n Enter license: ";
            std::wcin >> license_key;
            KeyAuthApp.regstr(ws2s(username), ws2s(password), ws2s(license_key));
            break;
        case 3:
            std::wcout << L"\n Enter license: ";
            std::wcin >> license_key;
            KeyAuthApp.license(ws2s(license_key));
            break;
        default:
            exit(1);
        }

        if (!KeyAuthApp.response.success)
        {
            std::wcout << L"\n Status: " << s2ws(KeyAuthApp.response.message);
            Sleep(1500);
            exit(1);
        }
        std::string json_data;
        if (username.empty() || password.empty())
        {
            WriteToJson("temp.json", "license", ws2s(license_key), false, "", "");
            std::ifstream temp_file("temp.json");
            json_data.assign((std::istreambuf_iterator<char>(temp_file)), std::istreambuf_iterator<char>());
            temp_file.close();
            std::remove("temp.json");
            write_encrypted_json("test.json", json_data, encryption_key);
            std::wcout << L"Successfully Created File For Auto Login";
        }
        else
        {
            WriteToJson("temp.json", "username", ws2s(username), true, "password", ws2s(password));
            std::ifstream temp_file("temp.json");
            json_data.assign((std::istreambuf_iterator<char>(temp_file)), std::istreambuf_iterator<char>());
            temp_file.close();
            std::remove("temp.json");
            write_encrypted_json("test.json", json_data, encryption_key);
            std::wcout << L"Successfully Created File For Auto Login";
        }
    }

    std::wcout << L"\n User data:";
    std::wcout << L"\n Username: " << s2ws(KeyAuthApp.user_data.username);
    std::wcout << L"\n IP address: " << s2ws(KeyAuthApp.user_data.ip);
    std::wcout << L"\n Hardware-Id: " << s2ws(KeyAuthApp.user_data.hwid);
    std::wcout << L"\n Create date: " << tm_to_readable_time(timet_to_tm(string_to_timet(KeyAuthApp.user_data.createdate)));
    std::wcout << L"\n Last login: " << tm_to_readable_time(timet_to_tm(string_to_timet(KeyAuthApp.user_data.lastlogin)));
    std::wcout << L"\n Subscription(s): ";

    for (int i = 0; i < KeyAuthApp.user_data.subscriptions.size(); i++) {
        auto sub = KeyAuthApp.user_data.subscriptions.at(i);
        std::wcout << L"\n name: " << s2ws(sub.name);
        std::wcout << L" : expiry: " << tm_to_readable_time(timet_to_tm(string_to_timet(sub.expiry)));
    }
}