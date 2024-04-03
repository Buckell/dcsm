//
// Created by maxng on 4/1/2024.
//

#include <gtest/gtest.h>

#include "dcsm.hpp"

static uint16_t test_values[] {
    324,
    123,
    3,
    4
};

struct cmd_deletemask_interface final : dcsm::dispatch_interface {
    bool received = false;
    uint16_t universe = 0;

    void dcsm_delmu(dcsm::command_context &a_ctx, uint16_t const a_universe) override {
        received = true;
        universe = a_universe;
    }
};

TEST(dispatch_commands, deletemask) {
    cmd_deletemask_interface itf;
    dcsm::dispatch dsp(itf);

    for (auto const& test_value : test_values) {
        itf.received = false;

        dsp.process_command("deletemask " + std::to_string(test_value));

        EXPECT_EQ(itf.universe, test_value);
        EXPECT_TRUE(itf.received);
    }
}