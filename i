set -ex
sudo make
sudo make install
#statslog=$(date +%FT%T%:z)-stats.log
statslog="$HOME/keyloggerstats.log"
sudo keylogger "$statslog"
#grep '\[' "$statslog" | sort -n -k2
#grep -v '\[' "$statslog" | sort -n -k2
grep '^[a-zA-Z0-9]' ~/keyloggerstats.log | sort -n -k2
grep -v '^[a-zA-Z0-9]' ~/keyloggerstats.log | sort -n -k2
