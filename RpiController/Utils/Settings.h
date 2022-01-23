// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <string>
#include <vector>
#include "Json.h"
#include "Utils.h"
#include "FileSystem.h"

class Settings
{
    std::string filepath;
    nlohmann::json doc_;
    bool load_success_ = false;

public:
    static Settings& singleton() {
        static Settings instance;
        return instance;
    }

    static Settings& loadJSonFile(std::string filepath)
    {
        try {
            std::ifstream s;
            FileSystem::openTextFile(filepath, s);
            if (!s.fail()) {
                s >> singleton().doc_;
                singleton().load_success_ = true;
                singleton().filepath = filepath;
            }
            else {
                // new empty settings file.
                singleton().load_success_ = true;
                singleton().filepath = filepath;
            }
        }
        catch (std::exception& e)
        {
            std::cout << "error loadding " << filepath << ": ";
            const char* msg = e.what();
            if (msg != nullptr) {
                std::cout << msg;
            }
            else {
                std::cout << "unknown reason";
            }
            std::cout << "\n";
        }

        return singleton();
    }

    void saveJSonFile()
    {
        saveAsJSonFile(singleton().filepath);
    }

    void saveAsJSonFile(std::string path)
    {
        singleton().filepath = path;
        std::ofstream s;
        FileSystem::createTextFile(path, s);
        s << std::setw(2) << doc_ << std::endl;
    }

    std::string getString(const std::string& name, std::string defaultValue) const
    {
        if (doc_.count(name) == 1) {
            return doc_[name].get<std::string>();
        }
        else {
            return defaultValue;
        }
    }

    double getDouble(const std::string& name, double defaultValue) const
    {
        if (doc_.count(name) == 1) {
            return doc_[name].get<double>();
        }
        else {
            return defaultValue;
        }
    }

    float getFloat(const std::string& name, float defaultValue) const
    {
        if (doc_.count(name) == 1) {
            return doc_[name].get<float>();
        }
        else {
            return defaultValue;
        }
    }

    bool getBool(const std::string& name, bool defaultValue) const
    {
        if (doc_.count(name) == 1) {
            return doc_[name].get<bool>();
        }
        else {
            return defaultValue;
        }
    }

    bool hasKey(const std::string& key) const
    {
        return doc_.find(key) != doc_.end();
    }

    int getInt(const std::string& name, int defaultValue) const
    {
        if (doc_.count(name) == 1) {
            return doc_[name].get<int>();
        }
        else {
            return defaultValue;
        }
    }

    bool setString(const std::string& name, std::string value)
    {
        if (doc_.count(name) != 1 || doc_[name].type() != nlohmann::detail::value_t::string || doc_[name] != value) {
            doc_[name] = value;
            return true;
        }
        return false;
    }

    bool setDouble(const std::string& name, double value)
    {
        if (doc_.count(name) != 1 || doc_[name].type() != nlohmann::detail::value_t::number_float || static_cast<double>(doc_[name]) != value) {
            doc_[name] = value;
            return true;
        }
        return false;
    }

    bool setBool(const std::string& name, bool value)
    {
        if (doc_.count(name) != 1 || doc_[name].type() != nlohmann::detail::value_t::boolean || static_cast<bool>(doc_[name]) != value) {
            doc_[name] = value;
            return true;
        }
        return false;
    }

    bool setInt(const std::string& name, int value)
    {
        if (doc_.count(name) != 1 || doc_[name].type() != nlohmann::detail::value_t::number_integer || static_cast<int>(doc_[name]) != value) {
            doc_[name] = value;
            return true;
        }
        return false;
    }

};

#endif
