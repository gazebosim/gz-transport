# Ubuntu 18.04 with nvidia-docker2 beta opengl support
FROM nvidia/opengl:1.0-glvnd-devel-ubuntu18.04

# Tools I find useful during development
RUN apt-get update \
 && apt-get install -y \
        cmake \
        pkg-config \
        cppcheck \
        git \
        mercurial \
        build-essential \
        curl \
        libprotobuf-dev \
        protobuf-compiler \
        libprotoc-dev \
        libzmq3-dev \
        net-tools \
        uuid-dev \
        doxygen \
        ruby-ronn \
        libsqlite3-dev \
        g++-8 \
        sudo \
 && apt-get clean

# Use GCC 8
 RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 800 \
        --slave /usr/bin/g++ g++ /usr/bin/g++-8 \
        --slave /usr/bin/gcov gcov /usr/bin/gcov-8

# Set USER and GROUP
ARG USER=developer
ARG GROUP=developer

# Add a user with the same user_id as the user outside the container
# Requires a docker build argument `user_id`.

RUN curl -SsL https://github.com/boxboat/fixuid/releases/download/v0.4/fixuid-0.4-linux-amd64.tar.gz | tar -C /usr/local/bin -xzf - && \
    chown root:root /usr/local/bin/fixuid && \
    chmod 4755 /usr/local/bin/fixuid && \
    mkdir -p /etc/fixuid && \
    printf "user: $USER\ngroup: $GROUP\n" > /etc/fixuid/config.yml

RUN addgroup --gid 1000 $USER && \
    adduser --uid 1000 --ingroup $USER --home /home/$USER --shell /bin/sh --disabled-password --gecos "" $USER

RUN adduser $USER sudo \
 && echo "$USER ALL=NOPASSWD: ALL" >> /etc/sudoers.d/$USER

# Commands below run as the developer user.
USER $USER:$GROUP

# When running a container start in the developer's home folder.
WORKDIR /home/$USER

RUN export DEBIAN_FRONTEND=noninteractive \
 && sudo apt-get update \
 && sudo -E apt-get install -y \
    tzdata \
 && sudo ln -fs /usr/share/zoneinfo/America/Los_Angeles /etc/localtime \
 && sudo dpkg-reconfigure --frontend noninteractive tzdata \
 && sudo apt-get clean

# lcov
RUN git clone https://github.com/linux-test-project/lcov.git \
 && cd lcov \
 && sudo make install \
 && cd ..

# Ignition cmake
RUN hg clone http://bitbucket.org/ignitionrobotics/ign-cmake -b ign-cmake2 \
 && cd ign-cmake \
 && mkdir build \
 && cd build \
 && cmake .. -DCMAKE_INSTALL_PREFIX=/usr \
 && sudo make -j4 install \
 && cd ../..

# Ignition math
RUN hg clone http://bitbucket.org/ignitionrobotics/ign-math -b ign-math6 \
 && cd ign-math \
 && mkdir build \
 && cd build \
 && cmake .. -DCMAKE_INSTALL_PREFIX=/usr \
 && sudo make -j4 install \
 && cd ../..

# Ignition msgs
RUN hg clone http://bitbucket.org/ignitionrobotics/ign-msgs -b ign-msgs3 \
 && cd ign-msgs \
 && mkdir build \
 && cd build \
 && cmake .. -DCMAKE_INSTALL_PREFIX=/usr \
 && sudo make -j4 install \
 && cd ../..

# Ignition transport
RUN hg clone http://bitbucket.org/ignitionrobotics/ign-transport -b new_horizons \
 && cd ign-transport \
 && mkdir build \
 && cd build \
 && cmake .. -DCMAKE_INSTALL_PREFIX=/usr \
 && sudo make -j4 install \
 && cd ../..

# Ignition transport examples
RUN cd ign-transport/example \
 && mkdir build \
 && cd build \
 && cmake .. \
 && make -j4 \
 && cd ../..

# Customize your image here.
# E.g.:
# ENV PATH="/opt/sublime_text:$PATH"
