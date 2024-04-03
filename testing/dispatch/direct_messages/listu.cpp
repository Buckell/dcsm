//
// Created by maxng on 4/1/2024.
//

#include <gtest/gtest.h>

#include <dcsm.hpp>

struct listu_interface final : dcsm::dispatch_interface {
    bool received = false;

    void dcsm_listu(dcsm::command_context &a_ctx) override {
        received = true;
    }
};

TEST(dispatch_direct_messages, listu) {
    listu_interface itf;
    dcsm::dispatch dsp(itf);

    // Message buffer.
    std::vector<uint8_t> data;
    data.resize(5 /* header */);
    uint8_t* buf = data.data();

    // Identifying byte.
    buf[0] = 0x00;

    // Copy message header into buffer.
    dcsm::message_header const header { 0x0014, static_cast<uint16_t>(data.size() - 5) };
    memcpy(buf + 1, &header, sizeof(header));

    EXPECT_EQ(dsp.process_message(buf), dcsm::dispatch_status::success);

    EXPECT_TRUE(itf.received);
}