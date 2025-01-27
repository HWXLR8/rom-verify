#include "console.hpp"

#include <filesystem>
#include <future>
#include <iostream>

#include <tinyxml2.h>
#include <yaml-cpp/yaml.h>
#include <zip.h>
#include <zlib.h>

Console::Console(const std::string& name) {
    name_ = name;

    // load romdirs from config
    YAML::Node conf = YAML::LoadFile("rom.yaml");
    YAML::Node dirs = conf["console"][name_]["dirs"];
    if (dirs && dirs.IsSequence() && dirs.size() > 0) {
        for (const auto& dir : dirs) {
            std::string romdir = dir.as<std::string>();
            romdirs_.push_back(romdir);
        }
    } else {
        throw std::runtime_error("no valid rom directory found, check rom.yaml");
    }

    // TODO: move to YAML
    cats_ = {
        "Japan",
        "USA",
        // "Europe",
        // "China",
        // "Asia",
        // "(Beta)"
    };
    // TODO: move to YAML
    dat_file_ = "dat/nes.dat";

    // split the dat into seperate dats for each cat
    for (const auto& cat : cats_) {
        std::unordered_map<std::string, std::string> rom_crc;
        parse_dat(cat, rom_crc);
        // capture the original size of the dat before we remove items
        dat_sizes_[cat] = rom_crc.size();
        dats_[cat] = rom_crc;
        // initialize empty romset for each cat
        romsets_[cat] = std::unordered_set<std::string>();
    }
}

void Console::find_roms() {
    for (const auto& dir : romdirs_) {
        for (const auto& f : std::filesystem::recursive_directory_iterator(dir)) {
            const auto& path = f.path();

            if (f.is_regular_file()) {
                roms_.push_back(path.string());
            }
        }
    }
}

void Console::parse_dat(std::string cat, std::unordered_map<std::string, std::string>& rom_crc) {
    tinyxml2::XMLDocument dat;
    dat.LoadFile(dat_file_.c_str());
    tinyxml2::XMLElement* root = dat.RootElement();

    for (tinyxml2::XMLElement* game = root->FirstChildElement("game");
         game != nullptr;
         game = game->NextSiblingElement("game")) {
        const char* name = game->Attribute("name");

        // filter based on category
        if (std::string(name).find(cat) == std::string::npos) {
            continue;
        }

        tinyxml2::XMLElement* rom = game->FirstChildElement("rom");
        const char* crc = rom->Attribute("crc");

        rom_crc[crc] = name;
    }

}

void Console::process_roms() {
    for (const auto& cat : cats_) {
        std::vector<std::future<void>> futures;
        for (const auto& rom : roms_) {
            futures.push_back
                (std::async
                 (std::launch::async,
                  [this, rom, cat]() {
                      check_rom(rom, cat);
                  }
                  ));
        }
        for (auto& f : futures) {
            f.get();
        }
    }
}

void Console::check_rom(const std::string& rom, const std::string& cat) {
    uLong crc;
    // if rom is zip archive
    if (rom.substr(rom.find_last_of(".") + 1) == "zip") {
        int err = 0;
        zip* z = zip_open(rom.c_str(), ZIP_RDONLY, &err);
        if (!z) {
            zip_error_t error;
            zip_error_init_with_code(&error, err);
            std::cout << "Failed to unzip " << rom << ": " << zip_error_strerror(&error) << std::endl;
            zip_error_fini(&error);
            return;
        }

        // zip stats (size, etc)
        zip_stat_t stat;
        zip_stat(z, zip_get_name(z, 0, 0), 0, &stat);

        // open first file in  zip (assume only 1 file)
        zip_file* f = zip_fopen(z, zip_get_name(z, 0, 0), ZIP_FL_UNCHANGED);
        std::vector<char> data(stat.size);
        zip_fread(f, data.data(), data.size());
        zip_fclose(f);
        zip_close(z);

        // strip iNES header (first 16B)
        std::vector<char> rom_data(data.begin() + 16, data.end());

        // calculate crc
        crc = crc32(0, reinterpret_cast<const Bytef*>(rom_data.data()), rom_data.size());
    }
    else { // ignore non-zips for now...
        return;
    }

    // cast crc to 32 bit hex string
    std::stringstream ss;
    ss << std::setw(8) << std::setfill('0') << std::hex << crc;
    std::string crc_str = ss.str();

    // if crc is in dat, add to romset, remove from dat
    auto it = dats_[cat].find(crc_str);
    if (it != dats_[cat].end()) {
        // std::cout << crc_str << " found" << std::endl;
        romsets_mtx_.lock();
        romsets_[cat].insert(crc_str);
        romsets_mtx_.unlock();
        dats_mtx_.lock();
        dats_[cat].erase(it);
        dats_mtx_.unlock();
    }
}

void Console::print_results() {
    for (const auto& cat : cats_) {
        const int romset_size = romsets_[cat].size();
        const int dat_size = dat_sizes_[cat];
        const double completion_rate = ((double)romset_size/dat_size * 100);

        std::cout << std::setw(10) << std::left << cat
                  << std::setw(8) << std::right << std::setprecision(4) << completion_rate << "%"
                  << std::setw(10) << romset_size << "/" << dat_size << std::endl;

        for (const auto& rom_map : dats_[cat]) {
            std::string rom = "(" + rom_map.first + ") " + rom_map.second;
            missing_.insert(rom);
        }
    }
}


void Console::print_missing() {
    for (const auto& rom : missing_) {
        std::cout << rom << std::endl;
    }
}
