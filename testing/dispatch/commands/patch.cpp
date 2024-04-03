//
// Created by maxng on 4/1/2024.
//

#include <gtest/gtest.h>

#include "dcsm.hpp"

static constexpr std::tuple<size_t, size_t, size_t> test_values[] {
    { 1, 2, 3 },
    { 100, 200, 102 }
};

struct cmd_patch_interface final : dcsm::dispatch_interface {
    bool received = false;
    uint16_t input_universe = 0;
    uint16_t output_universe = 0;
    uint16_t mask_universe = 0;

    void dcsm_patch(dcsm::command_context &a_ctx, uint16_t const a_input_universe, uint16_t const a_output_universe, uint16_t const a_mask_universe) override {
        received = true;
        input_universe = a_input_universe;
        output_universe = a_output_universe;
        mask_universe = a_mask_universe;
    }
};

TEST(dispatch_commands, patch) {
    cmd_patch_interface itf;
    dcsm::dispatch dsp(itf);

    for (auto const& test_value : test_values) {
        itf.received = false;

        dsp.process_command("patch " + std::to_string(std::get<0>(test_value)) + " to " + std::to_string(std::get<1>(test_value)) + " mask " + std::to_string(std::get<2>(test_value)));

        EXPECT_EQ(itf.input_universe, std::get<0>(test_value));
        EXPECT_EQ(itf.output_universe, std::get<1>(test_value));
        EXPECT_EQ(itf.mask_universe, std::get<2>(test_value));
        EXPECT_TRUE(itf.received);
    }
}