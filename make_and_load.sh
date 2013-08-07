make && cp *test.o test && make && fsync * && fsync .git/* && sync && sleep 1 && sudo make load

