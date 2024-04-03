//
// Created by maxng on 4/1/2024.
//

#include <gtest/gtest.h>

#include <dcsm.hpp>

static constexpr uint16_t universe_number = 3;

static constexpr std::pair<uint16_t, uint8_t> values[] = {
    { 5,   100 },
    { 10,  110 },
    { 30,  20  },
    { 55,  255 },
    { 100, 70  },
    { 300, 44  },
    { 410, 120 },
    { 220, 30  },
};

struct setu_interface final : dcsm::dispatch_interface {
    std::vector<uint8_t> universe;
    bool received = false;

    void dcsm_setu(dcsm::command_context &a_ctx, uint16_t const a_universe, uint8_t const *a_data) override {
        received = true;

        EXPECT_EQ(a_universe, universe_number);

        std::vector<uint8_t> data;
        data.resize(512);
        memcpy(data.data(), a_data, 512);

        EXPECT_EQ(universe, data);
    }
};

TEST(dispatch_direct_messages, setu) {
    setu_interface itf;
    dcsm::dispatch dsp(itf);

    // Initialize universe buffer.
    itf.universe.resize(512, 0);

    for (auto const& pair : values) {
        itf.universe[pair.first] = pair.second;
    }

    // Message buffer.
    std::vector<uint8_t> data;
    data.resize(5 /* header */ + 2 /* universe number */ + 512 /* universe data */);
    uint8_t* buf = data.data();

    // Identifying byte.
    buf[0] = 0x00;

    // Copy message header into buffer.
    dcsm::message_header const header { 0x0002, static_cast<uint16_t>(data.size() - 5) };
    memcpy(buf + 1, &header, sizeof(header));

    // Copy universe number into buffer.
    memcpy(buf + 5, &universe_number, sizeof(universe_number));

    // Copy universe data into buffer.
    memcpy(buf + 7, itf.universe.data(), 512);

    EXPECT_EQ(dsp.process_message(buf), dcsm::dispatch_status::success);

    EXPECT_TRUE(itf.received);
}