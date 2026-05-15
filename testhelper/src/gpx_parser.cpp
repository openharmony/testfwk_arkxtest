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

#include <iostream>
#include <cmath>
#include <cstring>
#include <sys/stat.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "gpx_parser.h"
#include "common_utils.h"

namespace OHOS::testhelper {
    constexpr double DEFAULT_ALTITUDE = 0.0;
    constexpr double DEFAULT_ACCURACY = 10.0;
    constexpr double DEFAULT_SPEED = 0.0;
    constexpr double DEFAULT_DIRECTION = 0.0;
    constexpr double INVALID_LATITUDE = 999.0;
    constexpr double INVALID_LONGITUDE = 999.0;
    constexpr double LATITUDE_MIN = -90.0;
    constexpr double LATITUDE_MAX = 90.0;
    constexpr double LONGITUDE_MIN = -180.0;
    constexpr double LONGITUDE_MAX = 180.0;
    constexpr double DIRECTION_MIN = 0.0;
    constexpr double DIRECTION_MAX = 360.0;

    bool SafeStod(const std::string& str, double& value)
    {
        if (str.empty()) {
            return false;
        }
        char* end = nullptr;
        value = std::strtod(str.c_str(), &end);
        if (end != str.c_str() && *end == '\0') {
            if (std::isnan(value)) {
                return false;
            }
            return true;
        }
        return false;
    }
    
    LocationList GPXParser::ParseFile(const std::string& filePath)
    {
        LocationList locations;
        LOG_I("GPXParser: Processing GPX file %{public}s", filePath.c_str());
        xmlDocPtr doc = xmlReadFile(filePath.c_str(), nullptr, XML_PARSE_NOBLANKS | XML_PARSE_NONET);
        if (doc == nullptr) {
            PrintToConsole("Error: The parameter validation in gpx file has failed. Valid XML format is required.");
            LOG_E("GPXParser: xmlReadFile returned nullptr");
            return locations;
        }
        xmlNodePtr root = xmlDocGetRootElement(doc);
        if (root == nullptr) {
            PrintToConsole("Error: The parameter validation in gpx file has failed. Root element is required.");
            xmlFreeDoc(doc);
            LOG_E("GPXParser: Root element is null");
            return locations;
        }
        if (!ValidateGPXRoot(root, locations)) {
            xmlFreeDoc(doc);
            return locations;
        }
        ProcessGPXChildren(root, locations);
        xmlFreeDoc(doc);
        if (locations.empty()) {
            PrintToConsole(
                "Error: The parameter validation in gpx file has failed. Valid location points are required.");
            LOG_E("GPXParser: No valid location points found");
        } else {
            LOG_I("GPXParser: Successfully parsed %{public}zu points", locations.size());
        }
        return locations;
    }

    bool GPXParser::ValidateGPXRoot(xmlNodePtr root, LocationList& locations)
    {
        const char* rootName = reinterpret_cast<const char*>(root->name);
        LOG_I("GPXParser: Root element: %{public}s", rootName);
        if (xmlStrcasecmp(root->name, BAD_CAST "gpx") != 0) {
            PrintToConsole("Error: The parameter validation in gpx file has failed. <gpx> root element is required.");
            LOG_E("GPXParser: Invalid root element");
            return false;
        }
        return true;
    }

    void GPXParser::ProcessGPXChildren(xmlNodePtr root, LocationList& locations)
    {
        for (xmlNodePtr child = root->children; child != nullptr; child = child->next) {
            if (child->type != XML_ELEMENT_NODE) continue;
            if (child->name == nullptr) continue;
            const char* childName = reinterpret_cast<const char*>(child->name);
            ProcessChildElement(child, childName, locations);
        }
    }

    void GPXParser::ProcessChildElement(xmlNodePtr child, const char* childName, LocationList& locations)
    {
        if (child->name == nullptr) return;
        if (xmlStrcasecmp(child->name, BAD_CAST "wpt") == 0) {
            LOG_D("GPXParser: Found waypoint element");
            ProcessWaypoint(child, locations);
        } else if (xmlStrcasecmp(child->name, BAD_CAST "trk") == 0) {
            LOG_D("GPXParser: Found track element");
            ProcessTrack(child, locations);
        } else if (xmlStrcasecmp(child->name, BAD_CAST "rte") == 0) {
            LOG_D("GPXParser: Found route element");
            ProcessRoute(child, locations);
        }
    }

    int32_t GPXParser::CreateLocationFromData(const GPXElementData& data,
        testserver::TestServerLocation& location)
    {
        double lat = INVALID_LATITUDE;
        double lon = INVALID_LONGITUDE;
        double ele = DEFAULT_ALTITUDE;
        double speed = DEFAULT_SPEED;
        double direction = DEFAULT_DIRECTION;
        if (!data.lat.empty() && !SafeStod(data.lat, lat)) {
            LOG_E("GPXParser: Failed to parse latitude");
            return PARSE_GPX_INVALID_LAT;
        }
        if (!data.lon.empty() && !SafeStod(data.lon, lon)) {
            LOG_E("GPXParser: Failed to parse longitude");
            return PARSE_GPX_INVALID_LON;
        }
        if (lat < LATITUDE_MIN || lat > LATITUDE_MAX) {
            LOG_E("GPXParser: Invalid latitude");
            return PARSE_GPX_INVALID_COORDS;
        }
        if (lon < LONGITUDE_MIN || lon > LONGITUDE_MAX) {
            LOG_E("GPXParser: Invalid longitude");
            return PARSE_GPX_INVALID_COORDS;
        }
        if (!data.ele.empty() && !SafeStod(data.ele, ele)) {
            ele = DEFAULT_ALTITUDE;
        }
        if (!data.speed.empty() && !SafeStod(data.speed, speed)) {
            speed = DEFAULT_SPEED;
        } else if (!data.speed.empty() && speed < 0) {
            speed = DEFAULT_SPEED;
        }
        if (!data.direction.empty() && SafeStod(data.direction, direction)) {
            if (direction < DIRECTION_MIN || direction > DIRECTION_MAX) {
                direction = DEFAULT_DIRECTION;
            }
        } else if (!data.direction.empty()) {
            direction = DEFAULT_DIRECTION;
        }
        location.latitude = lat;
        location.longitude = lon;
        location.altitude = ele;
        location.accuracy = DEFAULT_ACCURACY;
        location.speed = speed;
        location.direction = direction;
        location.timeStamp = 0;
        location.timeSinceBoot = 0;
        location.isFromMock = true;
        return PARSE_GPX_SUCCESS;
    }

    void GPXParser::ParseExtensions(xmlNodePtr node, std::string& speedStr, std::string& directionStr)
    {
        if (node == nullptr) return;
        for (xmlNodePtr child = node->children; child != nullptr; child = child->next) {
            if (child->type != XML_ELEMENT_NODE) continue;
            if (child->name == nullptr) continue;
            const char* childName = reinterpret_cast<const char*>(child->name);
            if (strcasestr(childName, "speed") != nullptr) {
                xmlChar* content = xmlNodeGetContent(child);
                if (content != nullptr) {
                    speedStr = reinterpret_cast<char*>(content);
                    xmlFree(content);
                }
            }
            if (strcasestr(childName, "course") != nullptr || strcasestr(childName, "direction") != nullptr) {
                xmlChar* content = xmlNodeGetContent(child);
                if (content != nullptr) {
                    directionStr = reinterpret_cast<char*>(content);
                    xmlFree(content);
                }
            }
        }
    }

    void GPXParser::ParsePointData(xmlNodePtr node, GPXElementData& data, std::string& speedStr,
        std::string& directionStr)
    {
        if (node == nullptr) return;
        for (xmlNodePtr child = node->children; child != nullptr; child = child->next) {
            if (child->type != XML_ELEMENT_NODE) continue;
            if (child->name == nullptr) continue;
            const char* childName = reinterpret_cast<const char*>(child->name);
            bool isExtensionsContainer = xmlStrcasecmp(child->name, BAD_CAST "extensions") == 0 ||
                strcasestr(childName, "extensions") != nullptr;
            if (isExtensionsContainer) {
                ParseExtensions(child, speedStr, directionStr);
                continue;
            }
            if (xmlStrcasecmp(child->name, BAD_CAST "ele") == 0) {
                xmlChar* content = xmlNodeGetContent(child);
                if (content != nullptr) {
                    data.ele = reinterpret_cast<char*>(content);
                    xmlFree(content);
                }
            } else if (xmlStrcasecmp(child->name, BAD_CAST "time") == 0) {
                xmlChar* content = xmlNodeGetContent(child);
                if (content != nullptr) {
                    data.time = reinterpret_cast<char*>(content);
                    xmlFree(content);
                }
            } else if (strcasestr(childName, "speed") != nullptr && speedStr.empty()) {
                xmlChar* content = xmlNodeGetContent(child);
                if (content != nullptr) {
                    speedStr = reinterpret_cast<char*>(content);
                    xmlFree(content);
                }
            } else if ((strcasestr(childName, "course") != nullptr || strcasestr(childName, "direction") != nullptr) &&
                directionStr.empty()) {
                xmlChar* content = xmlNodeGetContent(child);
                if (content != nullptr) {
                    directionStr = reinterpret_cast<char*>(content);
                    xmlFree(content);
                }
            }
        }
    }

    void GPXParser::AddLocation(const GPXElementData& data, const std::string& speedStr,
        const std::string& directionStr, LocationList& locations, const std::string& pointType)
    {
        GPXElementData locationData = data;
        if (!speedStr.empty()) locationData.speed = speedStr;
        if (!directionStr.empty()) locationData.direction = directionStr;
        testserver::TestServerLocation location;
        if (CreateLocationFromData(locationData, location) == PARSE_GPX_SUCCESS) {
            locations.push_back(location);
            LOG_I("GPXParser: Added valid %{public}s point", pointType.c_str());
        }
    }

    void GPXParser::ProcessPoint(xmlNodePtr node, LocationList& locations, const std::string& pointType)
    {
        if (node == nullptr || node->properties == nullptr) {
            std::string msg = "GPXParser: ProcessGPX" + pointType + ": Invalid node";
            LOG_W("%{public}s", msg.c_str());
            return;
        }
        xmlChar* latAttr = xmlGetProp(node, BAD_CAST "lat");
        xmlChar* lonAttr = xmlGetProp(node, BAD_CAST "lon");
        if (latAttr == nullptr || lonAttr == nullptr) {
            if (latAttr) xmlFree(latAttr);
            if (lonAttr) xmlFree(lonAttr);
            LOG_W("GPXParser: Missing lat or lon attribute");
            return;
        }
        GPXElementData data;
        data.lat = reinterpret_cast<char*>(latAttr);
        data.lon = reinterpret_cast<char*>(lonAttr);
        xmlFree(latAttr);
        xmlFree(lonAttr);
        std::string speedStr;
        std::string directionStr;
        ParsePointData(node, data, speedStr, directionStr);
        AddLocation(data, speedStr, directionStr, locations, pointType);
    }

    void GPXParser::ProcessWaypoint(xmlNodePtr node, LocationList& locations)
    {
        ProcessPoint(node, locations, "waypoint");
    }

    void GPXParser::ProcessTrackSegment(xmlNodePtr trksegNode, LocationList& locations)
    {
        if (trksegNode == nullptr) {
            LOG_W("GPXParser: ProcessGPXTrackSegment: Invalid node");
            return;
        }
        for (xmlNodePtr child = trksegNode->children; child != nullptr; child = child->next) {
            if (child->type != XML_ELEMENT_NODE) continue;
            if (child->name == nullptr) continue;
            if (xmlStrcasecmp(child->name, BAD_CAST "trkpt") == 0) {
                ProcessPoint(child, locations, "track");
            }
        }
    }

    void GPXParser::ProcessTrack(xmlNodePtr trkNode, LocationList& locations)
    {
        if (trkNode == nullptr) {
            LOG_W("GPXParser: ProcessGPXTrack: Invalid node");
            return;
        }
        for (xmlNodePtr child = trkNode->children; child != nullptr; child = child->next) {
            if (child->type != XML_ELEMENT_NODE) continue;
            if (child->name == nullptr) continue;
            if (xmlStrcasecmp(child->name, BAD_CAST "trkseg") == 0) {
                ProcessTrackSegment(child, locations);
            }
        }
    }

    void GPXParser::ProcessRoute(xmlNodePtr rteNode, LocationList& locations)
    {
        if (rteNode == nullptr) {
            LOG_W("GPXParser: ProcessGPXRoute: Invalid node");
            return;
        }
        for (xmlNodePtr child = rteNode->children; child != nullptr; child = child->next) {
            if (child->type != XML_ELEMENT_NODE) continue;
            if (child->name == nullptr) continue;
            if (xmlStrcasecmp(child->name, BAD_CAST "rtept") == 0) {
                ProcessPoint(child, locations, "route");
            }
        }
    }
}
