
FROM debian:bullseye

RUN apt-get update
RUN apt-get -y upgrade
RUN apt-get -y install build-essential cmake libfltk1.3-dev

