#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace runtime_core {
// Selects data records, not CPU or OS threads. No opaque engine context.
class Contexts {
    struct State {
        uint32_t id;
        uint32_t error = 0;
        uint64_t allocated = 0;
        std::array<uint32_t, 64> slots{};
    };
public:
    explicit Contexts(const std::vector<uint32_t>& identifiers) {
        if (identifiers.empty() || identifiers.size() > 64)
            throw std::invalid_argument("invalid context count");
        for (uint32_t id : identifiers) {
            if (!id) throw std::invalid_argument("zero context identifier");
            for (const auto& prior : states_)
                if (prior.id == id) throw std::invalid_argument("duplicate context identifier");
            states_.push_back(State{id});
        }
    }
    bool select(uint32_t id) {
        for (size_t i = 0; i < states_.size(); ++i)
            if (states_[i].id == id) { current_ = i; return true; }
        return false;
    }
    uint32_t id() const { return states_[current_].id; }
    uint32_t error() const { return states_[current_].error; }
    void setError(uint32_t value) { states_[current_].error = value; }
    bool allocateSlot(uint32_t slot) {
        if (slot >= 64 || (states_[current_].allocated & (uint64_t(1) << slot))) return false;
        states_[current_].allocated |= uint64_t(1) << slot;
        states_[current_].slots[slot] = 0;
        return true;
    }
    bool releaseSlot(uint32_t slot) {
        if (!knownSlot(slot)) return false;
        states_[current_].allocated &= ~(uint64_t(1) << slot);
        states_[current_].slots[slot] = 0;
        return true;
    }
    bool getSlot(uint32_t slot, uint32_t& output) {
        if (!knownSlot(slot)) return false;
        output = states_[current_].slots[slot];
        states_[current_].error = 0;
        return true;
    }
    bool setSlot(uint32_t slot, uint32_t value) {
        if (!knownSlot(slot)) return false;
        states_[current_].slots[slot] = value;
        return true;
    }
private:
    bool knownSlot(uint32_t slot) const {
        return slot < 64 && (states_[current_].allocated & (uint64_t(1) << slot));
    }
    std::vector<State> states_;
    size_t current_ = 0;
};
} // namespace runtime_core
