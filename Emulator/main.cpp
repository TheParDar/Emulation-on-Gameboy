#include "cartridge.hpp"
#include "cpu.hpp"
#include "memory.hpp"
#include "ppu.hpp"
#include "timer.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#if GB_USE_SDL
#include <SDL.h>
#endif

namespace {

constexpr int CYCLES_PER_FRAME = 70224; // ~59.73 Hz on DMG

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: gb_emulator <rom.gb> [--headless]\n";
        return 1;
    }

    const std::string rom_path = argv[1];
    const bool headless = (argc >= 3 && std::string(argv[2]) == "--headless");

    gb::Cartridge cart;
    if (!cart.load_from_file(rom_path)) {
        return 1;
    }

    gb::Memory mem(cart);
    gb::CPU cpu(mem);
    gb::PPU ppu(mem);
    gb::Timer timer(mem);
    mem.connect(&cpu, &ppu, &timer);

    cpu.reset();
    ppu.reset();
    timer.reset();

    std::cout << "Game Boy emulator\n";
    std::cout << "ROM: " << cart.title() << '\n';
    std::cout << "PC=0x" << std::hex << cpu.regs().pc << std::dec << '\n';

#if GB_USE_SDL
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;

    if (!headless) {
        window = SDL_CreateWindow(
            ("GB: " + cart.title()).c_str(),
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            gb::PPU::SCREEN_W * 3, gb::PPU::SCREEN_H * 3,
            SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        texture = SDL_CreateTexture(
            renderer, SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            gb::PPU::SCREEN_W, gb::PPU::SCREEN_H);
    }
#endif

    int frame_cycles = 0;
    bool running = true;

#if GB_USE_SDL
    if (!headless) {
        while (running) {
            SDL_Event e;
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT)
                    running = false;
            }

            while (frame_cycles < CYCLES_PER_FRAME && running) {
                const int c = cpu.step();
                timer.step(c);
                ppu.step(c);
                frame_cycles += c;
            }
            frame_cycles = 0;

            SDL_UpdateTexture(texture, nullptr, ppu.framebuffer(),
                              gb::PPU::SCREEN_W * sizeof(gb::u32));
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    } else
#endif
    {
        for (int i = 0; i < 10'000'000; ++i) {
            const int c = cpu.step();
            timer.step(c);
            ppu.step(c);
            if (i % 1'000'000 == 0) {
                std::cout << "PC=0x" << std::hex << cpu.regs().pc
                          << " cycles=" << std::dec << cpu.total_cycles() << '\n';
            }
        }
    }

    std::cout << "Done. Total cycles: " << cpu.total_cycles() << '\n';
    return 0;
}

