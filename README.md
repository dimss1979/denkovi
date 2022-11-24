# Denkovi 16-channel USB relay board control tool

There is a 16-channel USB relay board made by Denkovi:

https://denkovi.com/usb-16-relay-board

Besides other tools and code samples, the vendor provides CLI board control tool written in Java. It's difficult to use with Linux:

* Too heavy for some embedded Linux hosts
* It expects a plain USB device - does not work while USB serial kernel modules are loaded

Fortunately, relay board control protocol is described in the document:

http://denkovi.com/Documents/USB-Relay-16Channels-v3/UserManual.pdf

This tool is a lightweight plain C implementation of the protocol which should run on any Linux and probably *BSD.

## Build

```
gcc -o denkovi -Wall -Werror denkovi.c
```

Or write a Makefile if you wish.

## Run

```
dimss@domik:~/p/denkovi$ ./denkovi /dev/serial/by-id/usb-FTDI_FT232R_USB_UART_DAE004XC-if00-port0 status
1  0
2  0
3  0
4  0
5  0
6  0
7  0
8  0
9  0
10 0
11 0
12 0
13 0
14 0
15 0
16 0
dimss@domik:~/p/denkovi$ ./denkovi /dev/serial/by-id/usb-FTDI_FT232R_USB_UART_DAE004XC-if00-port0 status_hex
0x0000
dimss@domik:~/p/denkovi$ ./denkovi /dev/serial/by-id/usb-FTDI_FT232R_USB_UART_DAE004XC-if00-port0 on_all
dimss@domik:~/p/denkovi$ ./denkovi /dev/serial/by-id/usb-FTDI_FT232R_USB_UART_DAE004XC-if00-port0 status_hex
0xffff
dimss@domik:~/p/denkovi$ ./denkovi /dev/serial/by-id/usb-FTDI_FT232R_USB_UART_DAE004XC-if00-port0 off_all
dimss@domik:~/p/denkovi$ ./denkovi /dev/serial/by-id/usb-FTDI_FT232R_USB_UART_DAE004XC-if00-port0 status_hex
0x0000
dimss@domik:~/p/denkovi$ ./denkovi /dev/serial/by-id/usb-FTDI_FT232R_USB_UART_DAE004XC-if00-port0 on 1
dimss@domik:~/p/denkovi$ ./denkovi /dev/serial/by-id/usb-FTDI_FT232R_USB_UART_DAE004XC-if00-port0 status_hex
0x8000
dimss@domik:~/p/denkovi$ ./denkovi /dev/serial/by-id/usb-FTDI_FT232R_USB_UART_DAE004XC-if00-port0 on 5
dimss@domik:~/p/denkovi$ ./denkovi /dev/serial/by-id/usb-FTDI_FT232R_USB_UART_DAE004XC-if00-port0 status_hex
0x8800
dimss@domik:~/p/denkovi$ ./denkovi /dev/serial/by-id/usb-FTDI_FT232R_USB_UART_DAE004XC-if00-port0 on 9
dimss@domik:~/p/denkovi$ ./denkovi /dev/serial/by-id/usb-FTDI_FT232R_USB_UART_DAE004XC-if00-port0 status_hex
0x8880
dimss@domik:~/p/denkovi$ ./denkovi /dev/serial/by-id/usb-FTDI_FT232R_USB_UART_DAE004XC-if00-port0 set 0xabcd
dimss@domik:~/p/denkovi$ ./denkovi /dev/serial/by-id/usb-FTDI_FT232R_USB_UART_DAE004XC-if00-port0 status_hex
0xabcd
dimss@domik:~/p/denkovi$ ./denkovi /dev/serial/by-id/usb-FTDI_FT232R_USB_UART_DAE004XC-if00-port0 status
1  1
2  0
3  1
4  0
5  1
6  0
7  1
8  1
9  1
10 1
11 0
12 0
13 1
14 1
15 0
16 1
dimss@domik:~/p/denkovi$ ./denkovi /dev/serial/by-id/usb-FTDI_FT232R_USB_UART_DAE004XC-if00-port0 off 1
dimss@domik:~/p/denkovi$ ./denkovi /dev/serial/by-id/usb-FTDI_FT232R_USB_UART_DAE004XC-if00-port0 status_hex
0x2bcd
dimss@domik:~/p/denkovi$
```

Make sure serial port is accessible.
In Debian, the user must be a member of "dialout" group.

The tool creates lock file `/tmp/denkovi.lock`.

## Serial port naming

Default USB serial port name in Linux is in the form:

```
/dev/ttyUSB0
/dev/ttyUSB1
/dev/ttyUSB2
...
```

When multiple USB serial ports are connected to the same computer, numbering order is not guaranteed.
For example, Debian is providing an alternative naming scheme based on USB device serial number.
In our case, board serial number is "DAE004XC":

```
/dev/serial/by-id/usb-FTDI_FT232R_USB_UART_DAE004XC-if00-port0
```

You better use these if available in your system.
