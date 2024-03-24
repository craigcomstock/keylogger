set -ex
sudo make
sudo make install
sudo keylogger
grep '\[' stats.log | sort -n
grep -v '\[' stats.log | sort -n
