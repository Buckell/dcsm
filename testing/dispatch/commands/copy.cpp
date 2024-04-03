//
// Created by maxng on 4/1/2024.
//

#include <gtest/gtest.h>

#include "dcsm.hpp"

static constexpr std::pair<size_t, size_t> test_values[] {
    { 1, 2 },
    { 100, 200 }
};

struct cmd_copy_interface final : dcsm::dispatch_interface {
    bool received = false;
    uint16_t source_universe = 0;
    uint16_t destination_universe = 0;

    void dcsm_copy(dcsm::command_context &a_ctx, uint16_t const a_source_universe, uint16_t const a_destination_universe) override {
        received = true;
        source_universe = a_source_universe;
        destination_universe = a_destination_universe;
    }
};

TEST(dispatch_commands, copy) {
    cmd_copy_interface itf;
    dcsm::dispatch dsp(itf);

    for (auto const& test_value : test_values) {
        itf.received = false;

        dsp.process_command("copy " + std::to_string(test_value.first) + " to " + std::to_string(test_value.second));

        EXPECT_EQ(itf.source_universe, test_value.first);
        EXPECT_EQ(itf.destination_universe, test_value.second);
        EXPECT_TRUE(itf.received);
    }
}