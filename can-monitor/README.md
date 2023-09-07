## Märlin CAN monitor

This Git (OpenWRT feed) contains a Märklin CAN frame reader to print human readable informataions

![terminal](https://github.com/GBert/railroad/raw/master/can-monitor/docs/can-monitor_5.13_s.png)

```
Usage: can-monitor -i <can|net interface>
   Version 5.14

         -i <can|net int>  CAN or network interface - default can0
         -r <pcap file>    read PCAP file instead from CAN socket
         -s                select only network internal frames
         -l <candump file> read candump file instead from CAN socket
         -t <rocrail file> read Rocrail file instead from CAN socket
         -d                dump to candump file

         -v                verbose output for TCP/UDP and errorframes

         -x                expose config data

         -h                show this help
```

### Using under Debian/Ubuntu

To compile can-monitor under Debian/Ubuntu you need to install some libs and do:
```
sudo apt install zlib1g-dev libpcap-dev
git clone https://github.com/GBert/railroad.git
cd railroad/can-monitor/src
cd src
make
```

