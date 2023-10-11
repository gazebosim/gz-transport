# Ubuntu 22.04
FROM ubuntu:jammy

ENV TZ=America/Los_Angeles
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

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
        python3-pybind11 \
        sudo \
        gnupg \
        lsb-release \
        wget \
        tzdata \
 && apt-get clean

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
 && sudo apt-get clean

# Install gazebo dependencies
RUN sudo /bin/sh -c 'echo "deb [trusted=yes] http://packages.osrfoundation.org/gazebo/ubuntu-stable `lsb_release -cs` main" > /etc/apt/sources.list.d/gazebo-stable.list' \
 && sudo /bin/sh -c 'wget http://packages.osrfoundation.org/gazebo.key -O - | sudo apt-key add -' \
 && sudo apt-get update \
 && sudo apt-get install -y \
    libgz-cmake3-dev \
    libgz-math7-dev \
    libgz-msgs10-dev \
    libgz-utils2-cli-dev \
 && sudo apt-get clean

# Gazebo transport
RUN git clone https://github.com/gazebosim/gz-transport.git -b gz-transport14 \
 && cd gz-transport \
 && mkdir build \
 && cd build \
 && cmake .. -DCMAKE_INSTALL_PREFIX=/usr \
 && sudo make -j4 install \
 && cd ../..

# Gazebo transport examples
RUN cd gz-transport/example \
 && mkdir build \
 && cd build \
 && cmake .. \
 && make -j4 \
 && cd ../..

# Customize your image here.
# E.g.:
# ENV PATH="/opt/sublime_text:$PATH"
