s88udp systemd install
----------------------

```
cd
wget https://github.com/GBert/railroad/raw/master/can2udp/systemd/s88udp.service
# edit parameters as you need. e.g. -m# number of modules or -p #portnumber
sudo mv s88udp.service /lib/systemd/system/
sudo ln -s /lib/systemd/system/s88udp.service /etc/systemd/system/s88udp.service
sudo systemctl daemon-reload
sudo systemctl enable --now s88udp
```
