#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/io.h>

#define REG 0x2E
#define VAL 0x2F
#define LDNREG 0x07
#define GPIOLDN 0x07

void enter_conf_mode() {
    if (iopl(3)) {
        fprintf(stderr, "Can't get IO permissions\n");
        exit(1);
    }
    outb(0x87, REG);
    outb(0x01, REG);
    outb(0x55, REG);
    outb(0x55, REG);
}

void select_ldn(unsigned char ldn) {
    outb(LDNREG, REG);
    outb(ldn, VAL);
}

unsigned char read_reg(unsigned char reg) {
    outb(reg, REG);
    return inb(VAL);
}

void write_reg(unsigned char reg, unsigned char val) {
    outb(reg, REG);
    outb(val, VAL);
}

void exit_conf_mode() {
    outb(0x02, REG);
    outb(0x02, VAL);
    if (iopl(0)) {
        fprintf(stderr, "Can't drop IO permissions\n");
        exit(1);
    }
}

void cmd_on() {
    enter_conf_mode();
    select_ldn(GPIOLDN);
    write_reg(0xF9, 0x01);
    exit_conf_mode();
}

void cmd_blink(char* interval) {
    unsigned char val;
    if (strcmp(interval, "slow") == 0) {
        val = 0x04;
    } else if (strcmp(interval, "medium") == 0) {
        val = 0x02;
    } else if (strcmp(interval, "fast") == 0) {
        val = 0x00;
    } else {
        fprintf(stderr, "Invalid interval\n");
        exit(1);
    }

    enter_conf_mode();
    select_ldn(GPIOLDN);
    write_reg(0xF9, val);
    exit_conf_mode();
}

void cmd_status() {
    enter_conf_mode();
    select_ldn(GPIOLDN);
    unsigned char val = read_reg(0xF9);
    exit_conf_mode();

    if (val == 0x01) {
        printf("Power LED is on\n");
    } else if (val == 0x04) {
        printf("Power LED is blinking slowly\n");
    } else if (val == 0x02) {
        printf("Power LED is blinking medium\n");
    } else if (val == 0x00) {
        printf("Power LED is blinking fast\n");
    } else {
        printf("Power LED is %x (unknown)\n", val);
    }
}

int main(int argc, char** argv) {
    if (geteuid() != 0) {
        fprintf(stderr, "The program is not running with root privileges. Please run as root.\n");
        exit(1);
    }

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args]\n", argv[0]);
        exit(1);
    }

    if (strcmp(argv[1], "on") == 0) {
        cmd_on();
    } else if (strcmp(argv[1], "blink") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s blink <interval>\n", argv[0]);
            exit(1);
        }
        cmd_blink(argv[2]);
    } else if (strcmp(argv[1], "status") == 0) {
        cmd_status();
    } else {
        fprintf(stderr, "Invalid command: %s\n", argv[1]);
        exit(1);
    }

    return 0;
}