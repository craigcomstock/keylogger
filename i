set -ex
sudo make
sudo make install
#statslog=$(date +%FT%T%:z)-stats.log
statslog="$HOME/keyloggerstats.log"
sudo keylogger "$statslog"
#grep '\[' "$statslog" | sort -n -k2
#grep -v '\[' "$statslog" | sort -n -k2
log
$HOME/2024-04-04-keyloggerstats.log
grep '^[a-zA-Z0-9]' $HOME/2024-04-04-keyloggerstats.log | sort -n -k2
grep -v '^[a-zA-Z0-9]' $HOME/2024-04-04-keyloggerstats.log | sort -n -k2
