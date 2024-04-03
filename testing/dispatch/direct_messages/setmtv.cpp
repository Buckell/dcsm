//
// Created by maxng on 4/1/2024.
//

#include <gtest/gtest.h>

#include <dcsm.hpp>

static constexpr uint16_t         universe_number = 5;
static constexpr uint8_t          value           = 201;

struct setmtv_interface final : dcsm::dispatch_interface {
    bool received = false;
    std::bitset<512> bitmask;

    void dcsm_setmtv(dcsm::command_context &a_ctx, uint16_t const a_universe, uint8_t const a_value, dcsm::universe_mask const &a_mask) override {
        received = true;

        EXPECT_EQ(a_universe, universe_number);
        EXPECT_EQ(a_value,    value          );
        EXPECT_EQ(a_mask,     bitmask        );
    }
};

TEST(dispatch_direct_messages, setmtv) {
    setmtv_interface itf;
    dcsm::dispatch dsp(itf);

    itf.bitmask.set(10 );
    itf.bitmask.set(231);
    itf.bitmask.set(423);
    itf.bitmask.set(1  );
    itf.bitmask.set(23 );
    itf.bitmask.set(51 );
    itf.bitmask.set(63 );
    itf.bitmask.set(365);

    // Message buffer.
    std::vector<uint8_t> data;
    data.resize(5 /* header */ + 2 /* universe number */ + 1 /* value */ + 64 /* mask */);
    uint8_t* buf = data.data();

    // Identifying byte.
    buf[0] = 0x00;

    // Copy message header into buffer.
    dcsm::message_header const header { 0x0013, static_cast<uint16_t>(data.size() - 5) };
    memcpy(buf + 1, &header, sizeof(header));

    // Copy universe numbers into buffer.
    memcpy(buf + 5, &universe_number, sizeof(universe_number));
    memcpy(buf + 7, &value,           sizeof(value)          );

    // Copy mask into buffer.
    dcsm::bitset_to_bytes(buf + 8, itf.bitmask);

    EXPECT_EQ(dsp.process_message(buf), dcsm::dispatch_status::success);

    EXPECT_TRUE(itf.received);
}