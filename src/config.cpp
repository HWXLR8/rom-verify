#include "config.hpp"

Config::Config(std::string f) {
    conf_ = YAML::LoadFile(f);
}

std::string Config::get_datfile(std::string_view console) {
    YAML::Node dat = conf_["console"][console]["dat"];
    if (dat && dat.IsSequence() && dat.size() > 0) {
        return dat[0].as<std::string>();
    } else {
        throw std::runtime_error("no valid dat file found, check config file");
    }
}

std::vector<std::string> Config::get_romdirs(std::string_view console) {
    YAML::Node dirs = conf_["console"][console]["dirs"];
    std::vector<std::string> romdirs;
    if (dirs && dirs.IsSequence() && dirs.size() > 0) {
        for (const auto& dir : dirs) {
            std::string romdir = dir.as<std::string>();
            romdirs.push_back(romdir);
        }
    } else {
        throw std::runtime_error("no valid rom directory found, check config file");
    }
    return romdirs;
}

std::vector<std::string> Config::get_cats(std::string_view console) {
   YAML::Node categories = conf_["console"][console]["categories"];
   std::vector<std::string> cats;
   if (categories && categories.IsSequence() && categories.size() > 0) {
       for (const auto& category : categories) {
           std::string cat = category.as<std::string>();
           cats.push_back(cat);
       }
   } else {
       throw std::runtime_error("no valid categories found, check config file");
   }
   return cats;
}
