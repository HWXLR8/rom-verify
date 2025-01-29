#pragma once

#include <string>
#include <string_view>

#include <yaml-cpp/yaml.h>

class Config {
public:
    Config(std::string file);
    std::string get_datfile(std::string_view console);
    std::vector<std::string> get_romdirs(std::string_view console);
    std::vector<std::string> get_cats(std::string_view console);

private:
    YAML::Node conf_;
};
