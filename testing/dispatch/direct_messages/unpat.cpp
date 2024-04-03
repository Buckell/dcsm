//
// Created by maxng on 4/1/2024.
//

#include <gtest/gtest.h>

#include <dcsm.hpp>

static constexpr uint16_t universe_number = 5;

struct unpat_interface final : dcsm::dispatch_interface {
    bool received = false;

    void dcsm_unpat(dcsm::command_context &a_ctx, uint16_t const a_universe) override {
        received = true;

        EXPECT_EQ(a_universe, universe_number);
    }
};

TEST(dispatch_direct_messages, unpat) {
    unpat_interface itf;
    dcsm::dispatch dsp(itf);

    // Message buffer.
    std::vector<uint8_t> data;
    data.resize(5 /* header */ + 2 /* universe number */);
    uint8_t* buf = data.data();

    // Identifying byte.
    buf[0] = 0x00;

    // Copy message header into buffer.
    dcsm::message_header const header { 0x000F, static_cast<uint16_t>(data.size() - 5) };
    memcpy(buf + 1, &header, sizeof(header));

    // Copy universe number into buffer.
    memcpy(buf + 5, &universe_number, sizeof(universe_number));

    EXPECT_EQ(dsp.process_message(buf), dcsm::dispatch_status::success);

    EXPECT_TRUE(itf.received);
}