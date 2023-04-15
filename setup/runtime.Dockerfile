FROM ubuntu:16.04
RUN apt-get update
RUN apt-get install -y uuid-dev
RUN apt-get install -y libcurl4-openssl-dev