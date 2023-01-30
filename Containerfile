
FROM debian:bullseye

RUN apt-get update 1>&2 && apt-get install -y build-essential cmake libfltk1.3-dev file 1>&2

