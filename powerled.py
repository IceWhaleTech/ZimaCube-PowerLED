#!/usr/bin/env python3

import argparse
import ctypes
import os
import sys

from portio import inb, outb, iopl

# Address of the Super I/O device
REG = 0x2E
VAL = 0x2F
LDNREG = 0x07
GPIOLDN = 0x07


def enter_conf_mode():
    if iopl(3):
        raise Exception("Can't get IO permissions")
    outb(0x87, REG)
    outb(0x01, REG)
    outb(0x55, REG)
    outb(0x55, REG)


def select_ldn(ldn):
    outb(LDNREG, REG)
    outb(ldn, VAL)


def read_reg(reg):
    outb(reg, REG)
    return inb(VAL)


def write_reg(reg, val):
    outb(reg, REG)
    outb(val, VAL)


def exit_conf_mode():
    outb(0x02, REG)
    outb(0x02, VAL)
    if iopl(0):
        raise Exception("Can't drop IO permissions")


def build_arg_parser():
    description = "ZimaCube Power LED Control"
    parser = argparse.ArgumentParser(description=description)

    subparsers = parser.add_subparsers(dest="command")

    on = subparsers.add_parser("on", help="Turn on the power LED")
    on.set_defaults(func=cmd_on)

    blink = subparsers.add_parser("blink", help="Blink the power LED")
    blink.add_argument(
        "--interval", "-I", type=str, default="medium", help="slow, medium, or fast"
    )
    blink.set_defaults(func=cmd_blink)

    status = subparsers.add_parser("status", help="Show the power LED status")
    status.set_defaults(func=cmd_status)

    return parser


def cmd_on(args):
    enter_conf_mode()
    select_ldn(GPIOLDN)
    write_reg(0xF9, 0x01)
    exit_conf_mode()


def cmd_blink(args):
    if args.interval == "slow":
        val = 0x04
    elif args.interval == "medium":
        val = 0x02
    elif args.interval == "fast":
        val = 0x00
    else:
        raise Exception("Invalid interval")

    enter_conf_mode()
    select_ldn(GPIOLDN)
    write_reg(0xF9, val)
    exit_conf_mode()

def cmd_status(args):
    enter_conf_mode()
    select_ldn(GPIOLDN)
    val = read_reg(0xF9)
    exit_conf_mode()

    if val == 0x01:
        print("Power LED is on")
    elif val == 0x04:
        print("Power LED is blinking slowly")
    elif val == 0x02:
        print("Power LED is blinking medium")
    elif val == 0x00:
        print("Power LED is blinking fast")
    else:
        print("Power LED is " + hex(val) + " (unknown)")


def main():
    parser = build_arg_parser()
    args = parser.parse_args()

    if args.command:
        args.func(args)
    else:
        parser.print_help()
        return

    sys.exit(0)


if __name__ == "__main__":
    if os.geteuid() != 0:
        print("The script is not running with root privileges. Please run as root.")
        sys.exit(1)
    main()
