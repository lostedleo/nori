#!/usr/bin/env python
# coding=utf-8

from _nrai import BalanceInt, BalanceFloat, BalanceDouble

def main():
    balance1 = BalanceInt(2, 1)
    print(balance1.x(), balance1.y(), balance1.connection())
    balance1.grow()
    print(balance1.x(), balance1.y(), balance1.connection())

    balance2 = BalanceFloat(2, 2)
    print(balance2.x(), balance2.y(), balance2.connection())
    balance2.grow()
    print(balance2.x(), balance2.y(), balance2.connection())

if __name__ == '__main__':
    main()
