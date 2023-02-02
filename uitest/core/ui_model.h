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
    /**Enumerates the supported UiComponent attributes.*/
    enum UiAttr : uint8_t {
        ACCESSIBILITY_ID,
        ID,
        TEXT,
        KEY,
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
        // inner used attributes
        HIERARCHY,
        HASHCODE,
        BOUNDSCENTER,
        HOST_WINDOW_ID,
    };

    /**Supported UiComponent attribute names. Ordered by <code>UiAttr</code> definition.*/
    constexpr std::string_view ATTR_NAMES[] = {
        "accessibilityId", // ACCESSIBILITY_ID
        "id",            // ID
        "text",          // TEXT
        "key",           // KEY
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
        "hierarchy",     // HIERARCHY
        "hashcode",      // HASHCODE
        "boundsCenter",  // BOUNDSCENTER
        "hostWindowId",  // HOST_WINDOW_ID
    };

    struct Point {
        Point() : px_(0), py_(0) {};
        Point(int32_t px, int32_t py) : px_(px), py_(py) {};
        int32_t px_;
        int32_t py_;
    };

    /**Represents a reasonable rectangle area.*/
    struct Rect {
        Rect(int32_t left, int32_t right, int32_t top, int32_t bottom)
            : left_(left), right_(right), top_(top), bottom_(bottom)
        {
            DCHECK(right_ >= left_ && bottom_ >= top_);
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
            attributes_.insert(std::make_pair(ATTR_NAMES[UiAttr::HIERARCHY], hierarchy));
        };

        ~Widget() override {}

        const FrontEndClassDef &GetFrontendClassDef() const override
        {
            return COMPONENT_DEF;
        }

        bool HasAttr(std::string_view name) const;

        std::string GetAttr(std::string_view name, std::string_view defaultVal) const;

        Rect GetBounds() const
        {
            return bounds_;
        }

        std::string GetHierarchy() const
        {
            return hierarchy_;
        }

        void SetAttr(std::string_view name, std::string_view value);

        void SetHostTreeId(std::string_view tid);

        std::string GetHostTreeId() const;

        void SetBounds(const Rect &bounds);

        std::string ToStr() const;

        std::unique_ptr<Widget> Clone(std::string_view hostTreeId, std::string_view hierarchy) const;

        std::map<std::string, std::string> GetAttrMap() const;

    private:
        const std::string hierarchy_;
        std::string hostTreeId_;
        std::map<std::string, std::string> attributes_;
        Rect bounds_ = {0, 0, 0, 0};
    };

    // ensure Widget is movable, since we need to move a constructed Widget object into WidgetTree
    static_assert(std::is_move_constructible<Widget>::value, "Widget need to be movable");

    class WidgetVisitor {
    public:
        virtual void Visit(const Widget &widget) = 0;
    };

    class WidgetTree {
    public:
        WidgetTree() = delete;

        ~WidgetTree() {}

        explicit WidgetTree(std::string name) : name_(std::move(name)), identifier_(GenerateTreeId()) {}

        /**
         * Construct tree nodes from the given dom data.
         *
         * @param dom: the dom json data.
         * @param amendBounds: if or not amend widget bounds and visibility.
         * */
        void ConstructFromDom(const nlohmann::json &dom, bool amendBounds);

        /**
         * Marshal tree nodes hierarchy into the given dom data.
         *
         * @param dom: the dom json data.
         * */
        void MarshalIntoDom(nlohmann::json &dom) const;

        void DfsTraverse(WidgetVisitor &visitor) const;

        void DfsTraverseFronts(WidgetVisitor &visitor, const Widget &pivot) const;

        void DfsTraverseRears(WidgetVisitor &visitor, const Widget &pivot) const;

        void DfsTraverseDescendants(WidgetVisitor &visitor, const Widget &root) const;

        /**
         * Get the root widget node.
         *
         * @return the root widget node, or <code>nullptr</code> if the tree is empty or not constructed.
         * */
        const Widget *GetRootWidget() const;

        /**
         * Get the parent widget node of the given widget
         *
         * @param widget: the child widget.
         * @returns the parent widget pointer, <code>nullptr</code> if the given node it's the root node.
         * */
        const Widget *GetParentWidget(const Widget &widget) const;

        /**
         * Get the child widget node of the given widget at the given index.
         *
         * @param widget: the parent widget.
         * @param index: the child widget index, <b>starts from 0</b>.
         * @returns the child widget pointer, or <code>nullptr</code> if there's no such child.
         * */
        const Widget *GetChildWidget(const Widget &widget, uint32_t index) const;

        /**Check if the given widget node hierarchy is the root node hierarchy.*/
        static bool IsRootWidgetHierarchy(std::string_view hierarchy);

        /**
         * Merge several tree into one tree.
         *
         * @param from: the subtrees to merge, should be sorted by z-order and the top one be at first.
         * @param to: the root tree to merge into.
         * */
        static void MergeTrees(const std::vector<std::unique_ptr<WidgetTree>> &from, WidgetTree &to);

    private:
        const std::string name_;
        const std::string identifier_;
        bool widgetsConstructed_ = false;
        // widget-hierarchy VS widget-ptr
        std::map<std::string, Widget> widgetMap_;
        // widget-hierarchies, dfs order
        std::vector<std::string> widgetHierarchyIdDfsOrder_;

        /**
         * Get WidgetTree by hierarchy,return <code>nullptr</code> if no such widget exist.
         * */
        const Widget *GetWidgetByHierarchy(std::string_view hierarchy) const;

        /**Check if the given widget is in this tree.*/
        inline bool CheckIsMyNode(const Widget &widget) const;

        /**Generated an unique tree-identifier.*/
        static std::string GenerateTreeId();
    };

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
        std::string bundleName_ = "";
        std::string title_ = "";
        bool focused_ = false;
        bool actived_ = false;
        bool decoratorEnabled_ = false;
        Rect bounds_ = {0, 0, 0, 0};
        WindowMode mode_ = UNKNOWN;
    };
} // namespace OHOS::uitest

#endif