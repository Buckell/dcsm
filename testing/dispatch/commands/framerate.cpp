//
// Created by maxng on 4/1/2024.
//

#include <gtest/gtest.h>

#include "dcsm.hpp"

static constexpr uint8_t test_values[] {
    44,
    23,
    3,
    4,
    20
};

struct cmd_framerate_interface final : dcsm::dispatch_interface {
    bool received = false;
    uint8_t framerate = 0;

    void dcsm_setfr(dcsm::command_context &a_ctx, uint8_t const a_framerate) override {
        received = true;
        framerate = a_framerate;
    }

    void dcsm_getfr(dcsm::command_context &a_ctx) override {
        received = true;
    }
};

TEST(dispatch_commands, framerate) {
    cmd_framerate_interface itf;
    dcsm::dispatch dsp(itf);

    dsp.process_command("framerate");
    EXPECT_TRUE(itf.received);

    for (auto const& test_value : test_values) {
        itf.received = false;

        dsp.process_command("framerate " + std::to_string(test_value));

        EXPECT_EQ(itf.framerate, test_value);
        EXPECT_TRUE(itf.received);
    }
}