#include "serial_connection.hpp"

#include <argparse/argparse.hpp>
#include <fmt/color.h>
#include <fmt/ostream.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <random>
#include <thread>
#include <vector>
#ifndef APP_VERSION
#    define APP_VERSION "0.0.0"
#endif


using namespace std::literals::string_view_literals;
using namespace std::literals::chrono_literals;
using namespace std::literals::string_literals;
constexpr auto response_timeout         = 1s;
constexpr auto response_expected_length = 4;


bool verbose     = false;
bool colorless_mode = true;


auto when_colored(auto&& color) {
    if (colorless_mode) {
        return fmt::text_style();
    } else {
        return fg(color);
    }
}


void on_app_finished() {
    fmt::print(when_colored(fmt::color::lemon_chiffon), "======= USART stress test application end =======");
}


int fail_with_reason(std::string_view reason) {
    fmt::print(when_colored(fmt::color::aqua), "Test finished with result: ");
    fmt::print(when_colored(fmt::color::indian_red), "FAIL - {}\n", reason);
    return 0;
}

int perform_test(SerialConnection& c, std::size_t bytes_to_send) {
    std::random_device                          dev;
    std::mt19937                                rng(dev());
    std::uniform_int_distribution<std::uint8_t> dist(1);
    std::vector<std::uint8_t>                   to_send(bytes_to_send, 0);
    auto                                        gen = [&dist, &rng]() { return dist(rng); };

    std::ranges::generate(to_send, gen);

    auto checksum = std::accumulate(to_send.begin(), to_send.end(), std::uint32_t(0));
    if (verbose) {
        fmt::print("\tSending {} bytes. Expected checksum {:#x}", bytes_to_send, checksum);
    }
    c.clear();
    c.write({reinterpret_cast<std::byte const*>(to_send.data()), to_send.size()});

    auto transmission_duration = std::chrono::microseconds((2'000'000 * bytes_to_send) / c.baudrate);
    auto wait_for_response_end = std::chrono::system_clock::now() + response_timeout + transmission_duration;

    std::vector<std::byte> response;
    response.reserve(response_expected_length);

    while (response.size() < response_expected_length && (std::chrono::system_clock::now() < wait_for_response_end)) {
        std::byte tmp;
        if (1U == c.read({&tmp, sizeof(tmp)})) {
            response.push_back(tmp);
        } else {
            if (verbose) {
                ::putchar('.');
            }
        }
        std::this_thread::sleep_for(100ms);
    }
    if (verbose) {
        ::putchar('\n');
    }

    if (response.size() != response_expected_length) {
        return fail_with_reason(fmt::format("Invalid response (expected 4B, got {}B)", response.size()));
    } else {
        std::uint32_t received_checksum;
        std::memcpy(&received_checksum, response.data(), sizeof(received_checksum));
        if (received_checksum != checksum) {
            return fail_with_reason(fmt::format("Invalid checksum (expected {:#x}, got {:#x})", checksum, received_checksum));
        } else {
            fmt::print(when_colored(fmt::color::aqua), "Test finished with result: ");
            fmt::print(when_colored(fmt::color::lime_green), "PASS\n");
        }
    }
    return 1;
}

int main(int argc, char** argv) {
    colorless_mode = !enable_std_colored_output();
    fmt::print(when_colored(fmt::color::lemon_chiffon), "======= USART stress test application =======\n");
    std::atexit(on_app_finished);


    argparse::ArgumentParser program("usart_stress_test", APP_VERSION);

    program.add_argument("port").required().help("serial port to connect to");
    program.add_argument("-b", "--baudrate").help("serial port baudrate").default_value(115200).scan<'i', int>();
    program.add_argument("-c", "--count").help("number of test iterations to perform").default_value(5).scan<'i', int>();
    program.add_argument("-s", "--size").help("test payload size in bytes").default_value(10 * 1024).scan<'i', int>();
    program.add_argument("--verbose").help("increase output verbosity").default_value(false).implicit_value(true);

    int         size     = 0;
    int         count    = 0;
    int         baudrate = 0;
    std::string port;

    try {
        program.parse_args(argc, argv);
        port     = program.get<std::string>("port");
        baudrate = program.get<int>("--baudrate");
        size     = program.get<int>("--size");
        count    = program.get<int>("--count");
        verbose  = program.get<bool>("--verbose");
    } catch (const std::exception& e) {
        auto msg = fmt::format(when_colored(fmt::color::red), "Exception during parsing user input: {}\n", e.what());
        std::cerr << msg << program << std::endl;
        return -1;
    }

    if (size <= 0) {
        auto msg = fmt::format(when_colored(fmt::color::red), "Invalid payload size: {}\n", size);
        std::cerr << msg << program << std::endl;
        return -2;
    }

    if (count <= 0) {
        auto msg = fmt::format(when_colored(fmt::color::red), "Invalid test count: {}\n", count);
        std::cerr << msg << program << std::endl;
        return -2;
    }

    if (baudrate <= 0) {
        auto msg = fmt::format(when_colored(fmt::color::red), "Invalid baudrate: {}\n", baudrate);
        std::cerr << msg << program << std::endl;
        return -2;
    }


    if (verbose) {
        fmt::print("Opening serial port: {} with baudrate: {}\n", port, baudrate);
        fmt::print("{} tests to perform with {} bytes payload\n", count, size);
    }


    SerialConnection connection(port, baudrate);

    if (!connection.open()) {
        auto msg = fmt::format(when_colored(fmt::color::red), "Opening serial port: {} failed - make sure that no other application uses this serial port", connection.port);
        std::cerr << msg << std::endl;
        return -3;
    }
    if (verbose) {
        fmt::print("connection opened\n");
    }


    auto successes = 0;
    if (successes > 0) {
        fmt::print(when_colored(fmt::color::lime_green), "\n\t\t{}/{} TESTS PASSED\n", successes, count);
    }

    for (auto i = 0; i != count; ++i) {
        fmt::print(when_colored(fmt::color::aqua), "Test {}/{}:\n", i + 1, count);
        successes += perform_test(connection, size);
    }

    if (count == successes) {
        fmt::print(when_colored(fmt::color::lime_green), "\n\t\tALL TESTS PASSED\n");
    } else {
        fmt::print(when_colored(fmt::color::indian_red), "\n\t\t{}/{} TESTS FAILED\n", count - successes, count);
    }

    return 0;
}