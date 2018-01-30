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

#include "cluon/cluon.hpp"
#include "cluon/cluonDataStructures.hpp"
#include "cluon/EnvelopeToJSON.hpp"
#include "cluon/ToProtoVisitor.hpp"
#include "cluon/UDPSender.hpp"
#include "cluon/UDPReceiver.hpp"

#include "messages.hpp"

#include <cmath>
#include <cstring>
#include <array>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

int main(int argc, char **argv) {
    int retVal{0};
    const std::string PROGRAM(argv[0]);
    if (4 != argc) {
        std::cerr << PROGRAM << " decodes latitude/longitude/heading from an OXTS GPS/INSS unit and publishes it to a running OpenDaVINCI session using the OpenDLV Standard Message Set." << std::endl;
        std::cerr << "Usage:   " << PROGRAM << " <IPv4-address> <port> <OpenDaVINCI session>" << std::endl;
        std::cerr << "Example: " << PROGRAM << " 192.168.1.255 3000 111" << std::endl;
        retVal = 1;
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

//    const char *messageSpecification = R"(
//message opendlv.proxy.GeodeticWgs84Reading [id = 19] {
//  double latitude [id = 1];
//  double longitude [id = 3];
//}

//message opendlv.proxy.GeodeticHeadingReading [id = 1037] {
//  float northHeading [id = 1];
//})";

//cluon::EnvelopeToJSON env2JSON;
//env2JSON.setMessageSpecification(std::string(messageSpecification));
//const std::string JSON_A = env2JSON.getJSONFromEnvelope(envelope);
//std::cout << JSON_A << std::endl;

            toOD4Sender.send(std::move(dataToSend));
        };

        // Interface to OxTS.
        const std::string OXTS_ADDRESS(argv[1]);
        const std::string OXTS_PORT(argv[2]);
        cluon::UDPReceiver fromOXTS(OXTS_ADDRESS, std::stoi(OXTS_PORT),
            [&toOD4 = sendToOpenDaVINCI](std::string &&d, std::string &&/*from*/, std::chrono::system_clock::time_point &&tp) noexcept {
                constexpr std::size_t OXTS_PACKET_LENGTH{72};
                constexpr uint8_t OXTS_FIRST_BYTE{0xE7};
                std::string data{d};

// Some test data.
//std::vector<uint8_t> packet_bytes{
//  0xe7, 0x9c, 0x95, 0x95, 0x08, 0x00, 0x7c, 0x0e,
//  0x00, 0x06, 0x81, 0xfe, 0x45, 0x00, 0x00, 0xf4,
//  0x00, 0x00, 0xaa, 0xff, 0xff, 0x04, 0xc2, 0x92,
//  0xf2, 0x9e, 0x60, 0x0a, 0x35, 0xf0, 0x3f, 0x46,
//  0x63, 0x83, 0x3b, 0x7c, 0x96, 0xcc, 0x3f, 0x23,
//  0x5a, 0xd0, 0x42, 0x32, 0x00, 0x00, 0x05, 0x00,
//  0x00, 0x2c, 0x00, 0x00, 0xeb, 0xae, 0xe0, 0x00,
//  0x59, 0x00, 0xbe, 0x6b, 0xff, 0xe4, 0x1d, 0x01,
//  0x00, 0x00, 0x00, 0xff, 0xff, 0x01, 0xff, 0xe4
//};
//{"dataType":19,
//"sent":{"seconds":1517263024,
//"microseconds":225405},
//"received":{"seconds":0,
//"microseconds":0},
//"sampleTimeStamp":{"seconds":1517263024,
//"microseconds":225405},
//"senderStamp":0,
//"opendlv_proxy_GeodeticWgs84Reading":{"latitude":58.037722605,
//"longitude":12.796579564}}
//{"dataType":1037,
//"sent":{"seconds":1517263024,
//"microseconds":225405},
//"received":{"seconds":0,
//"microseconds":0},
//"sampleTimeStamp":{"seconds":1517263024,
//"microseconds":225405},
//"senderStamp":0,
//"opendlv_proxy_GeodeticHeadingReading":{"northHeading":11.46342}}
//                std::string data(reinterpret_cast<char*>(packet_bytes.data()), packet_bytes.size());

                std::clog << "Received packet of length " << data.size() << std::endl;

                if ( (OXTS_PACKET_LENGTH == data.size()) && (OXTS_FIRST_BYTE == static_cast<uint8_t>(data.at(0))) ) {
                    const std::chrono::system_clock::time_point timeStamp{tp};
                    cluon::ToProtoVisitor protoEncoder;
                    std::stringstream buffer{data};

                    // Decode latitude/longitude.
                    {
                        double latitude{0.0};
                        double longitude{0.0};

                        // Move to where latitude/longitude are encoded.
                        constexpr uint32_t START_OF_LAT_LON{23};
                        buffer.seekg(START_OF_LAT_LON);
                        buffer.read(reinterpret_cast<char*>(&latitude), sizeof(double));
                        buffer.read(reinterpret_cast<char*>(&longitude), sizeof(double));

                        opendlv::proxy::GeodeticWgs84Reading gps;
                        gps.latitude(latitude/M_PI*180.0).longitude(longitude/M_PI*180.0);

                        gps.accept(protoEncoder);
                        std::string payload{protoEncoder.encodedData()};
                        toOD4(opendlv::proxy::GeodeticWgs84Reading::ID(), std::move(payload), timeStamp);
                    }

                    // Decode heading.
                    {
                        float northHeading{0.0f};

                        // Move to where heading is encoded.
                        constexpr uint32_t START_OF_HEADING{51};
                        buffer.seekg(START_OF_HEADING);

                        // Extract only three bytes from OxTS.
                        std::array<char, 4> tmp{0, 0, 0, 0};
                        buffer.read(tmp.data(), 3);
                        uint32_t data{0};
                        std::memcpy(&data, tmp.data(), 4);
                        data = le32toh(data);
                        northHeading = data * 1e-6;

                        opendlv::proxy::GeodeticHeadingReading heading;
                        heading.northHeading(northHeading);

                        heading.accept(protoEncoder);
                        std::string payload{protoEncoder.encodedData()};
                        toOD4(opendlv::proxy::GeodeticHeadingReading::ID(), std::move(payload), timeStamp);
                    }
                }
            });

        // Just sleep as this microservice is data driven.
        using namespace std::literals::chrono_literals;
        while (fromOXTS.isRunning()) {
            std::this_thread::sleep_for(1s);
        }
    }
    return retVal;
}
