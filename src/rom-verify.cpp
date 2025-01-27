#include "console.hpp"

int main(int argc, char* argv[]) {
    Console* nes = new Console("nes");
    nes->find_roms();
    nes->process_roms();
    nes->print_results();
    return 0;
}
