//
// Created by maxng on 4/1/2024.
//

#include <gtest/gtest.h>

#include <dcsm.hpp>

static constexpr uint16_t source_universe  = 5;
static constexpr uint16_t destination_universe = 3;

struct copy_interface final : dcsm::dispatch_interface {
    bool received = false;

    void dcsm_copy(dcsm::command_context &a_ctx, uint16_t const a_source_universe, uint16_t const a_destination_universe) override {
        received = true;

        EXPECT_EQ(a_source_universe,      source_universe     );
        EXPECT_EQ(a_destination_universe, destination_universe);
    }
};

TEST(dispatch_direct_messages, copy) {
    copy_interface itf;
    dcsm::dispatch dsp(itf);

    // Message buffer.
    std::vector<uint8_t> data;
    data.resize(5 /* header */ + 2 /* universe number */ + 2 /* universe number */);
    uint8_t* buf = data.data();

    // Identifying byte.
    buf[0] = 0x00;

    // Copy message header into buffer.
    dcsm::message_header const header { 0x0011, static_cast<uint16_t>(data.size() - 5) };
    memcpy(buf + 1, &header, sizeof(header));

    // Copy universe numbers into buffer.
    memcpy(buf + 5, &source_universe,      sizeof(source_universe)     );
    memcpy(buf + 7, &destination_universe, sizeof(destination_universe));

    EXPECT_EQ(dsp.process_message(buf), dcsm::dispatch_status::success);

    EXPECT_TRUE(itf.received);
}