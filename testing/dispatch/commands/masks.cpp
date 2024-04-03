//
// Created by maxng on 4/1/2024.
//

#include <gtest/gtest.h>

#include "dcsm.hpp"

struct cmd_masks_interface final : dcsm::dispatch_interface {
    bool received = false;

    void dcsm_listmu(dcsm::command_context &a_ctx) override {
        received = true;
    }
};

TEST(dispatch_commands, masks) {
    cmd_masks_interface itf;
    dcsm::dispatch dsp(itf);

    dsp.process_command("masks");

    EXPECT_TRUE(itf.received);
}