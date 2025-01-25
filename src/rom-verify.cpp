#include <filesystem>
#include <future>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <tinyxml2.h>
#include <yaml-cpp/yaml.h>
#include <zip.h>
#include <zlib.h>

std::mutex dats_mtx;
std::mutex romsets_mtx;

void find_roms(const std::string& romdir, std::vector<std::string>& roms) {
    for (const auto& f : std::filesystem::recursive_directory_iterator(romdir)) {
        const auto& path = f.path();

        if (f.is_regular_file()) {
            roms.push_back(path.string());
        }
    }
}

void parse_dat(const std::string& dat_path,
               const std::string& cat,
               std::unordered_map<std::string, std::string>& rom_crc) {

    tinyxml2::XMLDocument dat;
    dat.LoadFile(dat_path.c_str());
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

void check_rom(const std::string& rom,
               const std::string& cat,
               std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& dats,
               std::unordered_map<std::string, std::unordered_set<std::string>>& romsets) {
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
    auto it = dats[cat].find(crc_str);
    if (it != dats[cat].end()) {
        // std::cout << crc_str << " found" << std::endl;
        romsets_mtx.lock();
        romsets[cat].insert(crc_str);
        romsets_mtx.unlock();
        dats_mtx.lock();
        dats[cat].erase(it);
        dats_mtx.unlock();
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> cats = {
        "Japan",
        "USA",
        // "Europe",
        // "China",
        // "Asia",
        // "(Beta)"
    };
    // cat : (crc : rom)
    std::unordered_map<std::string,
                       std::unordered_map<std::string, std::string>> dats;
    // cat : romset
    std::unordered_map<std::string,
                       std::unordered_set<std::string>> romsets;
    std::unordered_map<std::string, int> dat_sizes;
    // split the dat into seperate dats for each cat
    for (const auto& cat : cats) {
        std::unordered_map<std::string, std::string> rom_crc;
        parse_dat("dat/nes.dat", cat, rom_crc);
        // capture the original size of the dat before we remove items
        dat_sizes[cat] = rom_crc.size();
        dats[cat] = rom_crc;
        // initialize empty romset for each cat
        romsets[cat] = std::unordered_set<std::string>();
    }

    // parse config
    YAML::Node conf = YAML::LoadFile("rom.yaml");
    YAML::Node paths = conf["console"]["nes"]["path"];
    std::vector<std::string> roms;
    std::string rompath;
    if (paths && paths.IsSequence() && paths.size() > 0) {
        for (const auto& path : paths) {
            std::string rompath = path.as<std::string>();
            find_roms(rompath, roms);
        }
    } else {
        throw std::runtime_error("no valid rom path found, check rom.yaml");
    }

    for (const auto& cat : cats) {
        std::vector<std::future<void>> futures;
        for (const auto& rom : roms) {
            futures.push_back
                (std::async
                 (std::launch::async, check_rom, rom, cat, std::ref(dats), std::ref(romsets)));
        }
        for (auto& f : futures) {
            f.get();
        }
    }

    // print results, write missing roms to MISSING
    std::unordered_set<std::string> missing;
    for (const auto& cat : cats) {
        const int romset_size = romsets[cat].size();
        const int dat_size = dat_sizes[cat];
        const double completion_rate = ((double)romset_size/dat_size * 100);

        std::cout << std::setw(10) << std::left << cat
                  << std::setw(8) << std::right << std::setprecision(4) << completion_rate << "%"
                  << std::setw(10) << romset_size << "/" << dat_size << std::endl;

        for (const auto& rom_map : dats[cat]) {
            std::string rom = "(" + rom_map.first + ") " + rom_map.second;
            missing.insert(rom);
        }
    }
    // print missing roms
    // for (const auto& rom : missing) {
    //     std::cout << rom << std::endl;
    // }

    return 0;
}
