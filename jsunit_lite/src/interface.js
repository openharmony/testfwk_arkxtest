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

import Core from './core';

const core = Core.getInstance();

const describe = function (desc, func) {
    if (typeof core.describe === 'function') {
        return core.describe(desc, func);
    }
    return undefined;
};

const it = function (desc, filter, func, timeout, tag) {
    if (typeof core.it === 'function') {
        return core.it(desc, filter, func, timeout, tag);
    }
    return undefined;
};

const beforeItSpecified = function (itDescs, func) {
    if (typeof core.beforeItSpecified === 'function') {
        return core.beforeItSpecified(itDescs, func);
    }
    return undefined;
};

const afterItSpecified = function (itDescs, func) {
    if (typeof core.afterItSpecified === 'function') {
        return core.afterItSpecified(itDescs, func);
    }
    return undefined;
};

const beforeEach = function (func) {
    if (typeof core.beforeEach === 'function') {
        return core.beforeEach(func);
    }
    return undefined;
};

const afterEach = function (func) {
    if (typeof core.afterEach === 'function') {
        return core.afterEach(func);
    }
    return undefined;
};

const beforeEachIt = function (func) {
    if (typeof core.beforeEachIt === 'function') {
        return core.beforeEachIt(func);
    }
    return undefined;
};

const afterEachIt = function (func) {
    if (typeof core.afterEachIt === 'function') {
        return core.afterEachIt(func);
    }
    return undefined;
};

const beforeAll = function (func) {
    if (typeof core.beforeAll === 'function') {
        return core.beforeAll(func);
    }
    return undefined;
};

const afterAll = function (func) {
    if (typeof core.afterAll === 'function') {
        return core.afterAll(func);
    }
    return undefined;
};

const expect = function (actualValue) {
    if (typeof core.expect === 'function') {
        return core.expect(actualValue);
    }
    return undefined;
};

const xdescribe = function (desc, func) {
    if (typeof core.xdescribe === 'function') {
        return core.xdescribe(desc, func, null);
    }
    return undefined;
};
xdescribe.reason = function (reason) {
    return function (desc, func) {
        if (typeof core.xdescribe === 'function') {
            return core.xdescribe(desc, func, reason);
        }
        return undefined;
    };
};

const xit = function (desc, filter, func) {
    if (typeof core.xit === 'function') {
        return core.xit(desc, filter, func, null);
    }
    return undefined;
};
xit.reason = function (reason) {
    return function (desc, filter, func) {
        if (typeof core.xit === 'function') {
            return core.xit(desc, filter, func, reason);
        }
        return undefined;
    };
};

export {
    describe, it, beforeAll, beforeEach, beforeEachIt,
    afterEach, afterEachIt, afterAll, expect,
    beforeItSpecified, afterItSpecified, xdescribe, xit
};
