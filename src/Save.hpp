#pragma once

#include "json/json.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>

struct Save
{
    using Json = nlohmann::ordered_json;

    Json settings = Json::object();

    Json & section(const std::string & name)
    {
        Json & value = settings[name];
        if (!value.is_object())
        {
            value = Json::object();
        }
        return value;
    }

    const Json & section(const std::string & name) const
    {
        static const Json empty = Json::object();
        const auto found = settings.find(name);
        return found != settings.end() && found->is_object() ? *found : empty;
    }

    template <typename T>
    static void read(const Json & object, const char * key, T & value)
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

        output << std::setw(4) << settings << '\n';
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
            Json loaded;
            input >> loaded;
            if (!loaded.is_object())
            {
                std::cerr << "Settings file root must be a JSON object: " << filename << '\n';
                return false;
            }
            settings = std::move(loaded);
            return true;
        }
        catch (const std::exception & error)
        {
            std::cerr << "Failed to load settings from " << filename << ": " << error.what() << '\n';
            return false;
        }
    }
};
