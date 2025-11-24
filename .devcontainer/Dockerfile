FROM docker.io/espressif/idf:v5.5.1

ARG DEBIAN_FRONTEND=nointeractive
ARG CONTAINER_USER=esp
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN apt-get update \
    && apt install -y -q \
    cmake \
    git \
    libglib2.0-0 \
    libnuma1 \
    libpixman-1-0 \
    && rm -rf /var/lib/apt/lists/*

# QEMU
ENV QEMU_REL=esp_develop_8.2.0_20240122
ENV QEMU_SHA256=e7c72ef5705ad1444d391711088c8717fc89f42e9bf6d1487f9c2a326b8cfa83
ENV QEMU_DIST=qemu-xtensa-softmmu-${QEMU_REL}-x86_64-linux-gnu.tar.xz
ENV QEMU_URL=https://github.com/espressif/qemu/releases/download/esp-develop-8.2.0-20240122/${QEMU_DIST}

ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

RUN wget --no-verbose ${QEMU_URL} \
    && echo "${QEMU_SHA256} *${QEMU_DIST}" | sha256sum --check --strict - \
    && tar -xf $QEMU_DIST -C /opt \
    && rm ${QEMU_DIST}

ENV PATH=/opt/qemu/bin:${PATH}

# Handle user/group creation - the v5.5.1 base image has idf:idf (1000:1000)
# We need to either use that or rename it to esp:esp
RUN if id -u $USER_UID >/dev/null 2>&1; then \
        EXISTING_USER=$(id -nu $USER_UID); \
        if [ "$EXISTING_USER" != "$CONTAINER_USER" ]; then \
            usermod -l $CONTAINER_USER $EXISTING_USER; \
            groupmod -n $CONTAINER_USER $(id -gn $USER_UID); \
            usermod -d /home/$CONTAINER_USER -m $CONTAINER_USER; \
        fi; \
    else \
        groupadd --gid $USER_GID $CONTAINER_USER; \
        adduser --uid $USER_UID --gid $USER_GID --disabled-password --gecos "" ${CONTAINER_USER}; \
    fi \
    && usermod -a -G root $CONTAINER_USER \
    && usermod -aG dialout $CONTAINER_USER \
    && usermod -a -G 20 $CONTAINER_USER \
    && apt-get update && apt-get install -y sudo usbutils \
    && echo $CONTAINER_USER ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$CONTAINER_USER \
    && chmod 0440 /etc/sudoers.d/$CONTAINER_USER \
    && rm -rf /var/lib/apt/lists/*

RUN chmod -R 775 /opt/esp/python_env/

USER ${CONTAINER_USER}
ENV USER=${CONTAINER_USER}
WORKDIR /home/${CONTAINER_USER}

RUN echo "source /opt/esp/idf/export.sh > /dev/null 2>&1" >> ~/.bashrc

ENTRYPOINT [ "/opt/esp/entrypoint.sh" ]

CMD ["/bin/bash", "-c"]
