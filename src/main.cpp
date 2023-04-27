#include "serial_connection.hpp"

#include <termcolor/termcolor.hpp>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <random>
#include <thread>
#include <vector>


void on_app_finished() {
    std::cout << termcolor::yellow << "============ USART stress test application end ============" << termcolor::reset << std::endl;
}

using namespace std::literals::string_view_literals;
using namespace std::literals::chrono_literals;

constexpr auto response_timeout         = 2s;
constexpr auto response_expected_length = 2;


void display_help() {
    std::cout << "usage: usart_stress_test <serial_port> <baudrate>\n";
}

unsigned parse_baudrate(std::string_view str) {
    unsigned baudrate;
    auto [p, ec] = std::from_chars(str.begin(), str.end(), baudrate);

    return (std::errc() == ec) ? baudrate : 0;
}


void perform_test(SerialConnection& c, std::size_t bytes_to_send) {
    std::random_device                          dev;
    std::mt19937                                rng(dev());
    std::uniform_int_distribution<std::uint8_t> dist(1);
    std::vector<std::uint8_t>                   to_send(bytes_to_send, 0);
    auto                                        gen = [&dist, &rng]() { return dist(rng); };

    std::ranges::generate(to_send, gen);

    auto checksum = std::accumulate(to_send.begin(), to_send.end(), std::uintmax_t(0));

    std::cout << "to_send: ";
    for (auto c : to_send) {
        std::cout << std::hex << static_cast<int>(c) << ',';
    }
    std::cout << '\n';
    std::cout << "checksum: " << checksum << "\n";

    c.clear();
    c.write({reinterpret_cast<std::byte const*>(to_send.data()), to_send.size()});

    auto wait_for_response_end = std::chrono::system_clock::now() + response_timeout;

    std::vector<std::byte> response;
    response.reserve(response_expected_length);

    while (response.size() < response_expected_length && (std::chrono::system_clock::now() < wait_for_response_end)) {
        std::cout << ".";
        std::byte tmp;
        if (1U == c.read({&tmp, sizeof(tmp)})) {
            response.push_back(tmp);
        }
        std::this_thread::sleep_for(100ms);
    }
}

int main(int argc, char** argv) {
    std::cout << termcolor::yellow << "============ USART stress test application ============" << termcolor::reset << std::endl;
    std::atexit(on_app_finished);

    if (argc != 3) {
        std::cerr << termcolor::red << "Invalid argument count" << termcolor::reset << std::endl;
        display_help();
        return -1;
    }

    auto baudrate = parse_baudrate(argv[2]);
    if (0 == baudrate) {
        std::cerr << "Invalid baudrate\n";
        return -2;
    }

    std::cout << "Opening serial port: " << argv[1] << " with baudrate: " << baudrate << '\n';
    // Use second argument as a serial port name
    SerialConnection connection(argv[1], baudrate);
    if (!connection.open()) {
        std::cout << termcolor::red << "Opening serial port: " << connection.port << " failed - make sure that no other application uses this serial port" << termcolor::reset << std::endl;
        return -3;
    }

    std::cout << "connection opened\n";


    perform_test(connection, 100);
    perform_test(connection, 100);
    perform_test(connection, 100);


    return 0;
}