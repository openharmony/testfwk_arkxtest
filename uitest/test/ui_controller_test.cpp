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

#include "ui_controller.h"

#include <utility>
#include "gtest/gtest.h"
#include "dummy_controller.h"

using namespace OHOS::uitest;
using namespace std;

class UiControllerTest : public testing::Test {
protected:
    /**
     * Remove registered controllers at teardown.
     * */
    void TearDown() override
    {
        UiController::RemoveAllControllers();
    }
};

TEST_F(UiControllerTest, testGetControllerImplWithNoneRegistered)
{
    ASSERT_EQ(nullptr, UiController::GetController());
}

TEST_F(UiControllerTest, addAndRemoveController)
{
    auto c1 = make_unique<DummyController>("controller1");
    auto c2 = make_unique<DummyController>("controller2");
    c1->SetWorkable(true);
    c2->SetWorkable(true);
    UiController::RegisterController(move(c1), Priority::LOW);
    UiController::RegisterController(move(c2), Priority::HIGH);
    auto ctrl0 = UiController::GetController();
    ASSERT_TRUE(ctrl0 != nullptr);
    ASSERT_EQ("controller2", ctrl0->GetName()) << "Should get c2 because it has higher priority";

    UiController::RemoveController("controller2");
    auto ctrl1 = UiController::GetController();
    ASSERT_TRUE(ctrl1 != nullptr);
    ASSERT_EQ("controller1", ctrl1->GetName()) << "Should get c1 because c2 is is removed";

    UiController::RemoveController("controller1");
    ASSERT_EQ(nullptr, UiController::GetController()) << "Should get null because all controllers are removed";
}

TEST_F(UiControllerTest, controllerPriority)
{
    auto c1 = make_unique<DummyController>("controller1");
    auto c2 = make_unique<DummyController>("controller2");
    auto c3 = make_unique<DummyController>("controller3");
    c1->SetWorkable(true);
    c2->SetWorkable(true);
    c3->SetWorkable(true);
    UiController::RegisterController(move(c1), Priority::LOW);
    UiController::RegisterController(move(c2), Priority::HIGH);
    UiController::RegisterController(move(c3), Priority::MEDIUM);
    auto controller = UiController::GetController();
    ASSERT_TRUE(controller != nullptr);
    ASSERT_EQ("controller2", controller->GetName()) << "Should get ctrl2 because it has highest priority";
}

TEST_F(UiControllerTest, noWorkableController)
{
    auto c1 = make_unique<DummyController>("controller1");
    auto c2 = make_unique<DummyController>("controller2");
    c1->SetWorkable(false);
    c2->SetWorkable(false);
    UiController::RegisterController(move(c1), Priority::LOW);
    UiController::RegisterController(move(c2), Priority::HIGH);
    ASSERT_EQ(nullptr, UiController::GetController()) << "No workable controller should get";
}

TEST_F(UiControllerTest, testControllerWorkable)
{
    auto c1 = make_unique<DummyController>("controller1");
    auto c2 = make_unique<DummyController>("controller2");
    c1->SetWorkable(true);
    c2->SetWorkable(false);
    UiController::RegisterController(move(c1), Priority::LOW);
    UiController::RegisterController(move(c2), Priority::HIGH);
    auto controller = UiController::GetController();
    ASSERT_TRUE(controller != nullptr);
    ASSERT_EQ("controller1", controller->GetName()) << "Should get ctrl1 because ctrl2 is not workable";
}

TEST_F(UiControllerTest, controllerProvider)
{
    UiController::RegisterControllerProvider(nullptr);
    ASSERT_EQ(nullptr, UiController::GetController());

    auto controllerProvider = [](list<unique_ptr<UiController>> &receiver) {
        auto controller = make_unique<DummyController>("dummy_controller");
        controller->SetWorkable(true);
        receiver.push_back(move(controller));
    };

    // register controllerProvider
    UiController::RegisterControllerProvider(controllerProvider);
    UiController::InstallFromProvider();
    ASSERT_NE(nullptr, UiController::GetController());
    UiController::RemoveController("dummy_controller");
    ASSERT_EQ(nullptr, UiController::GetController());
}