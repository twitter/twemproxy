FROM ubuntu:16.04
MAINTAINER Devin Ekins <devops@keen.io>

# Setup the dependencies
RUN apt-get update -y && \
    apt-get install -y libtool make automake

# Install Twemproxy
RUN curl https://github.com/twitter/twemproxy/archive/v0.4.1.tar.gz -O && \
    tar -xzf v0.4.1.tar.gz && \
    cd twemproxy-0.4.1 && \
    autoreconf -fvi && \
    ./configure --enable-debug=full && \
    make

# Copy the configuration files for all twemproxy environments.
COPY twemproxy_dev_conf.yaml .
COPY twemproxy_stage_conf.yaml .
COPY twemproxy_prod_conf.yaml .

# Expose Twemproxy Ports
EXPOSE 22122 22222

# Declare variable
ARG CONFIG_FILE

# Start Twemproxy
ENTRYPOINT src/nutcracker --conf-file=$CONFIG_FILE --mbuf-size=102400
