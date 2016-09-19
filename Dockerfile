FROM ubuntu:16.04
MAINTAINER Devin Ekins <devops@keen.io>

# Setup the dependencies
RUN apt-get update -y
RUN apt-get install -y apt-utils
RUN apt-get install -y libtool make automake

# Copy local repo into the container
ADD . /twemproxy
WORKDIR /twemproxy

# Install Twemproxy
RUN autoreconf -fvi && \
    ./configure --enable-debug=full && \
    make

# Expose Twemproxy Ports
EXPOSE 22122 22222

# Declare variable
ARG CONFIG_FILE

# Start Twemproxy
ENTRYPOINT src/nutcracker --conf-file=$CONFIG_FILE --mbuf-size=102400
