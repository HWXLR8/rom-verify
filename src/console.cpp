#include "console.hpp"

#include <filesystem>
#include <future>
#include <iostream>

#include <tinyxml2.h>
#include <zip.h>
#include <zlib.h>

Console::Console(const std::string& name, Config* conf) {
    name_ = name;

    dat_file_ = conf->get_datfile(name);
    romdirs_ = conf->get_romdirs(name);
    cats_ = conf->get_cats(name);
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

void Console::parse_dat() {
    // split the dat into seperate dats for each cat
    for (const auto& cat : cats_) {
        std::unordered_map<std::string, std::string> rom_crc;

        // we start with a fresh dat each time since there may be
        // overlap between categories.
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

        // capture the original size of the dat before we remove items
        dat_sizes_[cat] = rom_crc.size();
        dats_[cat] = rom_crc;
        // initialize empty romset for each cat
        romsets_[cat] = std::unordered_set<std::string>();
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
        const auto ds = dat_sizes_[cat];
        for (auto& f : futures) {
            print_progress(cat, romsets_[cat].size(), ds);
            f.get();
        }
        std::cout << std::endl;
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

void Console::print_missing() {
    for (const auto& rom : missing_) {
        std::cout << rom << std::endl;
    }
}

void Console::print_progress(const std::string& desc, int current, int total) {
    const int bar_width = 40;
    const int desc_width = 8; // Fixed width for description
    float progress = static_cast<float>(current) / total;
    int pos = static_cast<int>(bar_width * progress);

    std::cout << "\r ";
    std::cout << std::left << std::setw(desc_width) << desc.substr(0, desc_width);
    std::cout << "[";

    // green filled area
    std::cout << "\033[32m";
    for (int i = 0; i < pos; ++i) {
        std::cout << "━";
    }

    // red unfilled area
    std::cout << "\033[31m";
    for (int i = pos; i < bar_width; ++i) {
        std::cout << "━";
    }

    std::cout << "\033[0m] "; // reset color, close bracket
    std::cout << static_cast<int>(progress * 100) << "% "
              << current << "/" << total;
    std::cout.flush();
}
