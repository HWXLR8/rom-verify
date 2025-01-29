#include "console.hpp"

int main(int argc, char* argv[]) {
    Config* conf = new Config("rom.yaml");
    std::vector<Console*> consoles;
    Console* nes = new Console("nes", conf);
    nes->parse_dat();
    nes->find_roms();
    nes->process_roms();
    return 0;
}
