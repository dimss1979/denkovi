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
dimss@domik:~/p/denkovi$ ./denkovi 
Usage: ./denkovi <serial_port> <command> [command_args]
Where command is:
    status
    status_hex
    on_all
    off_all
    on <relay_number>
    off <relay_number>
    set <relay_bitmap>
Where relay_number is: 1..16
      relay_bitmap is: 16-bit unsigned integer - MSB for relay 1, LSB for relay 16
dimss@domik:~/p/denkovi$
```

Make sure serial port is accessible.
In Debian, the user must be a member of "dialout" group.

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

## Known issues

1. When relay 9 is on, the status of relays 1-8 is always reported as on (on-board green LEDs are correct at the same time).

Example:

```
set 0x0101 - board reports 0x0101 (pass)
set 0x0181 - board reports 0xff81 (fail)
```

2. When using "set" command, at least one relay in each bitmap octet must be turned on (at least one bit in each byte must be one). If at least one of two bytes representing relays 1-8 and 9-16 respectively is zero, the command fails.

Example:

```
0x0000 - fail
0x0001 - fail
0x8000 - fail
0xff00 - fail
0x8001 - pass
0x8002 - pass
0x0102 - pass
0x80ff - pass
```

This behavior is not mentioned in the documentation.
It looks like a board firmware or hardware issue.
However, it is still possible to use the board with these limitations:

1. Don't rely on "status" of relays 1-8, or don't use relay 9
2. Don't use "set" command unless you are sure there will always be at least one relay on in each group 1-8 and 9-16 respectively. Use commands "on", "off", "on_all", "off_all" instead.
