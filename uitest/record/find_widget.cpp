/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "find_widget.h"
using namespace std;
namespace OHOS::uitest {
    const Widget FindWidget(UiDriver &driver, float x, float y)
    {
        ApiCallErr err(NO_ERROR);
        auto selector = WidgetSelector();
        vector<std::unique_ptr<Widget>> rev;
        driver.FindWidgets(selector, rev, err, true);
        auto tree = driver.GetWidgetTree();
        std::map<float, Widget> recv;
        auto matcher = WidgetMatcherByCoord(x, y);
        auto visitor = WidgetCollector(matcher, recv);
        tree->DfsTraverse(visitor);
        return visitor.GetMaxDepWidget();
    }
    std::string WidgetMatcherByCoord::Describe() const
    {
        return "Match widget by coordinates point";
    }
    bool WidgetMatcherByCoord::Matches(const Widget &widget) const
    {
        Rect rect = widget.GetBounds();
        if (x_ <= rect.right_ && x_ >= rect.left_ && y_ <= rect.bottom_ && y_ >= rect.top_) {
            return true;
        } else {
            return false;
        }
    }

    int32_t WidgetCollector::GetDept(const Widget &widget) const
    {
        return widget.GetHierarchy().length();
    }

    void WidgetCollector::Visit(const Widget &widget)
    {
        if (matcher_.Matches(widget)) {
            int32_t dept = GetDept(widget);
            if (receiver_.size() == 0) {
                maxDep = dept;
            } else {
                maxDep = max(maxDep, dept);
            }
            receiver_.insert(std::make_pair(dept, widget));
        }
    }
}