
FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y --no-install-recommends xinetd && \
    rm -rf /var/lib/apt/lists/* && \
    useradd -m -d /home -s /bin/sh ctf

COPY phantom /home/phantom
COPY flag.txt /home/flag.txt
COPY xinetd.conf /etc/xinetd.d/phantom.conf
COPY entrypoint.sh /entrypoint.sh

RUN chmod +x /home/phantom /entrypoint.sh && \
    chmod 440 /home/flag.txt && \
    chown root:root /home/flag.txt

EXPOSE 30207

ENTRYPOINT ["/entrypoint.sh"]
