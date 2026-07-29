FROM phusion/baseimage:bionic-1.0.0
LABEL maintainer="Lks Developers <dev@lksfoundation.org>"
LABEL description="Dockerised LksCore, built from Travis"

ARG USER_ID
ARG GROUP_ID

ENV HOME /lks

# add user with specified (or default) user/group ids
ENV USER_ID ${USER_ID:-1000}
ENV GROUP_ID ${GROUP_ID:-1000}
RUN groupadd -g ${GROUP_ID} lks
RUN useradd -u ${USER_ID} -g lks -s /bin/bash -m -d /lks lks
RUN mkdir /lks/.lkscore
RUN chown lks:lks -R /lks

COPY bin/* /usr/local/bin/

RUN chmod a+x /usr/local/bin/*

USER lks

VOLUME ["/lks"]

EXPOSE 9998 9999 19998 19999

WORKDIR /lks
