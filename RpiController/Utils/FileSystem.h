// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

#include <string>
#include <stdexcept>
#include <locale>
#include <codecvt>
#include <fstream>

class FileSystem
{
public:

    // please use the combine() method instead.
    static const char kPathSeparator =
#ifdef _WIN32
        '\\';
#else
        '/';
#endif

    // Combine two paths using standard path separator also being careful not to create two
    // separators if one of the paths already has one.
    static std::string combine(const std::string& parentFolder, const std::string& child) {
        if (child.size() == 0)
            return parentFolder;

        size_t len = parentFolder.size();
        if (parentFolder.size() > 0 && parentFolder[len - 1] == kPathSeparator) {
            // parent already ends with '/'
            return parentFolder + child;
        }
        len = child.size();
        if (len > 0 && child[0] == kPathSeparator) {
            // child already starts with '/'
            return parentFolder + child;
        }
        return parentFolder + kPathSeparator + child;
    }


    static std::string getUserHomeFolder()
    {
        //Windows uses USERPROFILE, Linux uses HOME
#ifdef _WIN32
        std::wstring userProfile = _wgetenv(L"USERPROFILE");
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        return converter.to_bytes(userProfile);
#else
        return std::getenv("HOME");
#endif
    }

    static void openTextFile(const std::string& filepath, std::ifstream& file) {

#ifdef _WIN32
        // WIN32 will create the wrong file names if we don't first convert them to UTF-16.
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        std::wstring wide_path = converter.from_bytes(filepath);
        file.open(wide_path, std::ios::in);
#else
        file.open(filepath, std::ios::in);
#endif
    }

    static void createTextFile(const std::string& filepath, std::ofstream& file) {

#ifdef _WIN32
        // WIN32 will create the wrong file names if we don't first convert them to UTF-16.
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        std::wstring wide_path = converter.from_bytes(filepath);
        file.open(wide_path, std::ios::out | std::ios::trunc);
#else
        file.open(filepath, std::ios::trunc);
#endif
        if (file.fail())
            throw std::ios_base::failure(std::strerror(errno));
    }

};

#endif
