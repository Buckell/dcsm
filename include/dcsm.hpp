//
// Created by maxng on 3/24/2024.
//

#ifndef DCSM_HPP
#define DCSM_HPP

#include <algorithm>
#include <map>
#include <vector>
#include <bitset>
#include <string>
#include <array>
#include <cstring>

namespace dcsm {
    constexpr char version[] = "1.0.0";

    enum class interface_mode {
        command,
        direct_control
    };

    enum class dispatch_status {
        success           = 0x00,
        invalid_body_size = 0x01,
        malformed_syntax  = 0x02,
        invalid_header    = 0x03
    };

    struct command_context {
        interface_mode mode;
    };

    constexpr size_t addresses_per_universe = 512;

    using universe_mask = std::bitset<512>;                  ///< A mask to specify which addresses in a universe are targeted (0 for non-targeted, 1 for targeted).
    using address_range = std::map<uint16_t, universe_mask>; ///< Key: universe number. Value: universe mask (selected addresses).
    using address_pack = std::pair<uint16_t, uint16_t>;      ///< First member: universe number. Second member: local address.

    struct dispatch_interface {
        virtual ~dispatch_interface() = default;

        virtual void dcsm_id    (command_context& a_ctx) {}
        virtual void dcsm_setu  (command_context& a_ctx, uint16_t a_universe, uint8_t const* a_data) {}
        /// pair: address, value
        virtual void dcsm_setv  (command_context& a_ctx, std::vector<std::pair<address_pack, uint8_t>> const& a_pairs) {}
        virtual void dcsm_getu  (command_context& a_ctx, uint16_t a_universe) {}
        virtual void dcsm_setfr (command_context& a_ctx, uint8_t a_framerate) {}
        virtual void dcsm_getfr (command_context& a_ctx) {}
        virtual void dcsm_newmu (command_context& a_ctx, uint16_t a_universe) {}
        virtual void dcsm_listmu(command_context& a_ctx) {}
        virtual void dcsm_delmu (command_context& a_ctx, uint16_t a_universe) {}
        virtual void dcsm_setmu (command_context& a_ctx, uint16_t a_universe, universe_mask const& a_mask, uint8_t const* a_data) {}
        /// tuple: local address, masking, value
        virtual void dcsm_setmv (command_context& a_ctx, uint16_t a_universe, std::vector<std::tuple<uint16_t, bool, uint8_t>> const& a_pairs) {}
        virtual void dcsm_getmu (command_context& a_ctx, uint16_t a_universe) {}
        virtual void dcsm_clrmu (command_context& a_ctx, uint16_t a_universe) {}
        virtual void dcsm_patch (command_context& a_ctx, uint16_t a_input_universe, uint16_t a_output_universe, uint16_t a_mask_universe) {}
        virtual void dcsm_unpat (command_context& a_ctx, uint16_t a_output_universe) {}
        virtual void dcsm_listp (command_context& a_ctx) {}
        virtual void dcsm_copy  (command_context& a_ctx, uint16_t a_source_universe, uint16_t a_destination_universe) {}
        virtual void dcsm_setutv(command_context& a_ctx, uint16_t a_universe, uint8_t a_value, universe_mask const& a_mask) {}
        virtual void dcsm_setmtv(command_context& a_ctx, uint16_t a_universe, uint8_t a_value, universe_mask const& a_mask) {}
        virtual void dcsm_listu (command_context& a_ctx) {}
        virtual void dcsm_geta  (command_context& a_ctx, std::vector<address_pack> const& a_addresses) {}
        virtual void dcsm_getma (command_context& a_ctx, std::vector<address_pack> const& a_addresses) {}
    };

    struct message_header {
        uint16_t opcode;
        uint16_t length;
    };

    /**
     * @brief Cast ambiguous data to a type without undefined behavior.
     *
     * @tparam t_type The type to which to convert.
     *
     * @param a_source The data from which to convert.
     *
     * @return The resultant converted value.
     */
    template <typename t_type>
    t_type bit_cast(void const* a_source) {
        t_type value;
                    // Cast to avoid warning of swapped arguments.
        std::memcpy(reinterpret_cast<void*>(&value), a_source, sizeof(t_type));
        return value;
    }

    class dispatch {
    public:
        using command_handler = dispatch_status (dispatch::*)(command_context&, std::string const&);
        using message_handler = dispatch_status (dispatch::*)(command_context&, message_header, uint8_t const*);

    private:
        dispatch_interface& m_interface;
        std::vector<message_handler> m_message_handlers;
        std::map<std::string, command_handler> m_command_handlers;

    public:
        explicit dispatch(dispatch_interface& a_interface) noexcept :
            m_interface(a_interface)
        {
            initialize_handlers();
        }

        /**
         * @brief Process a human-readable command and dispatch.
         *
         * @param a_command The command plaintext.
         *
         * @return Status of call, either success or an error code.
         */
        dispatch_status process_command(std::string const& a_command) {
            size_t const command_name_end_index = a_command.find_first_of(' ');
            std::string const command_name = a_command.substr(0, command_name_end_index);
            std::string const command_body = command_name_end_index == std::string::npos ? "" : a_command.substr(command_name_end_index + 1);

            command_context ctx{};
            ctx.mode = interface_mode::command;

            return (this->*m_command_handlers[command_name])(ctx, command_body);
        }

        /**
         * @brief Process a direct control interface message and dispatch.
         *
         * @param a_header The header of the message.
         * @param a_body   The body of the message.
         *
         * @return Status of call, either success or an error code.
         */
        dispatch_status process_message(message_header const a_header, uint8_t const* a_body) {
            command_context ctx{};
            ctx.mode = interface_mode::direct_control;

            return (this->*m_message_handlers[a_header.opcode])(ctx, a_header, a_body);
        }

        /**
         * @brief Process a direct control interface message and dispatch.
         *
         * @param a_body The body of the message.
         *
         * @return Status of call, either success or an error code.
         */
        dispatch_status process_message(uint8_t const* a_body) {
            command_context ctx{};
            ctx.mode = interface_mode::direct_control;

            if (*a_body != 0x00) {
                return dispatch_status::invalid_header;
            }

            message_header header{};
            std::memcpy(&header, a_body + 1, sizeof(header));

            return (this->*m_message_handlers[header.opcode])(ctx, header, a_body + 5);
        }

    private:
        void initialize_handlers() {
            m_message_handlers = {
                nullptr,                           // 0x0000
                &dispatch::process_id_message,     // 0x0001
                &dispatch::process_setu_message,   // 0x0002
                &dispatch::process_setv_message,   // 0x0003
                &dispatch::process_getu_message,   // 0x0004
                &dispatch::process_setfr_message,  // 0x0005
                &dispatch::process_getfr_message,  // 0x0006
                &dispatch::process_newmu_message,  // 0x0007
                &dispatch::process_listmu_message, // 0x0008
                &dispatch::process_delmu_message,  // 0x0009
                &dispatch::process_setmu_message,  // 0x000A
                &dispatch::process_setmv_message,  // 0x000B
                &dispatch::process_getmu_message,  // 0x000C
                &dispatch::process_clrmu_message,  // 0x000D
                &dispatch::process_patch_message,  // 0x000E
                &dispatch::process_unpat_message,  // 0x000F
                &dispatch::process_listp_message,  // 0x0010
                &dispatch::process_copy_message,   // 0x0011
                &dispatch::process_setutv_message, // 0x0012
                &dispatch::process_setmtv_message, // 0x0013
                &dispatch::process_listu_message,  // 0x0014
                &dispatch::process_geta_message,   // 0x0015
                &dispatch::process_getma_message,  // 0x0016
            };

            m_command_handlers = {
                { "set",        &dispatch::process_set_command        },
                { "mset",       &dispatch::process_mset_command       },
                { "get",        &dispatch::process_get_command        },
                { "mget",       &dispatch::process_mget_command       },
                { "copy",       &dispatch::process_copy_command       },
                { "patch",      &dispatch::process_patch_command      },
                { "patches",    &dispatch::process_patches_command    },
                { "unpatch",    &dispatch::process_unpatch_command    },
                { "framerate",  &dispatch::process_framerate_command  },
                { "identify",   &dispatch::process_identify_command   },
                { "ports",      &dispatch::process_ports_command      },
                { "createmask", &dispatch::process_createmask_command },
                { "masks",      &dispatch::process_masks_command      },
                { "deletemask", &dispatch::process_deletemask_command },
                { "clearmask",  &dispatch::process_clearmask_command  },
            };
        }

        dispatch_status process_id_message     (command_context& a_ctx, message_header a_header, uint8_t const* a_body);
        dispatch_status process_setu_message   (command_context& a_ctx, message_header a_header, uint8_t const* a_body);
        dispatch_status process_setv_message   (command_context& a_ctx, message_header a_header, uint8_t const* a_body);
        dispatch_status process_getu_message   (command_context& a_ctx, message_header a_header, uint8_t const* a_body);
        dispatch_status process_setfr_message  (command_context& a_ctx, message_header a_header, uint8_t const* a_body);
        dispatch_status process_getfr_message  (command_context& a_ctx, message_header a_header, uint8_t const* a_body);
        dispatch_status process_newmu_message  (command_context& a_ctx, message_header a_header, uint8_t const* a_body);
        dispatch_status process_listmu_message (command_context& a_ctx, message_header a_header, uint8_t const* a_body);
        dispatch_status process_delmu_message  (command_context& a_ctx, message_header a_header, uint8_t const* a_body);
        dispatch_status process_setmu_message  (command_context& a_ctx, message_header a_header, uint8_t const* a_body);
        dispatch_status process_setmv_message  (command_context& a_ctx, message_header a_header, uint8_t const* a_body);
        dispatch_status process_getmu_message  (command_context& a_ctx, message_header a_header, uint8_t const* a_body);
        dispatch_status process_clrmu_message  (command_context& a_ctx, message_header a_header, uint8_t const* a_body);
        dispatch_status process_patch_message  (command_context& a_ctx, message_header a_header, uint8_t const* a_body);
        dispatch_status process_unpat_message  (command_context& a_ctx, message_header a_header, uint8_t const* a_body);
        dispatch_status process_listp_message  (command_context& a_ctx, message_header a_header, uint8_t const* a_body);
        dispatch_status process_copy_message   (command_context& a_ctx, message_header a_header, uint8_t const* a_body);
        dispatch_status process_setutv_message (command_context& a_ctx, message_header a_header, uint8_t const* a_body);
        dispatch_status process_setmtv_message (command_context& a_ctx, message_header a_header, uint8_t const* a_body);
        dispatch_status process_listu_message  (command_context& a_ctx, message_header a_header, uint8_t const* a_body);
        dispatch_status process_geta_message   (command_context& a_ctx, message_header a_header, uint8_t const* a_body);
        dispatch_status process_getma_message  (command_context& a_ctx, message_header a_header, uint8_t const* a_body);

        dispatch_status process_set_command        (command_context& a_ctx, std::string const& a_command);
        dispatch_status process_mset_command       (command_context& a_ctx, std::string const& a_command);
        dispatch_status process_get_command        (command_context& a_ctx, std::string const& a_command);
        dispatch_status process_mget_command       (command_context& a_ctx, std::string const& a_command);
        dispatch_status process_copy_command       (command_context& a_ctx, std::string const& a_command);
        dispatch_status process_patch_command      (command_context& a_ctx, std::string const& a_command);
        dispatch_status process_patches_command    (command_context& a_ctx, std::string const& a_command);
        dispatch_status process_unpatch_command    (command_context& a_ctx, std::string const& a_command);
        dispatch_status process_framerate_command  (command_context& a_ctx, std::string const& a_command);
        dispatch_status process_identify_command   (command_context& a_ctx, std::string const& a_command);
        dispatch_status process_ports_command      (command_context& a_ctx, std::string const& a_command);
        dispatch_status process_createmask_command (command_context& a_ctx, std::string const& a_command);
        dispatch_status process_masks_command      (command_context& a_ctx, std::string const& a_command);
        dispatch_status process_deletemask_command (command_context& a_ctx, std::string const& a_command);
        dispatch_status process_clearmask_command  (command_context& a_ctx, std::string const& a_command);
    };

    // --------------------------- UTILITY ---------------------------

    /**
     * @brief Convert packed bytes into a bitset object.
     *
     * @tparam v_bit_count The bit count of the bitset.
     *
     * @param a_data The source data from which to convert (expected size of v_bit_count / 8).
     */
    template <size_t v_bit_count>
    std::bitset<v_bit_count> bytes_to_bitset(uint8_t const* a_data) {
        constexpr size_t byte_count = v_bit_count / 8;

        std::bitset<v_bit_count> set;

        for (size_t i = 0; i < byte_count; ++i) {
            uint8_t const byte = *(a_data + i);

            set.set((i * 8) + 0, byte & 0b10000000);
            set.set((i * 8) + 1, byte & 0b01000000);
            set.set((i * 8) + 2, byte & 0b00100000);
            set.set((i * 8) + 3, byte & 0b00010000);
            set.set((i * 8) + 4, byte & 0b00001000);
            set.set((i * 8) + 5, byte & 0b00000100);
            set.set((i * 8) + 6, byte & 0b00000010);
            set.set((i * 8) + 7, byte & 0b00000001);
        }

        return set;
    }

    /**
     * @brief Convert a bitset into a packed set of bytes.
     *
     * @tparam v_bit_count The bit count of the bitset.
     *
     * @param a_data The destination of the converted bytes (expected size of v_bit_count / 8).
     * @param a_set  The set which to convert.
     */
    template <size_t v_bit_count>
    void bitset_to_bytes(uint8_t* a_data, std::bitset<v_bit_count> const& a_set) {
        constexpr size_t byte_count = v_bit_count / 8;

        for (size_t i = 0; i < byte_count; ++i) {
            *(a_data + i) = 0
            | 0b10000000 * a_set.test((i * 8) + 0)
            | 0b01000000 * a_set.test((i * 8) + 1)
            | 0b00100000 * a_set.test((i * 8) + 2)
            | 0b00010000 * a_set.test((i * 8) + 3)
            | 0b00001000 * a_set.test((i * 8) + 4)
            | 0b00000100 * a_set.test((i * 8) + 5)
            | 0b00000010 * a_set.test((i * 8) + 6)
            | 0b00000001 * a_set.test((i * 8) + 7);
        }
    }

    /**
     * @brief Convert a master address to its respective universe and channel numbers.
     *
     * @param a_master_address The master address to convert.
     *
     * @return A tuple containing two values, the universe number in the first position [0]
     *         and the local address in the second position [1].
     */
    constexpr address_pack from_master_address(size_t const a_master_address) noexcept {
        if (a_master_address == 0) { // Null ID.
            return {0, 0};
        }

        // Adjust for one-based indexing, divide by channel count per universe,
        // floor (automatic with integer division), and increase by one for one-base indexing.
        auto const universe = std::max<size_t>(a_master_address - 1, 0) / addresses_per_universe;

        // Determine remainder.
        auto const address = a_master_address - universe * addresses_per_universe;

        return { universe + 1, address };
    }

    /**
     * @brief Converts a universe and address number to a master address.
     *
     * @param a_universe The universe number to convert.
     * @param a_channel  The channel number within the specified universe to convert.
     *
     * @return The converted master address.
     */
    constexpr size_t to_master_address(size_t const a_universe, size_t const a_channel) noexcept {
        if (a_universe == 0 || a_channel == 0) { // Null ID.
            return 0;
        }

        return (a_universe - 1) * addresses_per_universe + a_channel;
    }

    /**
     * @brief Parses a simple address string and returns a pack.
     *
     * @param a_string The address value string.
     *
     * @return The address pack of the string.
     */
    inline address_pack parse_address(std::string const& a_string) {
        size_t const division_index = a_string.find_first_of('/');

        // If address is formatted as a master address, simply forward to from_master_address.
        if (division_index == std::string::npos) {
            return from_master_address(std::stoul(a_string));
        }

        // Address is formatted as universe/address.
        return { std::stoul(a_string.substr(0, division_index)), std::stoul(a_string.substr(division_index + 1)) };
    }

    /**
     * @brief Add one address range to another.
     *
     * @param a_destination The address range to which to add.
     * @param a_source      The address range from which to add.
     */
    inline void add_address_range(address_range& a_destination, address_range const& a_source) noexcept {
        for (auto const& pair : a_source) {
            a_destination[pair.first] |= pair.second;
        }
    }

    /**
     * @brief Subtract one address range from another.
     *
     * @param a_destination The address range from which to subtract.
     * @param a_source      The address range to subtract.
     */
    inline void subtract_address_range(address_range& a_destination, address_range const& a_source) noexcept {
        for (auto const& pair : a_source) {
            a_destination[pair.first] &= ~pair.second;
        }
    }

    /// Left-trim string.
    inline void ltrim(std::string& a_string) {
        a_string.erase(a_string.begin(), std::find_if(a_string.begin(), a_string.end(), [](char const a_char) {
            return !std::isspace(a_char);
        }));
    }

    /// Right-trim string.
    inline void rtrim(std::string& a_string) {
        a_string.erase(std::find_if(a_string.rbegin(), a_string.rend(), [](char const a_char) {
            return !std::isspace(a_char);
        }).base(), a_string.end());
    }

    /// Trim string.
    inline std::string& trim(std::string& a_string) {
        rtrim(a_string);
        ltrim(a_string);

        return a_string;
    }

    inline std::bitset<512> generate_even_bitmask() {
        static std::array<uint8_t, 64> bytes;
        std::fill(bytes.begin(), bytes.end(), 0b01010101);
        return bytes_to_bitset<512>(bytes.data());
    }

    inline void address_range_selector_even(address_range& a_range) {
        static std::bitset<512> even_bitmask{ generate_even_bitmask() };

        for (auto& universe_pair : a_range) {
            universe_pair.second &= even_bitmask;
        }
    }

    inline void address_range_selector_odd(address_range& a_range) {
        static std::bitset<512> odd_bitmask{ ~generate_even_bitmask() };

        for (auto& universe_pair : a_range) {
            universe_pair.second &= odd_bitmask;
        }
    }

    inline void address_range_selector_offset(address_range& a_range, size_t const a_offset) {
        size_t offset_count = 0;

        for (auto& universe_pair : a_range) {
            auto& bitset = universe_pair.second;

            for (size_t i = 0; i < 512; ++i) {
                bool const value = bitset.test(i);

                // Set bit if already set and offset is correct.
                bitset.set(i, value && offset_count == 0);
                // Increment counter if address (bit) is used (set).
                offset_count += value;

                // Reset counter when offset is reached.
                offset_count = offset_count * (offset_count != a_offset);
            }
        }
    }

    inline address_range parse_address_range_term(std::string const& a_string) {
        address_range range;

        // Find end index of the first address in range.
        size_t const start_address_end_index = a_string.find_first_of(' ');

        // Input is a single address, not a range.
        if (start_address_end_index == std::string::npos) {
            auto const pack = parse_address(a_string);
            range[pack.first].set(pack.second - 1);

            return range;
        }

        if (a_string.substr(start_address_end_index, 6) != " thru ") {
            throw std::runtime_error("malformed address range term: expected 'thru' after starting address");
        }

        // Find end index of the second address in range.
        size_t const end_address_end_index = a_string.find_first_of(' ', start_address_end_index + 7);
        size_t const end_address_start_index = start_address_end_index + 6;

        auto const start_address = parse_address(a_string.substr(0, start_address_end_index));
        auto const end_address = parse_address(a_string.substr(end_address_start_index, end_address_end_index - end_address_start_index + 1));

        for (size_t universe_i = start_address.first; universe_i <= end_address.first; ++universe_i) {
            auto& universe = range[universe_i];
            // Set address to starting address on first iteration.                     // Set address bound to end address on last iteration.
            for (size_t address_i = universe_i == start_address.first ? start_address.second : 1; address_i <= (universe_i == end_address.first ? end_address.second : 512); ++address_i) {
                universe.set(address_i - 1);
            }
        }

        for (size_t selector_offset = end_address_end_index + 1; selector_offset < a_string.size();) {
            auto const current_char = a_string[selector_offset];

            if (current_char == 'e' && a_string.substr(selector_offset, 4) == "even") {
                address_range_selector_even(range);
                selector_offset += 5;

                continue;
            }

            if (current_char == 'o' && a_string.substr(selector_offset, 3) == "odd") {
                address_range_selector_odd(range);
                selector_offset += 4;

                continue;
            }

            if (current_char == 'o' && a_string.substr(selector_offset, 7) == "offset ") {
                size_t const offset_number_start_index = selector_offset + 7;
                size_t offset_number_end_index = a_string.find_first_of(' ', offset_number_start_index + 1);
                offset_number_end_index = (offset_number_end_index == std::string::npos) ? a_string.size() - 1 : offset_number_end_index;

                size_t const offset_number = std::stoull(a_string.substr(offset_number_start_index, offset_number_end_index - offset_number_start_index + 1));

                address_range_selector_offset(range, offset_number);

                selector_offset = offset_number_end_index + 1;

                continue;
            }

            ++selector_offset;
        }

        return range;
    }

    /**
     * @brief Parses a full address range, including selectors and combinations/exclusions.
     *
     * @param a_string The raw range string.
     *
     * @return The address range that is represented by the string.
     *
     * @pre Input string should be trimmed (no surrounding whitespace).
     */
    inline address_range parse_address_range(std::string const& a_string) {
        std::vector<std::pair<size_t, char>> combinatorials;

        for (size_t i = 0; i < a_string.size(); ++i) {
            char current_char = a_string[i];

            switch (current_char) {
                case '+':
                case '-':
                    combinatorials.emplace_back(i, current_char);
                break;
                default:
                    break;
            }
        }

        if (combinatorials.empty()) {
            return parse_address_range_term(a_string);
        }

        std::string first_term = a_string.substr(0, combinatorials[0].first);
        trim(first_term);

        address_range range = parse_address_range_term(first_term);

        for (size_t i = 0; i < combinatorials.size(); ++i) {
            auto const& combinatorial = combinatorials[i];

            size_t const term_begin = combinatorial.first + 1;
            size_t const term_end = i + 1 == combinatorials.size() ? a_string.size() - 1 : combinatorials[i + 1].first - 1;

            std::string term = a_string.substr(term_begin, term_end - term_begin + 1);
            trim(term);

            switch (combinatorial.second) {
                case '+':
                    add_address_range(range, parse_address_range_term(term));
                break;
                case '-':
                    subtract_address_range(range, parse_address_range_term(term));
                break;
                default:
                    // Should never happen.
                        break;
            }
        }

        return range;
    }

    /**
     * @brief Parses a value string like '100%' or 'full'.
     *
     * @param a_string The raw value string.
     *
     * @return The value that is represented by the string.
     *
     * @pre Input string should be trimmed (no surrounding whitespace).
     */
    inline uint8_t parse_value(std::string const& a_string) {
        if (a_string == "full") {
            return 255;
        }

        if (a_string == "half") {
            return 128;
        }

        if (a_string == "out") {
            return 0;
        }

        // Percentage value.
        if (a_string.find_last_of('%') != std::string::npos) {
            return static_cast<uint8_t>(std::stod(a_string.substr(0, a_string.size() - 1)) / 100.0 * 255.0);
        }

        return std::stoi(a_string);
    }

    // ---------------------------------------------------------------

    // ------------------- DIRECT CONTROL MESSAGES -------------------

    inline dispatch_status dispatch::process_id_message(command_context& a_ctx, message_header const a_header, uint8_t const* a_body) {
        m_interface.dcsm_id(a_ctx);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_setu_message(command_context& a_ctx, message_header const a_header, uint8_t const* a_body) {
        // 2 (universe number) + 512 (universe data)
        if (a_header.length != 2 + 512) {
            return dispatch_status::invalid_body_size;
        }

        auto const universe_number = bit_cast<uint16_t>(a_body);

        m_interface.dcsm_setu(a_ctx, universe_number, a_body + 2);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_setv_message(command_context& a_ctx, message_header const a_header, uint8_t const* a_body) {
        // Body size must be evenly divisible into address/value pairs.
        if (a_header.length % 5 != 0) {
            return dispatch_status::invalid_body_size;
        }

        size_t const pair_count = a_header.length / 5;

        /// tuple: universe number, local address, value
        std::vector<std::pair<address_pack, uint8_t>> pairs;

        for (size_t i = 0; i < pair_count; ++i) {
            uint8_t const* it = a_body + (i * 5);

            /*
             * Previously reinterpret_casts, but a strange error occurred on the RP2040/Arduino platform while casting
             * the bytes at offset 5 in the data buffer to a uint16_t caused a crash. The problem was isolated to the
             * single cast at offset 5. Casting at offset 6 worked fine. Fine with one pair, but two will cause fault.
             *
             * Solution was to use an implementation of bit_cast, which is technically the correct way to do it, anyway.
             */

            auto const universe_number = bit_cast<uint16_t>(it);
            auto const local_address   = bit_cast<uint16_t>(it + 2);
            auto const value           = *(it + 4);

            pairs.emplace_back(address_pack{ universe_number, local_address }, value);
        }

        m_interface.dcsm_setv(a_ctx, pairs);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_getu_message(command_context& a_ctx, message_header const a_header, uint8_t const* a_body) {
        // 2 (universe number)
        if (a_header.length != 2) {
            return dispatch_status::invalid_body_size;
        }

        auto const universe_number = bit_cast<uint16_t>(a_body);

        m_interface.dcsm_getu(a_ctx, universe_number);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_setfr_message(command_context& a_ctx, message_header const a_header, uint8_t const* a_body) {
        // 1 (framerate)
        if (a_header.length != 1) {
            return dispatch_status::invalid_body_size;
        }

        uint8_t const framerate = *a_body;

        m_interface.dcsm_setfr(a_ctx, framerate);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_getfr_message(command_context& a_ctx, message_header const a_header, uint8_t const* a_body) {
        m_interface.dcsm_getfr(a_ctx);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_newmu_message(command_context& a_ctx, message_header const a_header, uint8_t const* a_body) {
        // 2 (universe number)
        if (a_header.length != 2) {
            return dispatch_status::invalid_body_size;
        }

        auto const universe_number = bit_cast<uint16_t>(a_body);

        m_interface.dcsm_newmu(a_ctx, universe_number);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_listmu_message(command_context& a_ctx, message_header const a_header, uint8_t const* a_body) {
        m_interface.dcsm_listmu(a_ctx);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_delmu_message(command_context& a_ctx, message_header const a_header, uint8_t const* a_body) {
        // 2 (universe number)
        if (a_header.length != 2) {
            return dispatch_status::invalid_body_size;
        }

        auto const universe_number = bit_cast<uint16_t>(a_body);

        m_interface.dcsm_delmu(a_ctx, universe_number);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_setmu_message(command_context& a_ctx, message_header const a_header, uint8_t const* a_body) {
        // 2 (universe number) + 64 (mask) + 512 (data)
        if (a_header.length != 2 + 64 + 512) {
            return dispatch_status::invalid_body_size;
        }

        auto const universe_number = bit_cast<uint16_t>(a_body);

        universe_mask const mask = bytes_to_bitset<512>(a_body + 2);

        m_interface.dcsm_setmu(a_ctx, universe_number, mask, a_body + 2 + 64);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_setmv_message(command_context& a_ctx, message_header const a_header, uint8_t const* a_body) {
        // Body size must be evenly divisible into address/value pairs.
        if (a_header.length < 6 || (a_header.length - 2) % 4 != 0) {
            return dispatch_status::invalid_body_size;
        }

        auto const universe_number = bit_cast<uint16_t>(a_body);
        size_t const pair_count = (a_header.length - 2) / 4;

        /// tuple: local address, masking, value
        std::vector<std::tuple<uint16_t, bool, uint8_t>> pairs;

        for (size_t i = 0; i < pair_count; ++i) {
            uint8_t const* it = a_body + 2 + (i * 4);

            auto     const local_address = bit_cast<uint16_t>(it);
            bool     const masking       = static_cast<bool>(*(it + 2));
            uint8_t  const value         = *(it + 3);

            pairs.emplace_back(local_address, masking, value);
        }

        m_interface.dcsm_setmv(a_ctx, universe_number, pairs);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_getmu_message(command_context& a_ctx, message_header const a_header, uint8_t const* a_body) {
        // 2 (universe number)
        if (a_header.length != 2) {
            return dispatch_status::invalid_body_size;
        }

        auto const universe_number = bit_cast<uint16_t>(a_body);

        m_interface.dcsm_getmu(a_ctx, universe_number);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_clrmu_message(command_context& a_ctx, message_header const a_header, uint8_t const* a_body) {
        // 2 (universe number)
        if (a_header.length != 2) {
            return dispatch_status::invalid_body_size;
        }

        auto const universe_number = bit_cast<uint16_t>(a_body);

        m_interface.dcsm_clrmu(a_ctx, universe_number);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_patch_message(command_context& a_ctx, message_header const a_header, uint8_t const* a_body) {
        // 2 (universe number) + 2 (universe number) + 2 (universe number)
        if (a_header.length != 6) {
            return dispatch_status::invalid_body_size;
        }

        auto const input_universe  = bit_cast<uint16_t>(a_body);
        auto const output_universe = bit_cast<uint16_t>(a_body + 2);
        auto const mask_universe   = bit_cast<uint16_t>(a_body + 4);

        m_interface.dcsm_patch(a_ctx, input_universe, output_universe, mask_universe);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_unpat_message(command_context& a_ctx, message_header const a_header, uint8_t const* a_body) {
        // 2 (universe number)
        if (a_header.length != 2) {
            return dispatch_status::invalid_body_size;
        }

        auto const output_universe = bit_cast<uint16_t>(a_body);

        m_interface.dcsm_unpat(a_ctx, output_universe);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_listp_message(command_context& a_ctx, message_header const a_header, uint8_t const* a_body) {
        m_interface.dcsm_listp(a_ctx);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_copy_message(command_context& a_ctx, message_header const a_header, uint8_t const* a_body) {
        // 2 (universe number) + 2 (universe number)
        if (a_header.length != 4) {
            return dispatch_status::invalid_body_size;
        }

        auto const source_universe      = bit_cast<uint16_t>(a_body);
        auto const destination_universe = bit_cast<uint16_t>(a_body);

        m_interface.dcsm_copy(a_ctx, source_universe, destination_universe);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_setutv_message(command_context& a_ctx, message_header const a_header, uint8_t const* a_body) {
        // 2 (universe number) + 1 (value) + 64 (mask)
        if (a_header.length != 2 + 1 + 64) {
            return dispatch_status::invalid_body_size;
        }

        auto const universe_number = bit_cast<uint16_t>(a_body);
        auto const value           = *(a_body + 2);

        universe_mask const mask = bytes_to_bitset<512>(a_body + 3);

        m_interface.dcsm_setutv(a_ctx, universe_number, value, mask);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_setmtv_message(command_context& a_ctx, message_header const a_header, uint8_t const* a_body) {
        // 2 (universe number) + 1 (value) + 64 (mask)
        if (a_header.length != 67) {
            return dispatch_status::invalid_body_size;
        }

        auto const universe_number = bit_cast<uint16_t>(a_body);
        auto const value           = *(a_body + 2);

        universe_mask const mask = bytes_to_bitset<512>(a_body + 3);

        m_interface.dcsm_setmtv(a_ctx, universe_number, value, mask);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_listu_message(command_context &a_ctx, message_header const a_header, uint8_t const *a_body) {
        m_interface.dcsm_listu(a_ctx);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_geta_message(command_context &a_ctx, message_header const a_header, uint8_t const *a_body) {
        // Body size must be evenly divisible into address pairs.
        if (a_header.length < 4 || a_header.length % 4 != 0) {
            return dispatch_status::invalid_body_size;
        }

        size_t const pair_count = a_header.length / 4;

        std::vector<address_pack> addresses;

        for (size_t i = 0; i < pair_count; ++i) {
            uint8_t const* it = a_body + (i * 4);

            auto const universe_number = bit_cast<uint16_t>(it);
            auto const local_address   = bit_cast<uint16_t>(it + 2);

            addresses.emplace_back(universe_number, local_address);
        }

        m_interface.dcsm_geta(a_ctx, addresses);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_getma_message(command_context &a_ctx, message_header const a_header, uint8_t const *a_body) {
        // Body size must be evenly divisible into address pairs.
        if (a_header.length < 4 || a_header.length % 4 != 0) {
            return dispatch_status::invalid_body_size;
        }

        size_t const pair_count = a_header.length / 4;

        std::vector<address_pack> addresses;

        for (size_t i = 0; i < pair_count; ++i) {
            uint8_t const* it = a_body + (i * 4);

            auto const universe_number = bit_cast<uint16_t>(it);
            auto const local_address   = bit_cast<uint16_t>(it + 2);

            addresses.emplace_back(universe_number, local_address);
        }

        m_interface.dcsm_getma(a_ctx, addresses);
        return dispatch_status::success;
    }

    // ----------------- END DIRECT CONTROL MESSAGES -----------------


    // -------------------------- COMMANDS ---------------------------


    inline dispatch_status dispatch::process_set_command(command_context &a_ctx, std::string const &a_command) {
        size_t const at_delim_index = a_command.find_first_of('@');

        if (at_delim_index == std::string::npos) {
            return dispatch_status::malformed_syntax;
        }

        std::string address_range_string = a_command.substr(0, at_delim_index);
        trim(address_range_string);

        std::string value_string = a_command.substr(at_delim_index + 1);
        trim(value_string);

        auto const range = parse_address_range(address_range_string);
        auto const value = parse_value(value_string);

        for (auto const& universe_pair : range) {
            m_interface.dcsm_setutv(a_ctx, universe_pair.first, value, universe_pair.second);
        }

        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_mset_command(command_context &a_ctx, std::string const &a_command) {
        size_t const at_delim_index = a_command.find_first_of('@');

        if (at_delim_index == std::string::npos) {
            return dispatch_status::malformed_syntax;
        }

        std::string address_range_string = a_command.substr(0, at_delim_index);
        trim(address_range_string);

        std::string value_string = a_command.substr(at_delim_index + 1);
        trim(value_string);

        auto const range = parse_address_range(address_range_string);
        auto const value = parse_value(value_string);

        for (auto const& universe_pair : range) {
            m_interface.dcsm_setmtv(a_ctx, universe_pair.first, value, universe_pair.second);
        }

        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_get_command(command_context &a_ctx, std::string const &a_command) {
        std::string address_range_string = a_command;
        trim(address_range_string);

        auto const range = parse_address_range(address_range_string);

        std::vector<address_pack> addresses;

        for (auto const& universe_pair : range) {
            auto const& set = universe_pair.second;

            for (size_t i = 0; i < 512; ++i) {
                if (set.test(i)) {
                    const auto address_number = i + 1;
                    addresses.emplace_back(universe_pair.first, address_number);

                    if (addresses.size() >= 100) {
                        break;
                    }
                }
            }
        }

        m_interface.dcsm_geta(a_ctx, addresses);

        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_mget_command(command_context &a_ctx, std::string const &a_command) {
        std::string address_range_string = a_command;
        trim(address_range_string);

        auto const range = parse_address_range(address_range_string);

        std::vector<address_pack> addresses;

        for (auto const& universe_pair : range) {
            auto const& set = universe_pair.second;

            for (size_t i = 0; i < 512; ++i) {
                if (set.test(i)) {
                    const auto address_number = i + 1;
                    addresses.emplace_back(universe_pair.first, address_number);

                    if (addresses.size() >= 100) {
                        break;
                    }
                }
            }
        }

        m_interface.dcsm_getma(a_ctx, addresses);

        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_copy_command(command_context &a_ctx, std::string const &a_command) {
        size_t const to_delim_index = a_command.find_first_of('t');

        if (to_delim_index == std::string::npos || to_delim_index + 1 == a_command.size() || a_command[to_delim_index + 1] != 'o') {
            return dispatch_status::malformed_syntax;
        }

        std::string source_universe_string = a_command.substr(0, to_delim_index);
        trim(source_universe_string);

        std::string dest_universe_string = a_command.substr(to_delim_index + 2);
        trim(dest_universe_string);

        uint16_t const source_universe = std::stoul(source_universe_string);
        uint16_t const dest_universe   = std::stoul(dest_universe_string);

        m_interface.dcsm_copy(a_ctx, source_universe, dest_universe);

        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_patch_command(command_context &a_ctx, std::string const &a_command) {
        size_t const to_delim_index = a_command.find_first_of('t');

        if (to_delim_index == std::string::npos || to_delim_index + 1 == a_command.size() || a_command[to_delim_index + 1] != 'o') {
            return dispatch_status::malformed_syntax;
        }

        std::string input_universe_string = a_command.substr(0, to_delim_index);
        trim(input_universe_string);

        uint16_t const input_universe = std::stoul(input_universe_string);
        uint16_t mask_universe   = 0;

        size_t const mask_specifier_index = a_command.find_first_of('m', to_delim_index + 3);

        std::string output_universe_string;

        if (mask_specifier_index != std::string::npos) {
            // Mask universe specified.

            if (a_command.substr(mask_specifier_index, 4) != "mask") {
                return dispatch_status::malformed_syntax;
            }

            output_universe_string = a_command.substr(to_delim_index + 2, (mask_specifier_index - 1) - (to_delim_index + 2) + 1);

            std::string mask_universe_string = a_command.substr(mask_specifier_index + 4);
            trim(mask_universe_string);
            mask_universe = std::stoul(mask_universe_string);
        } else {
            // Mask universe omitted.

            output_universe_string = a_command.substr(to_delim_index + 2);
        }

        trim(output_universe_string);
        uint16_t const output_universe = std::stoul(output_universe_string);

        m_interface.dcsm_patch(a_ctx, input_universe, output_universe, mask_universe);

        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_patches_command(command_context &a_ctx, std::string const &a_command) {
        m_interface.dcsm_listp(a_ctx);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_unpatch_command(command_context &a_ctx, std::string const &a_command) {
        std::string output_universe_string = a_command;
        trim(output_universe_string);
        uint16_t const output_universe = std::stoul(output_universe_string);

        m_interface.dcsm_unpat(a_ctx, output_universe);

        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_framerate_command(command_context &a_ctx, std::string const &a_command) {
        std::string framerate_string = a_command;
        trim(framerate_string);

        if (framerate_string.empty()) {
            m_interface.dcsm_getfr(a_ctx);
        } else {
            uint8_t const framerate = std::stoul(framerate_string);
            m_interface.dcsm_setfr(a_ctx, framerate);
        }

        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_identify_command(command_context &a_ctx, std::string const &a_command) {
        m_interface.dcsm_id(a_ctx);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_ports_command(command_context &a_ctx, std::string const &a_command) {
        m_interface.dcsm_listu(a_ctx);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_createmask_command(command_context &a_ctx, std::string const &a_command) {
        std::string universe_number_string = a_command;
        trim(universe_number_string);
        uint16_t const universe_number = std::stoul(universe_number_string);

        m_interface.dcsm_newmu(a_ctx, universe_number);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_masks_command(command_context &a_ctx, std::string const &a_command) {
        m_interface.dcsm_listmu(a_ctx);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_deletemask_command(command_context &a_ctx, std::string const &a_command) {
        std::string universe_number_string = a_command;
        trim(universe_number_string);
        uint16_t const universe_number = std::stoul(universe_number_string);

        m_interface.dcsm_delmu(a_ctx, universe_number);
        return dispatch_status::success;
    }

    inline dispatch_status dispatch::process_clearmask_command(command_context &a_ctx, std::string const &a_command) {
        std::string universe_number_string = a_command;
        trim(universe_number_string);
        uint16_t const universe_number = std::stoul(universe_number_string);

        m_interface.dcsm_clrmu(a_ctx, universe_number);
        return dispatch_status::success;
    }

    // ------------------------ END COMMANDS -------------------------


}

#endif //DCSM_HPP
