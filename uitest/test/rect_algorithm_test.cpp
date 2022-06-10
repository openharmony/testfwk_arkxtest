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

#include "gtest/gtest.h"
#include "ui_model.h"

using namespace OHOS::uitest;
using namespace std;

TEST(RectAlgorithmTest, testCheckEqual)
{
    Rect rect(100, 200, 300, 400);
    ASSERT_TRUE(RectAlgorithm::CheckEqual(rect, rect));
    Rect rect1(0, 0, 0, 0);
    ASSERT_FALSE(RectAlgorithm::CheckEqual(rect, rect1));
}

TEST(RectAlgorithmTest, testRectIntersection)
{
    Rect rect0(100, 200, 300, 400);
    Rect rect1(0, 100, 0, 100);
    Rect rect2(200, 300, 400, 500);
    Rect rect3(100, 150, 200, 350);
    Rect rect4(150, 250, 350, 450);
    Rect rect5(120, 180, 320, 380);

    Rect intersection {0, 0, 0, 0};
    ASSERT_FALSE(RectAlgorithm::CheckIntersectant(rect0, rect1));
    ASSERT_FALSE(RectAlgorithm::CheckIntersectant(rect0, rect2));
    ASSERT_TRUE(RectAlgorithm::CheckIntersectant(rect0, rect3));
    ASSERT_FALSE(RectAlgorithm::ComputeIntersection(rect0, rect1, intersection)); // no overlap
    ASSERT_FALSE(RectAlgorithm::ComputeIntersection(rect0, rect2, intersection)); // no overlap
    ASSERT_TRUE(RectAlgorithm::ComputeIntersection(rect0, rect3, intersection)); // x,y-overlap
    ASSERT_EQ(100, intersection.left_);
    ASSERT_EQ(150, intersection.right_);
    ASSERT_EQ(300, intersection.top_);
    ASSERT_EQ(350, intersection.bottom_);
    intersection = {0, 0, 0, 0};
    ASSERT_TRUE(RectAlgorithm::CheckIntersectant(rect0, rect4));
    ASSERT_TRUE(RectAlgorithm::ComputeIntersection(rect0, rect4, intersection)); // x,y-overlap
    ASSERT_EQ(150, intersection.left_);
    ASSERT_EQ(200, intersection.right_);
    ASSERT_EQ(350, intersection.top_);
    ASSERT_EQ(400, intersection.bottom_);
    intersection = {0, 0, 0, 0};
    ASSERT_TRUE(RectAlgorithm::CheckIntersectant(rect0, rect5));
    ASSERT_TRUE(RectAlgorithm::ComputeIntersection(rect0, rect5, intersection)); // fully contained
    ASSERT_EQ(120, intersection.left_);
    ASSERT_EQ(180, intersection.right_);
    ASSERT_EQ(320, intersection.top_);
    ASSERT_EQ(380, intersection.bottom_);
}

TEST(RectAlgorithmTest, testPointInRect)
{
    auto rect = Rect(10, 20, 10, 20);
    // x not in section
    ASSERT_FALSE(RectAlgorithm::IsInnerPoint(rect, Point(5, 15)));
    ASSERT_FALSE(RectAlgorithm::IsInnerPoint(rect, Point(10, 15)));
    ASSERT_FALSE(RectAlgorithm::IsInnerPoint(rect, Point(25, 15)));
    ASSERT_FALSE(RectAlgorithm::IsInnerPoint(rect, Point(20, 15)));
    // y not in section
    ASSERT_FALSE(RectAlgorithm::IsInnerPoint(rect, Point(15, 5)));
    ASSERT_FALSE(RectAlgorithm::IsInnerPoint(rect, Point(15, 10)));
    ASSERT_FALSE(RectAlgorithm::IsInnerPoint(rect, Point(15, 25)));
    ASSERT_FALSE(RectAlgorithm::IsInnerPoint(rect, Point(15, 20)));
    // x and y not in section
    ASSERT_FALSE(RectAlgorithm::IsInnerPoint(rect, Point(5, 5)));
    ASSERT_FALSE(RectAlgorithm::IsInnerPoint(rect, Point(25, 25)));
    // x and y in section
    ASSERT_TRUE(RectAlgorithm::IsInnerPoint(rect, Point(15, 15)));
}

TEST(RectAlgorithmTest, testPointOnRectEdge)
{
    auto rect = Rect(10, 20, 10, 20);
    // on corner
    ASSERT_TRUE(RectAlgorithm::IsPointOnEdge(rect, Point(10, 10)));
    ASSERT_TRUE(RectAlgorithm::IsPointOnEdge(rect, Point(10, 20)));
    ASSERT_TRUE(RectAlgorithm::IsPointOnEdge(rect, Point(20, 10)));
    ASSERT_TRUE(RectAlgorithm::IsPointOnEdge(rect, Point(20, 20)));
    // on edge
    ASSERT_TRUE(RectAlgorithm::IsPointOnEdge(rect, Point(10, 15)));
    ASSERT_TRUE(RectAlgorithm::IsPointOnEdge(rect, Point(15, 10)));
    ASSERT_TRUE(RectAlgorithm::IsPointOnEdge(rect, Point(20, 15)));
    ASSERT_TRUE(RectAlgorithm::IsPointOnEdge(rect, Point(15, 20)));
    // in rect
    ASSERT_FALSE(RectAlgorithm::IsPointOnEdge(rect, Point(15, 15)));
    // out of rect
    ASSERT_FALSE(RectAlgorithm::IsPointOnEdge(rect, Point(5, 10)));
    ASSERT_FALSE(RectAlgorithm::IsPointOnEdge(rect, Point(25, 10)));
    ASSERT_FALSE(RectAlgorithm::IsPointOnEdge(rect, Point(10, 5)));
    ASSERT_FALSE(RectAlgorithm::IsPointOnEdge(rect, Point(10, 25)));
    ASSERT_FALSE(RectAlgorithm::IsPointOnEdge(rect, Point(5, 15)));
    ASSERT_FALSE(RectAlgorithm::IsPointOnEdge(rect, Point(25, 15)));
    ASSERT_FALSE(RectAlgorithm::IsPointOnEdge(rect, Point(15, 5)));
    ASSERT_FALSE(RectAlgorithm::IsPointOnEdge(rect, Point(15, 25)));
}

TEST(RectAlgorithmTest, computeVisibleRegion)
{
    //  ______
    // |      |
    // |  ++++|++++++
    // |  + ..|...  +
    // |__+_:_| _:__+____
    //    + :..|.:  +    |
    //    +    |    +    |
    //    +++++|+++++    |
    //         |_________|
    Rect rect(100, 200, 100, 200);
    vector<Rect> overlaySet = {Rect(0, 140, 0, 140), Rect(160, 250, 160, 250), Rect(120, 160, 120, 160)};
    auto region = Rect(0, 0, 0, 0);
    constexpr size_t rounds = 1E5;
    constexpr size_t middle = rounds / 2;
    vector<uint32_t> elapseUs;
    for (size_t idx = 0; idx < rounds; idx++) {
        const auto micros0 = GetCurrentMicroseconds();
        ASSERT_TRUE(RectAlgorithm::ComputeMaxVisibleRegion(rect, overlaySet, region));
        const auto micros1 = GetCurrentMicroseconds();
        elapseUs.push_back(micros1 - micros0);
    }
    // check result
    auto expectedVisibleRegion = Rect(100, 160, 160, 200);
    ASSERT_TRUE(RectAlgorithm::CheckEqual(region, expectedVisibleRegion));
    // check elapse (use median), should not exceed baseline: 10us
    constexpr uint32_t epalseBaseline = 10;
    sort(elapseUs.begin(), elapseUs.end());
    ASSERT_LE(elapseUs.at(middle), epalseBaseline);
}

TEST(RectAlgorithmTest, computeVisibleRegionSpecialCases)
{
    //   same-to-overlay   fully-covered      not-covered       fully-covered 2      not-covered-2
    //                     _____________       __________       _____________        _______
    //    +++++++++++     | +++++++++++ |     |__________|     | ++++++++++++|      |_______|
    //    +         +     | +         + |     ++++++++++++     |_+__________+|_      +++++++++
    //    +++++++++++     | +++++++++++ |     +          +      |++++++++++++  |     +++++++++__
    //                    |_____________|     ++++++++++++      |______________|       |________|
    //
    Rect rect(100, 200, 100, 200);
    const vector<vector<Rect>> overlaySet = {
        {Rect(100, 200, 100, 200)},
        {Rect(0, 300, 0, 300)},
        {Rect(100, 200, 0, 100)},
        {Rect(0, 200, 0, 150), Rect(100, 300, 150, 300)},
        {Rect(50, 250, 0, 100), Rect(150, 300, 200, 300)}
    };
    const vector<Rect> expectedVisibleRegion = {
        {Rect(0, 0, 0, 0)},
        {Rect(0, 0, 0, 0)},
        {Rect(100, 200, 100, 200)},
        {Rect(0, 0, 0, 0)},
        {Rect(100, 200, 100, 200)}
    };

    for (size_t idx = 0; idx < 5; idx++) {
        auto region = Rect(0, 0, 0, 0);
        const auto visible = RectAlgorithm::ComputeMaxVisibleRegion(rect, overlaySet.at(idx), region);
        const auto& expectedRegion = expectedVisibleRegion.at(idx);
        ASSERT_TRUE(RectAlgorithm::CheckEqual(region, expectedRegion));
        ASSERT_EQ(expectedRegion.GetHeight() > 0 && expectedRegion.GetWidth() > 0, visible);
    }
}