FROM ubuntu:20.04 AS builder

# DCAP version (github repo branch, tag or commit hash)
ARG DCAP_VERSION=DCAP_1.14

# update and install packages
RUN DEBIAN_FRONTEND=noninteractive \
    apt-get update -yq && \
    apt-get upgrade -yq && \
    apt-get install -yq --no-install-recommends \
            build-essential \
            ca-certificates \
            curl \
            git \
            zip

# install node.js
RUN curl -sL https://deb.nodesource.com/setup_16.x | bash -
RUN DEBIAN_FRONTEND=noninteractive apt-get install -yq --no-install-recommends nodejs

RUN apt-get clean && rm -rf /var/lib/apt/lists/*

# clone DCAP repo
RUN git clone https://github.com/intel/SGXDataCenterAttestationPrimitives.git -b ${DCAP_VERSION} --depth 1

# set PWD to PCKCertSelection dir
WORKDIR /SGXDataCenterAttestationPrimitives/tools/PCKCertSelection/

# build libPCKCertSelection library and copy to lib folder
RUN make && \
    mkdir -p ../../QuoteGeneration/pccs/lib && \
    cp ./out/libPCKCertSelection.so ../../QuoteGeneration/pccs/lib/ && \
    make clean

# set PWD to PCCS dir
WORKDIR /SGXDataCenterAttestationPrimitives/QuoteGeneration/pccs/

# build pccs
RUN npm config set proxy $http_proxy && \
    npm config set http-proxy $http_proxy && \
    npm config set https-proxy $https_proxy && \
    npm config set engine-strict true && \
    npm install

# build final image
FROM ubuntu:20.04

ARG USER=pccs
ARG UID=65333

# create user and a group
RUN useradd -M -U ${USER} --uid=${UID} -s /bin/false

COPY --from=builder /usr/bin/node /usr/bin/node
COPY --from=builder --chown=${USER}:${USER} /SGXDataCenterAttestationPrimitives/QuoteGeneration/pccs/ /opt/intel/pccs/

WORKDIR /opt/intel/pccs/
USER ${USER}

# entrypoint to start pccs
ENTRYPOINT ["/usr/bin/node",  "-r", "esm", "pccs_server.js"]
