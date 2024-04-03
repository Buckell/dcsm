//
// Created by maxng on 4/1/2024.
//

#include <gtest/gtest.h>

#include "dcsm.hpp"

static std::string test_values[] {
    "1/20 thru 1/40 offset 5",
    "1",
};

struct cmd_mget_interface final : dcsm::dispatch_interface {
    bool received = false;
    size_t universes = 0;
    dcsm::address_range range;

    void dcsm_getma(dcsm::command_context &a_ctx, std::vector<dcsm::address_pack> const &a_addresses) override {
        received = true;
        ++universes;

        size_t range_count = 0;

        for (auto const& value : range) {
            for (size_t i = 0; i < 512; ++i) {
                range_count += value.second.test(i);
            }
        }

        EXPECT_EQ(range_count, a_addresses.size());

        for (auto const& pair : a_addresses) {
            EXPECT_TRUE(range[pair.first].test(pair.second - 1));
        }
    }

};

TEST(dispatch_commands, mget) {
    cmd_mget_interface itf;
    dcsm::dispatch dsp(itf);

    for (auto const& test_value : test_values) {
        itf.received = false;
        itf.universes = 0;

        itf.range = dcsm::parse_address_range(test_value);

        dsp.process_command("mget " + test_value);

        EXPECT_EQ(itf.universes, itf.range.size());
        EXPECT_TRUE(itf.received);
    }
}