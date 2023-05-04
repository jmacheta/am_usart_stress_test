# USART Stress test Application for _Microcontroller applications_ course

The app writes predefined amount of data on serial port, and expects to receive the checksum (32-bit sum of bytes) from the other end

![build status](https://github.com/jmacheta/am_usart_stress_test/actions/workflows/cmake.yml/badge.svg)

## Usage
```shell
usart_stress_test [-h] [--baudrate VAR] [--count VAR] [--size VAR] [--verbose] port
```
