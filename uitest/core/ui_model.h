/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef UI_MODEL_H
#define UI_MODEL_H

#include <vector>
#include <sstream>
#include "common_utilities_hpp.h"
#include "frontend_api_handler.h"
#include "json.hpp"

namespace OHOS::uitest {
    using namespace std;
    using namespace nlohmann;
    const std::string ROOT_HIERARCHY = "ROOT";
    /**Enumerates the supported string value match rules.*/
    enum ValueMatchPattern : uint8_t { EQ, CONTAINS, STARTS_WITH, ENDS_WITH, REG_EXP, REG_EXP_ICASE };

    enum AamsWorkMode : uint8_t { NORMAL, FASTGETNODE, END };

    /**Enumerates the supported UiComponent attributes.*/
    enum UiAttr : uint8_t {
        ACCESSIBILITY_ID,
        ID,
        TEXT,
        KEY,
        DESCRIPTION,
        TYPE,
        BOUNDS,
        ENABLED,
        FOCUSED,
        SELECTED,
        CLICKABLE,
        LONG_CLICKABLE,
        SCROLLABLE,
        CHECKABLE,
        CHECKED,
        HOST_WINDOW_ID,
        ORIGBOUNDS,
        ZINDEX,
        OPACITY,
        BACKGROUNDCOLOR,
        BACKGROUNDIMAGE,
        BLUR,
        HITTESTBEHAVIOR,
        CLIP,
        // inner used attributes
        HIERARCHY,
        HASHCODE,
        BOUNDSCENTER,
        VISIBLE,
        ABILITYNAME,
        BUNDLENAME,
        PAGEPATH,
        DUMMY_ATTRNAME_SELECTION,
        HINT,
        MAX, // mark the max length
    };

    /**Supported UiComponent attribute names. Ordered by <code>UiAttr</code> definition.*/
    constexpr std::string_view ATTR_NAMES[] = {
        "accessibilityId", // ACCESSIBILITY_ID
        "id",            // ID
        "text",          // TEXT
        "key",           // KEY
        "description",   // DESCRIPTION
        "type",          // TYPE
        "bounds",        // BOUNDS
        "enabled",       // ENABLED
        "focused",       // FOCUSED
        "selected",      // SELECTED
        "clickable",     // CLICKABLE
        "longClickable", // LONG_CLICKABLE
        "scrollable",    // SCROLLABLE
        "checkable",     // CHECKABLE
        "checked",       // CHECKED
        "hostWindowId",  // HOST_WINDOW_ID
        "origBounds",    // ORIGBOUNDS
        "zIndex",        // ZINDEX
        "opacity",       //OPACITY
        "backgroundColor",   // BACKGROUNDCOLOR
        "backgroundImage",   // BACKGROUNDIMAGE
        "blur",          //BLUR
        "hitTestBehavior",   // HITTESTBEHAVIOR
        "clip", // CLIP
        "hierarchy",     // HIERARCHY
        "hashcode",      // HASHCODE
        "boundsCenter",  // BOUNDSCENTER
        "visible",       // VISIBLE
        "abilityName",   // ABILITYNAME
        "bundleName",    // BUNDLENAME
        "pagePath",      // PAGEPATH
        "dummyAttrnameSelection", // DUMMY_ATTRNAME_SELECTION
        "hint", // HINT
    };

    struct Point {
        Point() : px_(0), py_(0){};
        Point(int32_t px, int32_t py) : px_(px), py_(py){};
        int32_t px_;
        int32_t py_;
    };

    /**Represents a reasonable rectangle area.*/
    struct Rect {
        Rect(int32_t left, int32_t right, int32_t top, int32_t bottom)
            : left_(left), right_(right), top_(top), bottom_(bottom) {};
        Rect()
        {
            left_ = 0;
            right_ = 0;
            top_ = 0;
            bottom_ = 0;
        };
        int32_t left_;
        int32_t right_;
        int32_t top_;
        int32_t bottom_;

        FORCE_INLINE int32_t GetCenterX() const
        {
            return (left_ + right_) / TWO;
        }

        FORCE_INLINE int32_t GetCenterY() const
        {
            return (top_ + bottom_) / TWO;
        }

        FORCE_INLINE int32_t GetWidth() const
        {
            return right_ - left_;
        }

        FORCE_INLINE int32_t GetHeight() const
        {
            return bottom_ - top_;
        }

        std::string Describe() const
        {
            std::stringstream ss;
            ss << "Rect {left:" << left_ << ",right:" << right_ << ",top:" << top_ << ",bottom:" << bottom_ << "}";
            return ss.str();
        }
    };

    struct WidgetMatchModel {
        WidgetMatchModel(UiAttr name, string value, ValueMatchPattern matchPattern)
            : attrName(name), attrValue(value), pattern(matchPattern)
        {
        }
        WidgetMatchModel() {}
        std::string Describe() const
        {
            stringstream ss;
            ss << "{";
            ss << "attrName:" << ATTR_NAMES[attrName].data() << "; value:" << attrValue
               << "; Pattern:" << to_string(pattern);
            ss << "}";
            return ss.str();
        }
        UiAttr attrName;
        string attrValue;
        ValueMatchPattern pattern;
    };

    /**Algorithm of rectangle.*/
    class RectAlgorithm {
    public:
        FORCE_INLINE static bool CheckEqual(const Rect &ra, const Rect &rb);
        FORCE_INLINE static bool CheckIntersectant(const Rect &ra, const Rect &rb);
        FORCE_INLINE static bool IsInnerPoint(const Rect &rect, const Point &point);
        FORCE_INLINE static bool IsPointOnEdge(const Rect &rect, const Point &point);
        static bool ComputeIntersection(const Rect &ra, const Rect &rb, Rect &result);
        static bool ComputeMaxVisibleRegion(const Rect &rect, const std::vector<Rect> &overlays, Rect &out);
    };

    FORCE_INLINE bool RectAlgorithm::CheckEqual(const Rect &ra, const Rect &rb)
    {
        return ra.left_ == rb.left_ && ra.right_ == rb.right_ && ra.top_ == rb.top_ && ra.bottom_ == rb.bottom_;
    }

    FORCE_INLINE bool RectAlgorithm::CheckIntersectant(const Rect &ra, const Rect &rb)
    {
        if (ra.left_ >= rb.right_ || ra.right_ <= rb.left_) {
            return false;
        }
        if (ra.top_ >= rb.bottom_ || ra.bottom_ <= rb.top_) {
            return false;
        }
        return true;
    }

    FORCE_INLINE bool RectAlgorithm::IsInnerPoint(const Rect &rect, const Point& point)
    {
        if (point.px_ <= rect.left_ || point.px_ >= rect.right_
            || point.py_ <= rect.top_ || point.py_ >= rect.bottom_) {
            return false;
        }
        return true;
    }

    FORCE_INLINE bool RectAlgorithm::IsPointOnEdge(const Rect &rect, const Point& point)
    {
        if ((point.px_ == rect.left_ || point.px_ == rect.right_)
            && point.py_ >= rect.top_ && point.py_ <= rect.bottom_) {
            return true;
        }
        if ((point.py_ == rect.top_ || point.py_ == rect.bottom_)
            && point.px_ >= rect.left_ && point.px_ <= rect.right_) {
            return true;
        }
        return false;
    }

    class Widget : public BackendClass {
    public:
        // disable default constructor, copy constructor and assignment operator
        explicit Widget(std::string_view hierarchy) : hierarchy_(hierarchy)
        {
            attributeVec_[UiAttr::HIERARCHY] = hierarchy;
        };

        ~Widget() override {}

        const FrontEndClassDef &GetFrontendClassDef() const override
        {
            return COMPONENT_DEF;
        }

        Rect GetBounds() const
        {
            return bounds_;
        }

        std::string GetHierarchy() const
        {
            return hierarchy_;
        }

        void SetBounds(const Rect &bounds);

        std::string ToStr() const;

        std::unique_ptr<Widget> Clone(std::string_view hierarchy) const;

        std::vector<std::string> GetAttrVec() const;

        bool IsVisible() const
        {
            return GetAttr(UiAttr::VISIBLE) == "true";
        }

        void SetAttr(UiAttr attrId, string value);

        std::string GetAttr(UiAttr attrId) const;

        bool MatchAttr(const WidgetMatchModel& matchModel) const;

        void SetHierarchy(const std::string& hierarch);

        void WrapperWidgetToJson(nlohmann::json& out);
    private:
        std::string hierarchy_;
        std::vector<string> attributeVec_ = std::vector<string>(UiAttr::MAX + 1);
        Rect bounds_ = {0, 0, 0, 0};
    };

    // ensure Widget is movable, since we need to move a constructed Widget object into WidgetTree
    static_assert(std::is_move_constructible<Widget>::value, "Widget need to be movable");

    /**Enumerates the supported UiComponent attributes.*/
    enum WindowMode : uint8_t { UNKNOWN, FULLSCREEN, SPLIT_PRIMARY, SPLIT_SECONDARY, FLOATING, PIP };

    // Represents a UI window on screen.
    class Window : public BackendClass {
    public:
        explicit Window(int32_t id) : id_(id) {};

        ~Window() override {}

        const FrontEndClassDef &GetFrontendClassDef() const override
        {
            return UI_WINDOW_DEF;
        }

        // plain properties, make them public for easy access
        int32_t id_ = 0;
        int32_t windowLayer_ = 0;
        std::string bundleName_ = "";
        std::string title_ = "";
        bool focused_ = false;
        bool actived_ = false;
        bool decoratorEnabled_ = false;
        Rect bounds_ = {0, 0, 0, 0};
        Rect visibleBounds_ = {0, 0, 0, 0};
        std::vector<Rect> invisibleBoundsVec_;
        std::string abilityName_ = "";
        std::string pagePath_ = "";
        WindowMode mode_ = UNKNOWN;
    };

    class DumpHandler {
    public:
        static void AddExtraAttrs(nlohmann::json &root, const map<int32_t, string_view> &elementTrees, size_t index);
        static void DumpWindowInfoToJson(vector<Widget> &allWidget, nlohmann::json &root);
    };
    
    class WidgetHierarchyBuilder {
    public:
        static string Build(string_view parentWidgetHierarchy, uint32_t childIndex);

        static string GetParentWidgetHierarchy(string_view hierarchy);

        static string GetChildHierarchy(string_view hierarchy, uint32_t childIndex);

    private:
        static constexpr auto HIERARCHY_SEPARATOR = ",";
    };

    struct DumpOption {
        string savePath_ = "";
        bool listWindows_ = false;
        bool addExternAttr_ = false;
        string bundleName_ = "";
        string windowId_ = "";
        bool notMergeWindow_ = false;
    };
} // namespace OHOS::uitest

#endif