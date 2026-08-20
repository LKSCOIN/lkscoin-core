FROM phusion/baseimage:bionic-1.0.0
LABEL maintainer="Lks Developers <dev@lksfoundation.org>"
LABEL description="Dockerised LksCore, built from Travis"

ARG USER_ID
ARG GROUP_ID

ENV HOME /home/lks

# add user with specified (or default) user/group ids
ENV USER_ID ${USER_ID:-1000}
ENV GROUP_ID ${GROUP_ID:-1000}
RUN groupadd -g ${GROUP_ID} lks && \
    useradd -u ${USER_ID} -g lks -s /bin/bash -m -d /home/lks lks && \
    mkdir /home/lks/.lkscore && \
    chown ${USER_ID}:${GROUP_ID} -R /home/lks

COPY bin/* /usr/local/bin/

RUN chmod a+x /usr/local/bin/*

USER lks

VOLUME ["/home/lks"]

EXPOSE 9998 9999 19998 19999

WORKDIR /home/lks
