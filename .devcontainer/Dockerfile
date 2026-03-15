FROM mcr.microsoft.com/devcontainers/base:bookworm AS buildbase
RUN apt-get update && apt-get install -y build-essential gawk flex git gettext libncurses5-dev libssl-dev zlib1g-dev unzip wget file rsync && rm -rf /var/lib/apt/lists/*

FROM buildbase AS sdkdownload

ADD http://lnxpps.de/can2udp/srseII/bin/openwrt-sdk-ramips-mt76x8_gcc-8.4.0_musl.Linux-x86_64.tar.xz /tmp/openwrt-sdk.tar.xz
RUN cd / && tar -xJf /tmp/openwrt-sdk.tar.xz
RUN cd /openwrt-sdk-ramips-mt76x8_gcc-8.4.0_musl.Linux-x86_64 && ./scripts/feeds update base packages luci routing telephony openwrtfiles openwrtmisc openwrtnetem node

FROM buildbase

COPY --from=sdkdownload --chown=vscode /openwrt-sdk-ramips-mt76x8_gcc-8.4.0_musl.Linux-x86_64 /sdk
RUN echo '[ ! -z "$TERM" -a -r /etc/motd ] && cat /etc/motd' >> /etc/bash.bashrc
COPY motd /etc/motd
