#include "serial_connection.hpp"

#include <list>
#include <string>

// clang-format off
#include <windows.h>
#include <winbase.h>
// clang-format on

using namespace std::literals::string_literals;

struct SerialConnection::SerialConnectionImpl {
    HANDLE handle{INVALID_HANDLE_VALUE}; ///< System handle for UART interface
};


SerialConnection::SerialConnection(std::string_view port, unsigned baudrate) : port{"\\\\.\\"s + port.data()}, baudrate{baudrate}, impl(new SerialConnection::SerialConnectionImpl) {}


SerialConnection::~SerialConnection() {
    close();
}


bool SerialConnection::open() {
    if (INVALID_HANDLE_VALUE == impl->handle) {
        impl->handle = ::CreateFileA(port.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (INVALID_HANDLE_VALUE == impl->handle) {
            return false;
        }

        ::COMMTIMEOUTS timeouts{0, 0, 10, 0, 0};
        ::SetCommTimeouts(impl->handle, &timeouts);

        ::DCB dcb{};

        if (0 == ::GetCommState(impl->handle, &dcb)) {
            goto close_and_fail;
        }

        dcb.BaudRate = baudrate;
        dcb.ByteSize = 8;
        dcb.Parity   = NOPARITY;
        dcb.StopBits = ONESTOPBIT;

        if (0 == ::SetCommState(impl->handle, &dcb)) {
            goto close_and_fail;
        }
    }
    // Clear inout buffers
    ::PurgeComm(impl->handle, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);


    return true;

close_and_fail:
    close();
    return false;
}


void SerialConnection::clear() {
    if (impl->handle != INVALID_HANDLE_VALUE) {
        ::PurgeComm(impl->handle, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
    }
}


void SerialConnection::close() {
    if (impl->handle != INVALID_HANDLE_VALUE) {
        ::CloseHandle(impl->handle);
        impl->handle = INVALID_HANDLE_VALUE;
    }
}


std::size_t SerialConnection::read(std::span<std::byte> buffer) {
    ::DWORD readChars{0};
    ::ReadFile(impl->handle, buffer.data(), static_cast<::DWORD>(buffer.size()), &readChars, nullptr);
    return static_cast<std::size_t>(readChars);
}


std::size_t SerialConnection::write(std::span<std::byte const> data) {
    ::DWORD written{0};
    ::WriteFile(impl->handle, data.data(), static_cast<::DWORD>(data.size_bytes()), &written, nullptr);
    return static_cast<std::size_t>(written);
}
