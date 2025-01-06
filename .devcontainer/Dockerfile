FROM --platform=linux/amd64 ubuntu:22.04
LABEL org.opencontainers.image.authors="support@halon.io"

ARG HALON_REPO_USER
ARG HALON_REPO_PASS

RUN apt-get update

RUN apt-get install -y wget gnupg
RUN apt-get install -y apt-transport-https

RUN wget -qO - https://raw.githubusercontent.com/halon/changelog/master/pgp-keys/7F0A73B5.asc | apt-key add -
RUN echo "deb https://repo.halon.io/ jammy stable" >> /etc/apt/sources.list.d/halon.list
RUN echo "machine repo.halon.io login ${HALON_REPO_USER} password ${HALON_REPO_PASS}" >> /etc/apt/auth.conf
RUN apt-get update && apt-get install -y halon=6.0.3

RUN apt-get install -y build-essential cmake curl autoconf file git
RUN mkdir /tmp/halon
RUN cd /tmp/halon \
    && curl -O -L https://github.com/omniti-labs/jlog/archive/refs/tags/2.6.0.tar.gz \
    && tar -zxvf 2.6.0.tar.gz && cd jlog-2.6.0 \
    && autoconf && CFLAGS=-fPIC ./configure && make && make install \
    && mkdir -p /jlog && cp LICENSE /jlog/LICENSE
RUN rm -rf /tmp/halon
RUN echo -n "UBU2204" > /OSRELEASE.txt

RUN apt-get install -y libcurl4-openssl-dev

RUN /usr/bin/install -d /var/run/halon
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH

RUN mkdir /var/log/halon && chown halon:halon /var/log/halon

RUN apt-get install -y supervisor
COPY ./supervisord.conf /etc/supervisor/conf.d/supervisord.conf
CMD ["/usr/bin/supervisord"]
