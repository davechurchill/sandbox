#pragma once

#include "json/json.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>

class Settings
{
public:
    using json = nlohmann::ordered_json;

    json & section(const std::string & name)
    {
        json & value = m_root[name];
        if (!value.is_object())
        {
            value = json::object();
        }
        return value;
    }

    const json & section(const std::string & name) const
    {
        static const json empty = json::object();
        const auto found = m_root.find(name);
        return found != m_root.end() && found->is_object() ? *found : empty;
    }

    template <typename T>
    static void read(const json & object, const char * key, T & value)
    {
        const auto found = object.find(key);
        if (found != object.end())
        {
            value = found->get<T>();
        }
    }

    bool saveToFile(const std::string & filename) const
    {
        std::ofstream output(filename);
        if (!output)
        {
            std::cerr << "Failed to open settings file for writing: " << filename << '\n';
            return false;
        }

        output << std::setw(4) << m_root << '\n';
        if (!output)
        {
            std::cerr << "Failed while writing settings file: " << filename << '\n';
            return false;
        }
        return true;
    }

    bool loadFromFile(const std::string & filename)
    {
        std::ifstream input(filename);
        if (!input)
        {
            std::cerr << "Settings file not found; using defaults: " << filename << '\n';
            return false;
        }

        try
        {
            json loaded;
            input >> loaded;
            if (!loaded.is_object())
            {
                std::cerr << "Settings file root must be a JSON object: " << filename << '\n';
                return false;
            }
            m_root = std::move(loaded);
            return true;
        }
        catch (const std::exception & error)
        {
            std::cerr << "Failed to load settings from " << filename << ": " << error.what() << '\n';
            return false;
        }
    }

private:
    json m_root = json::object();
};
