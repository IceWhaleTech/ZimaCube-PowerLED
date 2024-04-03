#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/io.h>
#include <stdint.h>

#define REG 0x2E
#define VAL 0x2F
#define LDNREG 0x07
#define GPIOLDN 0x07
#define CMDF9 0xF9

#define CMD66 0x66
#define CMD62 0x62
#define ADR   0xF8
#define CMD81 0x81

//-------------------n100 start-------------------

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
    write_reg(CMDF9, 0x01);
    exit_conf_mode();
}

void cmd_off() {
    enter_conf_mode();
    select_ldn(GPIOLDN);
    write_reg(0xC3, 0x08);
    write_reg(0xCB, 0x08);
    write_reg(0xB3, 0x08);
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

//----------------------------------n100 end / 1235u start--------------------------------
void OemEcIbFree()
{
    if (iopl(3)) {
        fprintf(stderr, "Can't get IO permissions\n");
        exit(1);
    }
    unsigned char Status;
    do {
        Status = inb(CMD66);
        outb(0x80, 0xEB);
    } while (Status & 2);
}

void OemEcObFull()
{
    unsigned char Status;
    do {
        Status = inb(CMD66);
        outb(0x80, 0xEC);
    } while (!(Status & 1));
}

void uexit_conf_mode() {
    if (iopl(0)) {
        fprintf(stderr, "Can't drop IO permissions\n");
        exit(1);
    }
}

unsigned char uread_reg(unsigned char reg) {
    OemEcIbFree();
    outb(0x80, CMD66);
    OemEcIbFree();
    outb(ADR, CMD62);
    OemEcObFull();
    return inb(CMD62);
}

void uwrite_reg(unsigned char cmd, unsigned char adr,unsigned char val) {
    OemEcIbFree();
    outb(cmd, CMD66);
    OemEcIbFree();
    outb(adr, CMD62);
    OemEcIbFree();
    outb(val, CMD62);
}

void ucmd_on() {
    uwrite_reg(CMD81,ADR,0x00);
    uexit_conf_mode();
}
void ucmd_off() {
    uwrite_reg(CMD81,ADR,0x01);
    uexit_conf_mode();
}

void ucmd_blink(char* interval) {
    unsigned char val;
    if (strcmp(interval, "slow") == 0) {
        val = 0x02;
    } else if (strcmp(interval, "medium") == 0) {
        val = 0x04;
    } else if (strcmp(interval, "fast") == 0) {
        val = 0x08;
    } else {
        fprintf(stderr, "Invalid interval\n");
        exit(1);
    }
    uwrite_reg(CMD81,ADR,val);
    uexit_conf_mode();
}

void ucmd_status() {
    unsigned char val = uread_reg(CMD81);

    if (val == 0x00) {
        printf("Power LED is on\n");
    } else if (val == 0x02) {
        printf("Power LED is blinking slowly\n");
    } else if (val == 0x04) {
        printf("Power LED is blinking medium\n");
    } else if (val == 0x08) {
        printf("Power LED is blinking fast\n");
    } else {
        printf("Power LED is %x (unknown)\n", val);
    }
}
//----------------------------------1235u end--------------------------------

int main(int argc, char** argv) {
    if (geteuid() != 0) {
        fprintf(stderr, "The program is not running with root privileges. Please run as root.\n");
        exit(1);
    }

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <project> <command> [args]\n", argv[0]);
        exit(1);
    }

    if (strcmp(argv[1], "pro") == 0) {

        if (strcmp(argv[2], "on") == 0) {
            ucmd_on();
        } else if (strcmp(argv[2], "blink") == 0) {
            if (argc < 4) {
                fprintf(stderr, "Usage: %s blink <interval>\n", argv[0]);
                exit(1);
            }
            ucmd_blink(argv[3]);
        } else if (strcmp(argv[2], "status") == 0) {
            ucmd_status();
        } else if(strcmp(argv[2], "off") == 0) {
            ucmd_off();
        } else {
            fprintf(stderr, "Invalid command: %s\n", argv[2]);
            exit(1);
        }

        return 0;
    } else {

        if (strcmp(argv[2], "on") == 0) {
            cmd_on();
        } else if (strcmp(argv[2], "blink") == 0) {
            if (argc < 4) {
                fprintf(stderr, "Usage: %s blink <interval>\n", argv[0]);
                exit(1);
            }
            cmd_blink(argv[3]);
        } else if (strcmp(argv[2], "status") == 0) {
            cmd_status();
        } else if (strcmp(argv[2], "off") == 0) {
            cmd_off();
        } else {
            fprintf(stderr, "Invalid command: %s\n", argv[2]);
            exit(1);
        }
    }
    return 0;
}