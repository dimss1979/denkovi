// gcc -o denkovi -Wall -Werror denkovi.c

#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#define RESP_LEN_ASK    2
#define RESP_LEN_ON     4
#define RESP_LEN_OFF    5
#define RESP_LEN_SET    5

char *serial_port = NULL;

int send_and_receive(char *command, char *response, int response_len)
{
    int fd;
    int rv = 0;
    int bytes_read;

    fd = open(serial_port, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        fprintf(stderr, "Cannot open serial port\n");
        rv = -1;
        goto end;
    }

    {
        struct termios t;

        tcgetattr(fd, &t);

        cfsetispeed(&t, B9600);
        cfsetospeed(&t, B9600);
        t.c_cflag = (t.c_cflag & ~CSIZE) | CS8;
        t.c_iflag &= ~IGNBRK;
        t.c_lflag = 0;
        t.c_oflag = 0;
        t.c_cc[VMIN]  = 0;
        t.c_cc[VTIME] = 5;
        t.c_iflag &= ~(IXON | IXOFF | IXANY);
        t.c_cflag |= (CLOCAL | CREAD);
        t.c_cflag &= ~(PARENB | PARODD);
        t.c_cflag &= ~CSTOPB;
        t.c_cflag &= ~CRTSCTS;

        if (tcsetattr(fd, TCSANOW, &t) != 0) {
            fprintf(stderr, "Cannot set serial port attributes\n");
            rv = -1;
            goto end;
        }
        tcflush(fd, TCIFLUSH);
    }

    write(fd, command, strlen(command));
    usleep(100 * 1000);
    bytes_read = read(fd, response, response_len);

    if (bytes_read != response_len) {
        fprintf(stderr, "Invalid response length: %i (must be %i)\n", bytes_read, response_len);
        rv = -1;
        goto end;
    }

end:

    if (fd >= 0)
        close(fd);
    
    return rv;
}

int cmd_ask()
{
    int rv;
    char response[RESP_LEN_ASK];
    char *command = "ask//";
    uint16_t relay_states;
    int relay_state;

    rv = send_and_receive(command, response, sizeof(response));
    if (rv) {
        goto end;
    }

    relay_states = ((response[0] << 8) | response[1]);
    for (int i = 1; i <= 16; i++) {
        relay_state = (relay_states & 0x8000) ? 1 : 0;
        relay_states <<= 1;
        printf("%-2i %i\n", i, relay_state);
    }

end:

    return rv;
}

int cmd_on()
{
    int rv;
    char response[RESP_LEN_ON];
    char *command = "on//";

    rv = send_and_receive(command, response, sizeof(response));

    return rv;
}

int cmd_off()
{
    int rv;
    char response[RESP_LEN_OFF];
    char *command = "off//";

    rv = send_and_receive(command, response, sizeof(response));

    return rv;
}

int cmd_set(int on, int port_number)
{
    int rv;
    char response[RESP_LEN_SET];
    char command[RESP_LEN_SET + 1];

    snprintf(command, sizeof(command), "%02u%c//", (unsigned int) port_number, on ? '+' : '-');

    rv = send_and_receive(command, response, sizeof(response));

    return rv;
}

int main(int argc, char **argv)
{
    char *cmd;
    int rv = 0;
    int port_number;

    if (argc > 2) {
        serial_port = argv[1];
        cmd = argv[2];
        if (!strcmp(cmd, "status")) {
            rv = cmd_ask();
        } else if (!strcmp(cmd, "on_all")) {
            rv = cmd_on();
        } else if (!strcmp(cmd, "off_all")) {
            rv = cmd_off();
        } else if (!strcmp(cmd, "on") || !strcmp(cmd, "off")) {
            if (argc < 4) {
                fprintf(stderr, "No port number specified\n");
                return 1;
            }
            port_number = atoi(argv[3]);
            rv = cmd_set(!strcmp(cmd, "on"), port_number);
        } else {
            fprintf(stderr, "Unknown command\n");
            return 1;
        }
    } else {
        fprintf(stderr, "Usage: %s <serial_port> <status|on_all|off_all|on|off> [port_number]\n", argv[0]);
        return 1;
    }

    if (rv) {
        fprintf(stderr, "Command failed\n");
        return 1;
    }

    return 0;
}
