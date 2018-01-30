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

#include "oxts_decoder.hpp"

#include <cmath>
#include <cstring>
#include <array>
#include <sstream>
#include <string>

std::pair<bool, std::pair<opendlv::proxy::GeodeticWgs84Reading, opendlv::proxy::GeodeticHeadingReading> >
    OxTSDecoder::decode(const std::string &data) noexcept {
    bool retVal{false};
    opendlv::proxy::GeodeticWgs84Reading gps;
    opendlv::proxy::GeodeticHeadingReading heading;

    constexpr std::size_t OXTS_PACKET_LENGTH{72};
    constexpr uint8_t OXTS_FIRST_BYTE{0xE7};
    if ( (OXTS_PACKET_LENGTH == data.size()) && (OXTS_FIRST_BYTE == static_cast<uint8_t>(data.at(0))) ) {
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
        }
        retVal = true;
    }
    return std::make_pair(retVal, std::make_pair(gps, heading));
}
