FROM --platform=linux/amd64 centos:7
LABEL org.opencontainers.image.authors="support@halon.io"

ARG HALON_REPO_USER
ARG HALON_REPO_PASS

RUN sed -i s/mirror.centos.org/vault.centos.org/g /etc/yum.repos.d/*.repo
RUN sed -i s/^#.*baseurl=http/baseurl=http/g /etc/yum.repos.d/*.repo
RUN sed -i s/^mirrorlist=http/#mirrorlist=http/g /etc/yum.repos.d/*.repo

RUN yum update -y && yum install -y epel-release

RUN echo "[halon]" >> /etc/yum.repos.d/Halon.repo
RUN echo "name=Halon repository" >> /etc/yum.repos.d/Halon.repo
RUN echo "baseurl=https://${HALON_REPO_USER}:${HALON_REPO_PASS}@repo.halon.io/dists/rhel7/amd64/stable" >> /etc/yum.repos.d/Halon.repo
RUN echo "gpgcheck=1" >> /etc/yum.repos.d/Halon.repo
RUN echo "gpgkey=https://raw.githubusercontent.com/halon/changelog/master/pgp-keys/7F0A73B5.asc" >> /etc/yum.repos.d/Halon.repo
RUN echo "enabled=1" >> /etc/yum.repos.d/Halon.repo
RUN yum install -y halon-6.0.3

RUN yum install -y centos-release-scl

RUN sed -i s/mirror.centos.org/vault.centos.org/g /etc/yum.repos.d/*.repo
RUN sed -i s/^#.*baseurl=http/baseurl=http/g /etc/yum.repos.d/*.repo
RUN sed -i s/^mirrorlist=http/#mirrorlist=http/g /etc/yum.repos.d/*.repo

RUN yum install -y devtoolset-8

SHELL ["/usr/bin/scl", "enable", "devtoolset-8"]

COPY scl_enable /usr/bin/scl_enable
ENV BASH_ENV="/usr/bin/scl_enable" \
    ENV="/usr/bin/scl_enable" \
    PROMPT_COMMAND=". /usr/bin/scl_enable"

RUN yum install -y cmake3 make gcc-c++ rpm-build autoconf
RUN mkdir /tmp/halon
RUN cd /tmp/halon \
    && curl -O -L https://github.com/curl/curl/releases/download/curl-7_80_0/curl-7.80.0.tar.gz \
    && echo "dab997c9b08cb4a636a03f2f7f985eaba33279c1c52692430018fae4a4878dc7 curl-7.80.0.tar.gz" | sha256sum -c \
    && tar -zxvf curl-7.80.0.tar.gz && cd curl-7.80.0 \
    && LD_LIBRARY_PATH=/usr/local/lib64 ./configure --without-ssl --without-ldap \
    && make && make install \
    && cp /usr/local/lib/pkgconfig/libcurl.pc /usr/lib64/pkgconfig/ \
    && rm /usr/local/bin/curl
RUN cd /tmp/halon \
    && curl -O -L https://github.com/omniti-labs/jlog/archive/refs/tags/2.6.0.tar.gz \
    && echo "c3c6e745557f789c2cef3e3760e3c68ee585727a0d55c14ad9cb3e8232f4e46b 2.6.0.tar.gz" | sha256sum -c \
    && tar -zxvf 2.6.0.tar.gz && cd jlog-2.6.0 \
    && autoconf && CFLAGS=-fPIC ./configure && make && make install \
    && mkdir -p /jlog && cp LICENSE /jlog/LICENSE
RUN rm -rf /tmp/halon
RUN echo -n "CENTOS7" > /OSRELEASE.txt

COPY build.sh /build.sh
CMD ["/build.sh"]
