## OpenDLV Microservice for OXTS GPS/INSS Units

This repository provides source code to interface with an OXTS GPS/INSS unit for the OpenDLV software ecosystem.

[![Build Status](https://travis-ci.org/se-research/opendlv.sensors.oxts.svg?branch=master)](https://travis-ci.org/se-research/opendlv.sensors.oxts) [![License: GPLv3](https://img.shields.io/badge/license-GPL--3-blue.svg
)](https://www.gnu.org/licenses/gpl-3.0.txt)

## Table of Contents
* [Dependencies](#dependencies)
* [Usage](#usage)
* [Build from sources on the example of Ubuntu 16.04 LTS](#build-from-sources-on-the-example-of-ubuntu-1604-lts)
* [License](#license)


## Dependencies
You need a C++14-compliant compiler to compile the project and [libcluon](https://github.com/chrberger/libcluon) for publish/subscribe communication and to interface with the OXTS GPS/INSS unit. This project ships the following dependency as part of the source distribution:

* [Unit Test Framework Catch2](https://github.com/catchorg/Catch2/releases/tag/v2.0.1) - [![License: Boost Software License v1.0](https://img.shields.io/badge/License-Boost%20v1-blue.svg)](http://www.boost.org/LICENSE_1_0.txt)


## Usage
This microservice is created automatically on changes to this repository via Docker's public registry for [x86_64](https://hub.docker.com/r/seresearch/opendlv.sensors.oxts) and [armhf](https://hub.docker.com/r/seresearch/opendlv.sensors.oxts-armhf). To simplify deployment, a [multi](https://hub.docker.com/r/seresearch/opendlv.sensors.oxts-multi)-architecture image is also provided.

To run this microservice, simply start it as follows to get a usage description:
```
docker run --rm seresearch/opendlv.sensors.oxts-multi
```

To run this microservice for connecting to an OXTS GPS/INSS unit broadcasting data to `192.168.1.255:3000` and to publish the messages according to OpenDLV Standard Message Set into session 111, simply start it as follows to get a usage description:
```
docker run --rm --net=host seresearch/opendlv.sensors.oxts-multi 192.168.1.255 3000 111
```

## Build from sources on the example of Ubuntu 16.04 LTS
To build this software, you need cmake, C++14 or newer, make, and [libcluon](https://github.com/chrberger/libcluon). You can either compile liblcuon from sources or use Launchpad.

To install libcluon from Launchpad, simply run the following commands on your Ubuntu 16.04 LTS:
```
sudo add-apt-repository ppa:chrberger/libcluon
sudo apt-get update
sudo apt-get install libcluon
```

To build this project, just run `cmake` and `make` as follows:
```
mkdir build && cd build
cmake -D CMAKE_BUILD_TYPE=Release ..
make && make test && make install
```


## License

* This project is released under the terms of the GNU GPLv3 License

