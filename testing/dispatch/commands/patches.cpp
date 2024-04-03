//
// Created by maxng on 4/1/2024.
//

#include <gtest/gtest.h>

#include "dcsm.hpp"

struct cmd_patches_interface final : dcsm::dispatch_interface {
    bool received = false;

    void dcsm_listp(dcsm::command_context &a_ctx) override {
        received = true;
    }
};

TEST(dispatch_commands, patches) {
    cmd_patches_interface itf;
    dcsm::dispatch dsp(itf);

    dsp.process_command("patches");

    EXPECT_TRUE(itf.received);
}