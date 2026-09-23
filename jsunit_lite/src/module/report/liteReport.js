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

import { TAG } from '../../Constant';

const ACE_LOG_MARKER = '[Console Info]';

function _sleepMs(ms) {
    // Busy-wait to prevent serial output collection from missing test markers during rapid execution
    let now = new Date();
    const exitTime = now.getTime() + ms;
    while (true) {
        now = new Date();
        if (now.getTime() > exitTime) { return }
    }
}

const liteReport = (function () {
    function liteReport(attr) {
        this.id = 'default';
        this.index = 0;
        this.duration = 0;
    }

    liteReport.prototype.init = function (coreContext) {
        this.coreContext = coreContext;
        this.suiteService = this.coreContext.getDefaultService('suite');
        this.specService = this.coreContext.getDefaultService('spec');
    };

    liteReport.prototype.taskStart = function () {
        console.info('[start] start run suites');
    };

    liteReport.prototype.taskDone = function () {
        console.info('[end] run suites end');
    };


    liteReport.prototype.suiteStart = function () {
        const suiteName = this.suiteService.getCurrentRunningSuite().description;
        console.info('[suite start] ' + suiteName);
    };

    liteReport.prototype.suiteDone = function () {
        console.info('[suite end]');
        _sleepMs(30);
    };

    liteReport.prototype.specStart = function () {
        const testName = this.specService.currentRunningSpec.description;
        console.info('[start] ' + testName);
    };

    liteReport.prototype.specDone = function () {
        const spec = this.specService.currentRunningSpec;
        const testName = spec.description;

        if (spec.error || spec.fail) {
            const errorObj = spec.fail || spec.error;
            const errorMsg = errorObj ? (errorObj.message !== undefined ? errorObj.message : String(errorObj)) : 'unknown error';
            console.info('[fail] ' + testName);
            console.info(errorMsg);
        } else if (spec.isSkip) {
            console.info('[ignore] ' + testName);
        } else {
            console.info('[pass] ' + testName);
        }
        _sleepMs(50);
    };

    return liteReport;
})();

liteReport.ACE_LOG_MARKER = ACE_LOG_MARKER;

export default liteReport;
