/*
 * Copyright (C) 2018  Christian Berger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cluon-complete.hpp"
#include "opendlv-standard-message-set.hpp"

#include "oxts-decoder.hpp"

#include <array>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

int main(int argc, char **argv) {
    int retCode{0};
    const std::string PROGRAM(argv[0]);
    if (4 != argc) {
        std::cerr << PROGRAM << " decodes latitude/longitude/heading from an OXTS GPS/INSS unit and publishes it to a running OpenDaVINCI session using the OpenDLV Standard Message Set." << std::endl;
        std::cerr << "Usage:   " << PROGRAM << " <IPv4-address> <port> <OpenDaVINCI session>" << std::endl;
        std::cerr << "Example: " << PROGRAM << " 0.0.0.0 3000 111" << std::endl;
        retCode = 1;
    } else {
        // Interface to a running OpenDaVINCI session.
        const std::string OPENDAVINCI_ADDRESS{"225.0.0." + std::string{argv[3]}};
        constexpr uint16_t OPENDAVINCI_PORT{12175};
        cluon::UDPSender toOpenDaVINCISender{OPENDAVINCI_ADDRESS, OPENDAVINCI_PORT};

        // Helper function to send data to OpenDaVINCI session.
        auto sendToOpenDaVINCI = [&toOD4Sender = toOpenDaVINCISender](uint32_t dataType, std::string &&payload, const std::chrono::system_clock::time_point &tp){
            cluon::data::TimeStamp now;
            {
                // Transform chrono time representation to same behavior as gettimeofday.
                typedef std::chrono::duration<int32_t> seconds_type;
                typedef std::chrono::duration<int64_t, std::micro> microseconds_type;

                auto duration = tp.time_since_epoch();
                seconds_type s = std::chrono::duration_cast<seconds_type>(duration);
                microseconds_type us = std::chrono::duration_cast<microseconds_type>(duration);
                microseconds_type partial_us = us - std::chrono::duration_cast<microseconds_type>(s);

                now.seconds(s.count()).microseconds(partial_us.count());
            }

            cluon::data::Envelope envelope;
            {
                envelope.dataType(dataType);
                envelope.serializedData(payload);
                envelope.sent(now);
                envelope.sampleTimeStamp(now);
            }

            std::string dataToSend;
            {
                cluon::ToProtoVisitor protoEncoder;
                envelope.accept(protoEncoder);

                const std::string tmp{protoEncoder.encodedData()};
                uint32_t length{static_cast<uint32_t>(tmp.size())};
                length = htole32(length);

                // Add OpenDaVINCI header.
                std::array<char, 5> header;
                header[0] = 0x0D;
                header[1] = 0xA4;
                header[2] = *(reinterpret_cast<char*>(&length)+0);
                header[3] = *(reinterpret_cast<char*>(&length)+1);
                header[4] = *(reinterpret_cast<char*>(&length)+2);

                std::stringstream sstr;
                sstr.write(header.data(), header.size());
                sstr.write(tmp.data(), tmp.size());
                dataToSend = sstr.str();
            }
            toOD4Sender.send(std::move(dataToSend));
        };

        // Interface to OxTS.
        const std::string OXTS_ADDRESS(argv[1]);
        const std::string OXTS_PORT(argv[2]);
        OxTSDecoder oxtsDecoder;
        cluon::UDPReceiver fromOXTS(OXTS_ADDRESS, std::stoi(OXTS_PORT),
            [&toOD4 = sendToOpenDaVINCI, &decoder=oxtsDecoder](std::string &&d, std::string &&/*from*/, std::chrono::system_clock::time_point &&tp) noexcept {
            auto retVal = decoder.decode(d);
            if (retVal.first) {
                const std::chrono::system_clock::time_point timeStamp{tp};

                opendlv::proxy::GeodeticWgs84Reading msg1 = retVal.second.first;
                opendlv::proxy::GeodeticHeadingReading msg2 = retVal.second.second;

                cluon::ToProtoVisitor protoEncoder;
                msg1.accept(protoEncoder);
                std::string payloadGPS{protoEncoder.encodedData()};
                toOD4(msg1.ID(), std::move(payloadGPS), timeStamp);

                msg2.accept(protoEncoder);
                std::string payloadHeading{protoEncoder.encodedData()};
                toOD4(msg2.ID(), std::move(payloadHeading), timeStamp);

                {
                    std::stringstream buffer;
                    msg1.accept([](uint32_t, const std::string &, const std::string &) {},
                               [&buffer](uint32_t, std::string &&, std::string &&n, auto v) { buffer << n << " = " << v << '\n'; },
                               []() {});
                    std::cout << buffer.str() << std::endl;

                    std::stringstream buffer2;
                    msg2.accept([](uint32_t, const std::string &, const std::string &) {},
                               [&buffer2](uint32_t, std::string &&, std::string &&n, auto v) { buffer2 << n << " = " << v << '\n'; },
                               []() {});
                    std::cout << buffer2.str() << std::endl;
                }
            }
        });

        // Just sleep as this microservice is data driven.
        using namespace std::literals::chrono_literals;
        while (fromOXTS.isRunning()) {
            std::this_thread::sleep_for(1s);
        }
    }
    return retCode;
}
