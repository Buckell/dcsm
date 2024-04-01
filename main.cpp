#include <iostream>

#include <dcsm.hpp>

class itf : public dcsm::dispatch_interface {
    void dcsm_setutv(dcsm::command_context &a_ctx, uint16_t a_universe, uint8_t a_value, dcsm::universe_mask const &a_mask) override {
        std::cout << a_universe << " " << (int)a_value << std::endl;
    }

    void dcsm_patch(dcsm::command_context &a_ctx, uint16_t a_input_universe, uint16_t a_output_universe, uint16_t a_mask_universe) override {

    }

    void dcsm_getma(dcsm::command_context &a_ctx, std::vector<dcsm::address_pack> const &a_addresses) override {

    }
};

int main()
{
    itf interface;

    dcsm::dispatch dsp(interface);

    dsp.process_command("mget 1/500 thru 2/500");

    return 0;
}
