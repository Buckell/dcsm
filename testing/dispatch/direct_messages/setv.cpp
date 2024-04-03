//
// Created by maxng on 4/1/2024.
//

#include <gtest/gtest.h>

#include <dcsm.hpp>

struct setv_interface final : dcsm::dispatch_interface {
    std::vector<std::pair<dcsm::address_pack, uint8_t>> pairs;
    bool received = false;

    void dcsm_setv(dcsm::command_context &a_ctx, std::vector<std::pair<dcsm::address_pack, uint8_t>> const &a_pairs) override {
        received = true;

        EXPECT_EQ(a_pairs, pairs);
    }
};

TEST(dispatch_direct_messages, setv) {
    setv_interface itf;
    dcsm::dispatch dsp(itf);

    itf.pairs.emplace_back(dcsm::address_pack{ 1, 20  }, 120);
    itf.pairs.emplace_back(dcsm::address_pack{ 2, 300 }, 20 );
    itf.pairs.emplace_back(dcsm::address_pack{ 1, 1   }, 131);
    itf.pairs.emplace_back(dcsm::address_pack{ 1, 201 }, 193);
    itf.pairs.emplace_back(dcsm::address_pack{ 3, 213 }, 112);
    itf.pairs.emplace_back(dcsm::address_pack{ 3, 324 }, 33 );
    itf.pairs.emplace_back(dcsm::address_pack{ 4, 123 }, 44 );
    itf.pairs.emplace_back(dcsm::address_pack{ 4, 454 }, 55 );

    // Message buffer.
    std::vector<uint8_t> data;
    data.resize(5 /* header */ + (itf.pairs.size() * 5) /* pairs */);
    uint8_t* buf = data.data();

    // Identifying byte.
    buf[0] = 0x00;

    // Copy message header into buffer.
    dcsm::message_header const header { 0x0003, static_cast<uint16_t>(data.size() - 5) };
    memcpy(buf + 1, &header, sizeof(header));

    // Copy pairs into buffer.
    for (size_t i = 0; i < itf.pairs.size(); ++i) {
        const auto& pair = itf.pairs[i];
        auto* buf_pos = buf + 5 + (i * 5);

        memcpy(buf_pos + 0, &pair.first.first,  sizeof(uint16_t));
        memcpy(buf_pos + 2, &pair.first.second, sizeof(uint16_t));
        memcpy(buf_pos + 4, &pair.second,       sizeof(uint8_t));
    }

    EXPECT_EQ(dsp.process_message(buf), dcsm::dispatch_status::success);

    EXPECT_TRUE(itf.received);
}