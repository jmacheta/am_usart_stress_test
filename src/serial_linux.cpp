#include "serial_connection.hpp"

#include <list>
#include <string>

// clang-format off
#include <asm/ioctls.h>
#include <asm/termbits.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <unistd.h>
// clang-format on

using namespace std::literals::string_literals;

struct SerialConnection::SerialConnectionImpl {
    int handle = -1; ///< System handle for UART interface
};


SerialConnection::SerialConnection(std::string_view port, unsigned baudrate) : port{"\\\\.\\"s + port.data()}, baudrate{baudrate}, impl(new SerialConnection::SerialConnectionImpl) {}


SerialConnection::~SerialConnection() {
    close();
}


bool SerialConnection::open() {
    impl->handle = ::open(port.c_str(), O_RDWR);

    if (impl->handle < 0) {
        return false;
    }

    if (-1 == flock(impl->handle, LOCK_EX | LOCK_NB)) {
        ::close(impl->handle);
        return false;
    }

    struct termios2 tty;

    if (ioctl(impl->handle, TCGETS2, &tty) < 0) {
        ::close(impl->handle);
        return false;
    }

    tty.c_cflag &= static_cast<tcflag_t>(~PARENB);
    tty.c_cflag &= static_cast<tcflag_t>(~CSTOPB);
    tty.c_cflag &= static_cast<tcflag_t>(~CSIZE);
    tty.c_cflag |= static_cast<tcflag_t>(CS8);
    tty.c_cflag &= static_cast<tcflag_t>(~CRTSCTS);
    tty.c_cflag |= static_cast<tcflag_t>(CREAD | CLOCAL);
    tty.c_lflag &= static_cast<tcflag_t>(~ICANON);
    tty.c_lflag &= static_cast<tcflag_t>(~ECHO);
    tty.c_lflag &= static_cast<tcflag_t>(~ECHOE);
    tty.c_lflag &= static_cast<tcflag_t>(~ECHONL);
    tty.c_lflag &= static_cast<tcflag_t>(~ISIG);
    tty.c_iflag &= static_cast<tcflag_t>(~(IXON | IXOFF | IXANY));
    tty.c_iflag &= static_cast<tcflag_t>(~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL));
    tty.c_oflag &= static_cast<tcflag_t>(~OPOST);
    tty.c_oflag &= static_cast<tcflag_t>(~ONLCR);
    tty.c_cc[VTIME] = 0;
    tty.c_cc[VMIN]  = 0;

    tty.c_cflag &= static_cast<tcflag_t>(~CBAUD);
    tty.c_cflag |= CBAUDEX;

    tty.c_ispeed = baudrate;
    tty.c_ospeed = baudrate;

    if (ioctl(impl->handle, TCSETS2, &tty) < 0) {
        ::close(impl->handle);
        return false;
    }

    return true;
}


void SerialConnection::clear() {
    if (impl->handle > -1) {
        tcflush(impl->handle, TCIOFLUSH);
    }
}


void SerialConnection::close() {
    if (impl->handle > -1) {
        ::close(impl->handle);
    }
}


std::size_t SerialConnection::read(std::span<std::byte> buffer) {
    auto readBytes = ::read(impl->handle, buffer.data(), buffer.size());
    if (readBytes < 0) {
        readBytes = 0U;
    }

    return static_cast<std::size_t>(readBytes);
}


std::size_t SerialConnection::write(std::span<std::byte const> data) {
    auto written = ::write(handle, data.data(), data.size());
    if (written < 0) {
        written = 0U;
    }
    return static_cast<std::size_t>(written);
}
