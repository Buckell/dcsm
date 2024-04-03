//
// Created by maxng on 4/1/2024.
//

#include <gtest/gtest.h>

#include <dcsm.hpp>

static constexpr uint16_t universe_number = 10;

struct setmv_interface final : dcsm::dispatch_interface {
    std::vector<std::tuple<uint16_t, bool, uint8_t>> pairs;
    bool received = false;

    void dcsm_setmv(dcsm::command_context &a_ctx, uint16_t const a_universe, std::vector<std::tuple<uint16_t, bool, uint8_t>> const &a_pairs) override {
        received = true;

        EXPECT_EQ(a_universe, universe_number);
        EXPECT_EQ(a_pairs, pairs);
    }
};

TEST(dispatch_direct_messages, setmv) {
    setmv_interface itf;
    dcsm::dispatch dsp(itf);

    itf.pairs.emplace_back(410, true,  211);
    itf.pairs.emplace_back(102, false, 211);
    itf.pairs.emplace_back(1,   true,  211);
    itf.pairs.emplace_back(410, true,  211);
    itf.pairs.emplace_back(129, false, 211);
    itf.pairs.emplace_back(23,  true,  211);
    itf.pairs.emplace_back(54,  true,  211);
    itf.pairs.emplace_back(510, true,  211);

    // Message buffer.
    std::vector<uint8_t> data;
    data.resize(5 /* header */ + 2 /* universe number */ + (itf.pairs.size() * 4) /* pairs */);
    uint8_t* buf = data.data();

    // Identifying byte.
    buf[0] = 0x00;

    // Copy message header into buffer.
    dcsm::message_header const header { 0x000B, static_cast<uint16_t>(data.size() - 5) };
    memcpy(buf + 1, &header, sizeof(header));

    memcpy(buf + 5, &universe_number, sizeof(universe_number));

    // Copy pairs into buffer.
    for (size_t i = 0; i < itf.pairs.size(); ++i) {
        const auto& pair = itf.pairs[i];
        auto* buf_pos = buf + 5 + 2 + (i * 4);

        memcpy(buf_pos + 0, &std::get<0>(pair), sizeof(uint16_t));
        memcpy(buf_pos + 2, &std::get<1>(pair), sizeof(uint8_t));
        memcpy(buf_pos + 3, &std::get<2>(pair), sizeof(uint8_t));
    }

    EXPECT_EQ(dsp.process_message(buf), dcsm::dispatch_status::success);

    EXPECT_TRUE(itf.received);
}