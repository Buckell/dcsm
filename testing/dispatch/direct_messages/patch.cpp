//
// Created by maxng on 4/1/2024.
//

#include <gtest/gtest.h>

#include <dcsm.hpp>

static constexpr uint16_t input_universe  = 5;
static constexpr uint16_t output_universe = 3;
static constexpr uint16_t mask_universe   = 1;

struct patch_interface final : dcsm::dispatch_interface {
    bool received = false;

    void dcsm_patch(dcsm::command_context &a_ctx, uint16_t const a_input_universe, uint16_t const a_output_universe, uint16_t const a_mask_universe) override {
        received = true;

        EXPECT_EQ(a_input_universe,  input_universe );
        EXPECT_EQ(a_output_universe, output_universe);
        EXPECT_EQ(a_mask_universe,   mask_universe  );
    }
};

TEST(dispatch_direct_messages, patch) {
    patch_interface itf;
    dcsm::dispatch dsp(itf);

    // Message buffer.
    std::vector<uint8_t> data;
    data.resize(5 /* header */ + 2 /* universe number */ + 2 /* universe number */ + 2 /* universe number */);
    uint8_t* buf = data.data();

    // Identifying byte.
    buf[0] = 0x00;

    // Copy message header into buffer.
    dcsm::message_header const header { 0x000E, static_cast<uint16_t>(data.size() - 5) };
    memcpy(buf + 1, &header, sizeof(header));

    // Copy universe numbers into buffer.
    memcpy(buf + 5, &input_universe,  sizeof(input_universe) );
    memcpy(buf + 7, &output_universe, sizeof(output_universe));
    memcpy(buf + 9, &mask_universe,   sizeof(mask_universe)  );

    EXPECT_EQ(dsp.process_message(buf), dcsm::dispatch_status::success);

    EXPECT_EQ(itf.received, true);
}