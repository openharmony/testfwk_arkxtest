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

import Core from './src/core';
import { TAG, DEFAULT, TestType, Size, Level, PrintTag } from './src/Constant';
import liteReport from './src/module/report/liteReport';
import { describe, it, expect, beforeAll, beforeEach, afterEach, afterAll, beforeItSpecified, afterItSpecified, xdescribe, xit, beforeEachIt, afterEachIt } from './src/interface';
import { SkipError } from './src/service';

const Hypium = {
    hypiumTest: function (abilityDelegator, abilityDelegatorArguments, testsuite) {
        console.info('[Hypium] start');
        const core = Core.getInstance();
        const reportInstance = new liteReport({ id: 'default' });
        core.addService('report', reportInstance);
        console.info('[Hypium] report added');
        core.init();
        console.info('[Hypium] init done');
        testsuite();
        console.info('[Hypium] testsuite done');
        core.execute();
        console.info('[Hypium] execute done');
    }
};

export {
    Hypium,
    Core,
    DEFAULT,
    TestType,
    Size,
    Level,
    PrintTag,
    TAG,
    liteReport,
    SkipError,
    describe, it, expect, beforeAll, beforeEach, afterEach, afterAll,
    beforeItSpecified, afterItSpecified, xdescribe, xit, beforeEachIt, afterEachIt
};
