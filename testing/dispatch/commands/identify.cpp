//
// Created by maxng on 4/1/2024.
//

#include <gtest/gtest.h>

#include "dcsm.hpp"

struct cmd_identify_interface final : dcsm::dispatch_interface {
    bool received = false;

    void dcsm_id(dcsm::command_context &a_ctx) override {
        received = true;
    }
};

TEST(dispatch_commands, identify) {
    cmd_identify_interface itf;
    dcsm::dispatch dsp(itf);

    dsp.process_command("identify");

    EXPECT_TRUE(itf.received);
}