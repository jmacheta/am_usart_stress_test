#ifndef SERIAL_CONNECTION_HPP_
#define SERIAL_CONNECTION_HPP_
#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <string_view>

class SerialConnection {
  public:
    std::string const port;
    unsigned const    baudrate;

    auto operator=(SerialConnection&&) = delete; // Surpress copy/move with rule of DesDeMovA

    SerialConnection(std::string_view port, unsigned baudrate);

    ~SerialConnection();

    std::size_t write(std::span<std::byte const> data);
    std::size_t read(std::span<std::byte> buffer);
    void        clear();
    bool        open();
    void        close();

    struct SerialConnectionImpl;

  private:
    std::unique_ptr<SerialConnectionImpl> impl;
};


bool enable_std_colored_output();



#endif