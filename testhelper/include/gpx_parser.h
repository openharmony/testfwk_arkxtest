/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GPX_PARSER_H
#define GPX_PARSER_H

#include <string>
#include <vector>
#include <libxml/tree.h>
#include "test_server_error_code.h"
#include "test_server_client.h"

namespace OHOS::testhelper {
    struct GPXElementData {
        std::string lat;
        std::string lon;
        std::string ele;
        std::string time;
        std::string speed;
        std::string direction;
    };

    class GPXParser {
    public:
        static std::vector<testserver::TestServerLocation> ParseFile(const std::string& filePath);

    private:
        static int32_t CreateLocationFromData(const GPXElementData& data, testserver::TestServerLocation& location);
        static void ParseExtensions(xmlNodePtr node, std::string& speedStr, std::string& directionStr);
        static void ParsePointData(xmlNodePtr node, GPXElementData& data, std::string& speedStr,
            std::string& directionStr);
        static void AddLocation(const GPXElementData& data, const std::string& speedStr,
            const std::string& directionStr, std::vector<testserver::TestServerLocation>& locations,
            const std::string& pointType);
        static void ProcessPoint(xmlNodePtr node, std::vector<testserver::TestServerLocation>& locations,
            const std::string& pointType);
        static void ProcessWaypoint(xmlNodePtr node, std::vector<testserver::TestServerLocation>& locations);
        static void ProcessTrackSegment(xmlNodePtr trksegNode, std::vector<testserver::TestServerLocation>& locations);
        static void ProcessTrack(xmlNodePtr trkNode, std::vector<testserver::TestServerLocation>& locations);
        static void ProcessRoute(xmlNodePtr rteNode, std::vector<testserver::TestServerLocation>& locations);
        static bool ValidateGPXRoot(xmlNodePtr root, std::vector<testserver::TestServerLocation>& locations);
        static void ProcessGPXChildren(xmlNodePtr root, std::vector<testserver::TestServerLocation>& locations);
        static void ProcessChildElement(xmlNodePtr child, const char* childName,
            std::vector<testserver::TestServerLocation>& locations);
    };

    using LocationList = std::vector<testserver::TestServerLocation>;
}

#endif // GPX_PARSER_H
