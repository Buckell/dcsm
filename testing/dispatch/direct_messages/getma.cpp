//
// Created by maxng on 4/1/2024.
//

#include <gtest/gtest.h>

#include <dcsm.hpp>

struct getma_interface final : dcsm::dispatch_interface {
    std::vector<dcsm::address_pack> addresses;
    bool received = false;

    void dcsm_getma(dcsm::command_context &a_ctx, std::vector<dcsm::address_pack> const &a_addresses) override {
        received = true;

        EXPECT_EQ(a_addresses, addresses);
    }
};

TEST(dispatch_direct_messages, getma) {
    getma_interface itf;
    dcsm::dispatch dsp(itf);

    itf.addresses.emplace_back(1,   21);
    itf.addresses.emplace_back(42,  21);
    itf.addresses.emplace_back(324, 21);
    itf.addresses.emplace_back(100, 21);
    itf.addresses.emplace_back(345, 21);
    itf.addresses.emplace_back(500, 21);
    itf.addresses.emplace_back(433, 21);
    itf.addresses.emplace_back(456, 21);
    itf.addresses.emplace_back(43,  21);

    // Message buffer.
    std::vector<uint8_t> data;
    data.resize(5 /* header */ + (itf.addresses.size() * 4) /* addresses */);
    uint8_t* buf = data.data();

    // Identifying byte.
    buf[0] = 0x00;

    // Copy message header into buffer.
    dcsm::message_header const header { 0x0016, static_cast<uint16_t>(data.size() - 5) };
    memcpy(buf + 1, &header, sizeof(header));

    // Copy pairs into buffer.
    for (size_t i = 0; i < itf.addresses.size(); ++i) {
        const auto& address = itf.addresses[i];
        auto* buf_pos = buf + 5 + (i * 4);

        memcpy(buf_pos + 0, &address.first,  sizeof(uint16_t));
        memcpy(buf_pos + 2, &address.second, sizeof(uint16_t));
    }

    EXPECT_EQ(dsp.process_message(buf), dcsm::dispatch_status::success);

    EXPECT_TRUE(itf.received);
}