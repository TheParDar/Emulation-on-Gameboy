#include "timer.hpp"
#include "memory.hpp"

namespace gb {

Timer::Timer(Memory& mem) : mem_(mem) {}

void Timer::reset() {
    divider_ = 0;
    tima_ = 0;
    tma_ = 0;
    tac_ = 0;
    prev_bit_ = false;
}

int Timer::tac_bit() const {
    static const int bits[] = {9, 3, 5, 7};
    return bits[tac_ & 0x03];
}

void Timer::tick_tima() {
    ++tima_;
    if (tima_ == 0) {
        tima_ = tma_;
        mem_.request_interrupt(2);
    }
}

void Timer::step(int machine_cycles) {
    for (int i = 0; i < machine_cycles; ++i) {
        const int bit = tac_bit();
        const bool enabled = (tac_ & 0x04) != 0;
        const bool new_bit = enabled && (((divider_ >> bit) & 1) != 0);

        if (new_bit && !prev_bit_)
            tick_tima();

        prev_bit_ = new_bit;
        ++divider_;
    }
}

void Timer::write_div(u8) {
    divider_ = 0;
    prev_bit_ = false;
}

void Timer::write_tima(u8 value) { tima_ = value; }
void Timer::write_tma(u8 value) { tma_ = value; }
void Timer::write_tac(u8 value) { tac_ = value & 0x07; }

} // namespace gb
