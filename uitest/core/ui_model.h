/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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

#include <string>
#include <utility>
#include <vector>
#include <map>
#include <memory>
#include <sstream>
#include "common_utilities.hpp"

namespace OHOS::uitest {
    struct Point {
        Point(int32_t px, int32_t py) : px_(px), py_(py) {};
        int32_t px_;
        int32_t py_;
    };

    /**Represents a reasonable rectangle area.*/
    struct Rect {
        Rect(int32_t left, int32_t right, int32_t top, int32_t bottom)
            : left_(left), right_(right), top_(top), bottom_(bottom)
        {
            DCHECK(right_ >= left_ && bottom_ >= top_, "Illegal bounds");
        };

        int32_t left_;
        int32_t right_;
        int32_t top_;
        int32_t bottom_;

        inline int32_t GetCenterX() const
        {
            return (left_ + right_) / 2;
        }

        inline int32_t GetCenterY() const
        {
            return (top_ + bottom_) / 2;
        }

        void ComputeOverlappingDimensions(const Rect &other, int32_t &width, int32_t& height) const;
    };

    class Widget;

    static constexpr auto ATTR_HIERARCHY = "hierarchy";
    static constexpr auto ATTR_HASHCODE = "hashcode";

    class Widget {
    public:
        // disable default constructor, copy constructor and assignment operator
        explicit Widget(std::string_view hierarchy) : hierarchy_(hierarchy)
        {
            attributes_.insert(std::make_pair(ATTR_HIERARCHY, hierarchy));
        };

        // disable copy and assign, to avoid object slicing
        DISALLOW_COPY_AND_ASSIGN(Widget);

        Widget(Widget &&val) noexcept: hierarchy_(val.hierarchy_), hostTreeId_(val.hostTreeId_),
                                       attributes_(val.attributes_), bounds_(val.bounds_) {}

        virtual ~Widget() {}

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

        void SetBounds(int32_t cl, int32_t cr, int32_t ct, int32_t cb);

        std::string ToStr(bool ignoreBounds = false) const;

        void DumpAttributes(std::map<std::string, std::string> &receiver) const;

    private:
        const std::string hierarchy_;
        std::string hostTreeId_;
        std::map <std::string, std::string> attributes_;
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
        DISALLOW_COPY_AND_ASSIGN(WidgetTree);

        ~WidgetTree() {}

        explicit WidgetTree(std::string name) : name_(std::move(name)), identifier_(GenerateTreeId()) {}

        /**
         * Construct tree nodes from the given dom content.
         *
         * @param content: the dom content text.
         * @param errReceiver: the dom parsing failure receiver.
         * @returns true if construction succeed, false otherwise.
         * */
        bool ConstructFromDomText(std::string_view content, std::stringstream &errReceiver);

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

    private:
        const std::string name_;
        const std::string identifier_;
        bool widgetsConstructed_ = false;
        // widget-hierarchy VS widget-ptr
        std::map <std::string, Widget> widgetMap_;
        // widget-hierarchies, dfs order
        std::vector <std::string> widgetHierarchyIdDfsOrder_;

        /**
         * Get WidgetTree by hierarchy,return <code>nullptr</code> if no such widget exist.
         * */
        const Widget *GetWidgetByHierarchy(std::string_view hierarchy) const;

        /**Check if the given widget is in this tree.*/
        inline bool CheckIsMyNode(const Widget &widget) const;

        /**Generated an unique tree-identifier.*/
        static std::string GenerateTreeId();
    };
} // namespace uitest

#endif