//
// Created by maxng on 4/1/2024.
//

#include <gtest/gtest.h>

#include "dcsm.hpp"

static std::pair<std::string, std::string> test_values[] {
    { "1/20 thru 1/40 offset 5", "50%" },
    { "1", "40" }
};

struct cmd_mset_interface final : dcsm::dispatch_interface {
    bool received = false;
    size_t universes = 0;
    dcsm::address_range range;
    uint8_t value = 0;

    void dcsm_setmtv(dcsm::command_context &a_ctx, uint16_t const a_universe, uint8_t const a_value, dcsm::universe_mask const &a_mask) override {
        received = true;
        ++universes;

        EXPECT_EQ(a_value, value);
        EXPECT_EQ(range[a_universe], a_mask);
    }
};

TEST(dispatch_commands, mset) {
    cmd_mset_interface itf;
    dcsm::dispatch dsp(itf);

    for (auto const& test_value : test_values) {
        itf.received = false;
        itf.universes = 0;

        itf.range = dcsm::parse_address_range(test_value.first);
        itf.value = dcsm::parse_value(test_value.second);

        dsp.process_command("mset " + test_value.first + " @ " + test_value.second);

        EXPECT_EQ(itf.universes, itf.range.size());
        EXPECT_TRUE(itf.received);
    }
}