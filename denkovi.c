// gcc -o denkovi -Wall -Werror denkovi.c

#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/file.h>

#define RESP_LEN_STATUS           2
#define  CMD_LEN_STATUS           5

#define RESP_LEN_ON_ALL           4
#define  CMD_LEN_ON_ALL           RESP_LEN_ON_ALL

#define RESP_LEN_OFF_ALL          5
#define  CMD_LEN_OFF_ALL          RESP_LEN_OFF_ALL

#define RESP_LEN_ON_OFF_SINGLE    5
#define  CMD_LEN_ON_OFF_SINGLE    RESP_LEN_ON_OFF_SINGLE

#define RESP_LEN_SET              5
#define  CMD_LEN_SET              RESP_LEN_SET

char *serial_port = NULL;

int send_and_receive(unsigned char *command, int command_len, unsigned char *response, int response_len)
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
        t.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);

        if (tcsetattr(fd, TCSANOW, &t) != 0) {
            fprintf(stderr, "Cannot set serial port attributes\n");
            rv = -1;
            goto end;
        }
        tcflush(fd, TCIFLUSH);
    }

    write(fd, command, command_len);
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

int cmd_status(int hex)
{
    int rv;
    unsigned char response[RESP_LEN_STATUS];
    unsigned char command[CMD_LEN_STATUS] = { 'a', 's', 'k', '/', '/' };
    uint16_t relay_states;
    int relay_state;

    rv = send_and_receive(command, sizeof(command), response, sizeof(response));
    if (rv) {
        goto end;
    }

    relay_states = ((response[0] << 8) | response[1]);
    if (hex) {
        printf("0x%04x\n", relay_states);
    } else {
        for (int i = 1; i <= 16; i++) {
            relay_state = (relay_states & 0x8000) ? 1 : 0;
            relay_states <<= 1;
            printf("%-2i %i\n", i, relay_state);
        }
    }

end:

    return rv;
}

int cmd_on_all()
{
    int rv;
    unsigned char response[RESP_LEN_ON_ALL];
    unsigned char command[CMD_LEN_ON_ALL] = { 'o', 'n', '/', '/' };

    rv = send_and_receive(command, sizeof(command), response, sizeof(response));

    return rv;
}

int cmd_off_all()
{
    int rv;
    unsigned char response[RESP_LEN_OFF_ALL];
    unsigned char command[CMD_LEN_OFF_ALL] = { 'o', 'f', 'f', '/', '/' };

    rv = send_and_receive(command, sizeof(command), response, sizeof(response));

    return rv;
}

int cmd_on_off_single(int on, int relay_number)
{
    int rv;
    unsigned char response[RESP_LEN_ON_OFF_SINGLE];
    unsigned char command[CMD_LEN_ON_OFF_SINGLE + 1];

    snprintf((char *) command, sizeof(command), "%02u%c//", (unsigned int) relay_number, on ? '+' : '-');

    rv = send_and_receive(command, CMD_LEN_ON_OFF_SINGLE, response, sizeof(response));

    return rv;
}

int cmd_set(unsigned int relay_bitmap)
{
    int rv;
    unsigned char response[RESP_LEN_SET];
    unsigned char command[CMD_LEN_SET];

    command[0] = 'x';
    command[1] = (relay_bitmap >> 8) & 0xff;
    command[2] = relay_bitmap & 0xff;
    command[3] = '/';
    command[4] = '/';

    rv = send_and_receive(command, sizeof(command), response, sizeof(response));

    return rv;
}

int main(int argc, char **argv)
{
    char *cmd;
    int rv = 0;
    int lock_fd;

    lock_fd = open("/tmp/denkovi.lock", O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (lock_fd < 0) {
        fprintf(stderr, "Cannot open lock file\n");
        return 1;
    }
    rv = flock(lock_fd, LOCK_EX);
    if (rv) {
        fprintf(stderr, "Cannot lock file\n");
        return 1;
    }

    if (argc > 2) {
        serial_port = argv[1];
        cmd = argv[2];
        if (!strcmp(cmd, "status")) {
            rv = cmd_status(0);
        } else if (!strcmp(cmd, "status_hex")) {
            rv = cmd_status(1);
        } else if (!strcmp(cmd, "on_all")) {
            rv = cmd_on_all();
        } else if (!strcmp(cmd, "off_all")) {
            rv = cmd_off_all();
        } else if (!strcmp(cmd, "on") || !strcmp(cmd, "off")) {
            if (argc < 4) {
                fprintf(stderr, "No relay number specified\n");
                return 1;
            }
            unsigned int relay_number = strtol(argv[3], NULL, 0);
            if (relay_number < 1 || relay_number > 16) {
                fprintf(stderr, "Invalid relay number\n");
                return 1;
            }
            rv = cmd_on_off_single(!strcmp(cmd, "on"), relay_number);
        } else if (!strcmp(cmd, "set")) {
            if (argc < 4) {
                fprintf(stderr, "No relay bitmap specified\n");
                return 1;
            }
            unsigned int relay_bitmap = strtol(argv[3], NULL, 0);
            if (relay_bitmap > 0xffff) {
                fprintf(stderr, "Invalid relay bitmap\n");
                return 1;
            }
            rv = cmd_set(relay_bitmap);
        } else {
            fprintf(stderr, "Unknown command\n");
            return 1;
        }
    } else {
        fprintf(stderr, "Usage: %s <serial_port> <command> [command_args]\n", argv[0]);
        fprintf(stderr, "Where command is:\n");
        fprintf(stderr, "    status\n");
        fprintf(stderr, "    status_hex\n");
        fprintf(stderr, "    on_all\n");
        fprintf(stderr, "    off_all\n");
        fprintf(stderr, "    on <relay_number>\n");
        fprintf(stderr, "    off <relay_number>\n");
        fprintf(stderr, "    set <relay_bitmap>\n");
        fprintf(stderr, "Where relay_number is: 1..16\n");
        fprintf(stderr, "      relay_bitmap is: 16-bit unsigned integer - MSB for relay 1, LSB for relay 16\n");
        return 1;
    }

    if (rv) {
        fprintf(stderr, "Command failed\n");
        return 1;
    }

    return 0;
}
