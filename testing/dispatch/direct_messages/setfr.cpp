//
// Created by maxng on 4/1/2024.
//

#include <gtest/gtest.h>

#include <dcsm.hpp>

static constexpr uint8_t framerate = 44;

struct setfr_interface final : dcsm::dispatch_interface {
    bool received = false;

    void dcsm_setfr(dcsm::command_context &a_ctx, uint8_t const a_framerate) override {
        received = true;

        EXPECT_EQ(a_framerate, framerate);
    }
};

TEST(dispatch_direct_messages, setfr) {
    setfr_interface itf;
    dcsm::dispatch dsp(itf);

    // Message buffer.
    std::vector<uint8_t> data;
    data.resize(5 /* header */ + 1 /* framerate */);
    uint8_t* buf = data.data();

    // Identifying byte.
    buf[0] = 0x00;

    // Copy message header into buffer.
    dcsm::message_header const header { 0x0005, static_cast<uint16_t>(data.size() - 5) };
    memcpy(buf + 1, &header, sizeof(header));

    // Copy framerate into buffer.
    memcpy(buf + 5, &framerate, sizeof(framerate));

    EXPECT_EQ(dsp.process_message(buf), dcsm::dispatch_status::success);

    EXPECT_TRUE(itf.received);
}