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

#include <algorithm>
#include "ui_model.h"

namespace OHOS::uitest {
    using namespace std;

    bool RectAlgorithm::ComputeIntersection(const Rect &ra, const Rect &rb, Rect &result)
    {
        if (ra.left_ >= rb.right_ || ra.right_ <= rb.left_) {
            return false;
        }
        if (ra.top_ >= rb.bottom_ || ra.bottom_ <= rb.top_) {
            return false;
        }
        array<int32_t, INDEX_FOUR> px = {ra.left_, ra.right_, rb.left_, rb.right_};
        array<int32_t, INDEX_FOUR> py = {ra.top_, ra.bottom_, rb.top_, rb.bottom_};
        sort(px.begin(), px.end());
        sort(py.begin(), py.end());
        result = {px[INDEX_ONE], px[INDEX_TWO], py[INDEX_ONE], py[INDEX_TWO]};
        return true;
    }

    /** Use static data buffers instead of dynamic-containers to ensure performance.*/
    static constexpr size_t MAX_OVERLAYS = 64;
    static constexpr size_t MAX_COORDS = (MAX_OVERLAYS + 1) << 1;
    static constexpr size_t MAX_POINTS = MAX_COORDS * MAX_COORDS;
    static int32_t g_XCoords[MAX_COORDS];
    static int32_t g_YCoords[MAX_COORDS];
    static Point g_Points[MAX_POINTS];
    static size_t g_XCount = 0;
    static size_t g_YCount = 0;
    static size_t g_PCount = 0;

    static void CollectGridLineCoords(const Rect& rect, const Rect& overlay)
    {
        bool contained = false;
        if (overlay.left_ >= rect.left_ && overlay.left_ <= rect.right_) {
            for (size_t idx = 0; idx < g_XCount; idx++) {
                if (overlay.left_ == g_XCoords[idx]) {
                    contained = true;
                    break;
                }
            }
            if (!contained) {
                g_XCoords[g_XCount++] = overlay.left_;
            }
        }

        contained = false;
        if (overlay.right_ >= rect.left_ && overlay.right_ <= rect.right_) {
            for (size_t idx = 0; idx < g_XCount; idx++) {
                if (overlay.right_ == g_XCoords[idx]) {
                    contained = true;
                    break;
                }
            }
            if (!contained) {
                g_XCoords[g_XCount++] = overlay.right_;
            }
        }

        contained = false;
        if (overlay.top_ >= rect.top_ && overlay.top_ <= rect.bottom_) {
            for (size_t idx = 0; idx < g_YCount; idx++) {
                if (overlay.top_ == g_YCoords[idx]) {
                    contained = true;
                    break;
                }
            }
            if (!contained) {
                g_YCoords[g_YCount++] = overlay.top_;
            }
        }

        contained = false;
        if (overlay.bottom_ >= rect.top_ && overlay.bottom_ <= rect.bottom_) {
            for (size_t idx = 0; idx < g_YCount; idx++) {
                if (overlay.bottom_ == g_YCoords[idx]) {
                    contained = true;
                    break;
                }
            }
            if (!contained) {
                g_YCoords[g_YCount++] = overlay.bottom_;
            }
        }
    }

    static void CollectGridPoint(int32_t px, int32_t py, const Rect& rect, const vector<Rect>& overlays)
    {
        const auto point = Point(px, py);
        const bool onRectEdge = RectAlgorithm::IsPointOnEdge(rect, point);
        if (!onRectEdge && !RectAlgorithm::IsInnerPoint(rect, point)) {
            return; // point must be on or in the rect
        }
        bool onOverlyEdge = false;
        for (const auto& overlay : overlays) {
            if (RectAlgorithm::IsInnerPoint(overlay, point)) {
                return; // point is in at least one of the overlays, discard
            }
            if (!onRectEdge && !onOverlyEdge) {
                onOverlyEdge = RectAlgorithm::IsPointOnEdge(overlay, point);
            }
        }
        if (onRectEdge || onOverlyEdge) {
            g_Points[g_PCount++] = point;
        }
    }

    static void FindMaxVisibleRegion(const vector<Rect>& overlays, Rect& out)
    {
        // sort grid points and use double-pointer traversing, to resuce time-complexity
        sort(g_Points, g_Points + g_PCount, [](const Point& pa, const Point& pb) {
            return pa.px_ == pb.px_ ? pa.py_ < pb.py_ : pa.px_ < pb.px_;
        });
        out = Rect(0, 0, 0, 0);
        for (size_t idx0 = 0; idx0 < g_PCount; idx0++) {
            for (size_t idx1 = g_PCount - 1; idx1 > idx0; idx1--) {
                const auto width = g_Points[idx1].px_ - g_Points[idx0].px_;
                const auto height = g_Points[idx1].py_ - g_Points[idx0].py_;
                if (width <= 0) {
                    break; // x is strictly ascending ordered
                }
                if (width * height <= out.GetWidth() * out.GetHeight()) {
                    continue; // discard smaller region
                }
                auto intersect = false;
                const auto area = Rect(g_Points[idx0].px_, g_Points[idx1].px_, g_Points[idx0].py_, g_Points[idx1].py_);
                for (auto& overlay : overlays) {
                    if (RectAlgorithm::CheckIntersectant(overlay, area)) {
                        intersect = true; // region is intersected with some overlays, discard
                        break;
                    }
                }
                if (!intersect) {
                    out = area;
                }
            }
        }
    }

    bool RectAlgorithm::ComputeMaxVisibleRegion(const Rect& rect, const vector<Rect>& overlays, Rect& out)
    {
        vector<Rect> filteredOverlays;
        for (auto& overlay : overlays) {
            if (!RectAlgorithm::CheckIntersectant(rect, overlay)) {
                continue; // no intersection, invalid overlay
            }
            auto intersection = Rect(0, 0, 0, 0);
            const auto intersected = RectAlgorithm::ComputeIntersection(rect, overlay, intersection);
            if (intersected && RectAlgorithm::CheckEqual(rect, intersection)) {
                out = Rect(0, 0, 0, 0);
                return false; // fully covered, no visible region
            }
            filteredOverlays.emplace_back(overlay);
        }
        if (filteredOverlays.empty()) {
            out = rect;
            return true; // not covered
        }
        g_XCount = 0;
        g_YCount = 0;
        g_PCount = 0;
        CollectGridLineCoords(rect, rect);
        for (auto& overlay : filteredOverlays) {
            CollectGridLineCoords(rect, overlay);
        }
        for (size_t xIdx = 0; xIdx < g_XCount; xIdx++) {
            for (size_t yIdx = 0; yIdx < g_YCount; yIdx++) {
                CollectGridPoint(g_XCoords[xIdx], g_YCoords[yIdx], rect, filteredOverlays);
            }
        }
        FindMaxVisibleRegion(filteredOverlays, out);
        return out.GetHeight() > 0 && out.GetWidth() > 0;
    }
} // namespace OHOS::uitest
