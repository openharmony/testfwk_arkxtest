import { TAG } from './Constant';


function AssertException(message) {
    this.name = 'AssertException';
    this.message = message;
}
AssertException.prototype = Object.create(Error.prototype);
AssertException.prototype.constructor = AssertException;

function SkipError(message) {
    this.name = 'SkipError';
    this.message = message;
}
SkipError.prototype = Object.create(Error.prototype);
SkipError.prototype.constructor = SkipError;

function processFunc(coreContext, func) {
    return function() { return func() };
}

function _isKeysMatch(keys, description) {
    if (Object.prototype.toString.call(keys) === '[object Array]') {
        for (let j = 0; j < keys.length; j++) {
            if (keys[j] === description) { return true }
        }
        return false;
    }
    return typeof keys === 'string' && keys === description;
}

function _executeMatcher(self, name, actualValue, args, wrapped, currentRunningSpec, currentRunningSuite) {
    const result = self.matchers[name](actualValue, args);
    if (wrapped.isNot) {
        result.pass = !result.pass;
        if (!result.pass && !result.message) {
            result.message = 'expected not to ' + name.replace('assert', 'be ') + ' ' + String(actualValue);
        }
    }
    if (!result.pass) {
        let msg = result.message || '';
        if (currentRunningSpec && currentRunningSpec.expectMsg) {
            msg = currentRunningSpec.expectMsg;
        }
        if (currentRunningSpec) {
            currentRunningSpec.expectMsg = '';
        }
        const assertError = new AssertException(msg);
        if (currentRunningSpec) {
            currentRunningSpec.fail = assertError;
        } else if (currentRunningSuite) {
            currentRunningSuite.hookError = assertError;
        }
        wrapped.isNot = false;
        throw assertError;
    }
    if (currentRunningSpec) {
        currentRunningSpec.expectMsg = '';
    }
    wrapped.isNot = false;
    return wrapped;
}

/* ========== SuiteService ========== */

function SuiteService(attr) {
    this.id = attr.id;
    this.rootSuite = new Suite({});
    this.currentRunningSuite = this.rootSuite;
    this.suitesStack = [this.rootSuite];
    this.isSkipSuite = false;
    this.suiteSkipReason = null;
}

SuiteService.prototype.describe = function (desc, func) {
    const self = this;
    const suite = new Suite({ description: desc });
    if (self.isSkipSuite) {
        suite.isSkip = true;
        suite.skipReason = self.suiteSkipReason;
    }
    self.suiteSkipReason = '';
    self.isSkipSuite = false;
    self.currentRunningSuite.childSuites.push(suite);
    self.currentRunningSuite = suite;
    self.suitesStack.push(suite);
    func.call();
    self.suitesStack.pop();
    self.currentRunningSuite = self.suitesStack.pop();
    self.suitesStack.push(self.currentRunningSuite);
};

SuiteService.prototype.xdescribe = function (desc, func, reason) {
    this.isSkipSuite = true;
    this.suiteSkipReason = reason;
    this.describe(desc, func);
};

SuiteService.prototype.beforeAll = function (func) {
    this.currentRunningSuite.beforeAll.push(processFunc(this.coreContext, func));
};
SuiteService.prototype.beforeEach = function (func) {
    this.currentRunningSuite.beforeEach.push(processFunc(this.coreContext, func));
};
SuiteService.prototype.beforeEachIt = function (func) {
    this.currentRunningSuite.beforeEachIt.push(processFunc(this.coreContext, func));
};
SuiteService.prototype.afterAll = function (func) {
    this.currentRunningSuite.afterAll.push(processFunc(this.coreContext, func));
};
SuiteService.prototype.afterEach = function (func) {
    this.currentRunningSuite.afterEach.push(processFunc(this.coreContext, func));
};
SuiteService.prototype.afterEachIt = function (func) {
    this.currentRunningSuite.afterEachIt.push(processFunc(this.coreContext, func));
};
SuiteService.prototype.beforeItSpecified = function (itDescs, func) {
    this.currentRunningSuite.beforeItSpecified.push({ keys: itDescs, func: processFunc(this.coreContext, func) });
};
SuiteService.prototype.afterItSpecified = function (itDescs, func) {
    this.currentRunningSuite.afterItSpecified.push({ keys: itDescs, func: processFunc(this.coreContext, func) });
};

SuiteService.prototype.getCurrentRunningSuite = function () { return this.currentRunningSuite };
SuiteService.prototype.setCurrentRunningSuite = function (suite) { this.currentRunningSuite = suite };
SuiteService.prototype.getRootSuite = function () { return this.rootSuite };

SuiteService.prototype.init = function (coreContext) { this.coreContext = coreContext };

SuiteService.prototype.execute = function () {
    this.coreContext.fireEvents('task', 'taskStart');
    this.rootSuite.run(this.coreContext);
    this.coreContext.fireEvents('task', 'taskDone');
};

SuiteService.prototype.apis = function () {
    const _this = this;
    return {
        describe: function (desc, func) { return _this.describe(desc, func) },
        xdescribe: function (desc, func, reason) { return _this.xdescribe(desc, func, reason) },
        beforeItSpecified: function (itDescs, func) { return _this.beforeItSpecified(itDescs, func) },
        afterItSpecified: function (itDescs, func) { return _this.afterItSpecified(itDescs, func) },
        beforeAll: function (func) { return _this.beforeAll(func) },
        beforeEach: function (func) { return _this.beforeEach(func) },
        beforeEachIt: function (func) { return _this.beforeEachIt(func) },
        afterAll: function (func) { return _this.afterAll(func) },
        afterEach: function (func) { return _this.afterEach(func) },
        afterEachIt: function (func) { return _this.afterEachIt(func) }
    };
};

/* ========== Suite ========== */

function Suite(attrs) {
    this.description = attrs.description || '';
    this.childSuites = [];
    this.specs = [];
    this.beforeAll = [];
    this.afterAll = [];
    this.beforeItSpecified = [];
    this.afterItSpecified = [];
    this.beforeEach = [];
    this.beforeEachIt = [];
    this.afterEach = [];
    this.afterEachIt = [];
    this.duration = 0;
    this.hookError = null;
    this.isSkip = false;
    this.skipReason = '';
}

Suite.prototype.pushSpec = function (spec) { this.specs.push(spec) };

Suite.prototype.runBeforeItSpecified = function (specItem) {
    for (let i = 0; i < this.beforeItSpecified.length; i++) {
        const entry = this.beforeItSpecified[i];
        if (_isKeysMatch(entry.keys, specItem.description)) {
            try { entry.func() } catch (e) { console.error(TAG + (e.stack ? e.stack : String(e))) }
            break;
        }
    }
};

Suite.prototype.runAfterItSpecified = function (specItem) {
    for (let i = 0; i < this.afterItSpecified.length; i++) {
        const entry = this.afterItSpecified[i];
        if (_isKeysMatch(entry.keys, specItem.description)) {
            try { entry.func() } catch (e) { console.error(TAG + (e.stack ? e.stack : String(e))) }
            break;
        }
    }
};

Suite.prototype.run = function (coreContext) {
    const suiteService = coreContext.getDefaultService('suite');
    suiteService.setCurrentRunningSuite(this);
    if (this.description !== '') {
        coreContext.fireEvents('suite', 'suiteStart');
    }
    this.runHookFunc('beforeAll');
    if (this.specs.length > 0 && !this.isSkip) {
        for (let i = 0; i < this.specs.length; i++) {
            if (this.hookError) {
                const spec = this.specs[i];
                spec.isSkip = true;
                spec.run(coreContext);
                continue;
            }
            const spec = this.specs[i];
            this.runBeforeItSpecified(spec);
            this.runHookFunc('beforeEachIt');
            this.runHookFunc('beforeEach');
            spec.run(coreContext);
            this.runHookFunc('afterEach');
            this.runHookFunc('afterEachIt');
            this.runAfterItSpecified(spec);
        }
    }
    if (this.childSuites.length > 0) {
        const beforeEachItSize = this.beforeEachIt.length;
        const afterEachItSize = this.afterEachIt.length;
        for (let i = 0; i < this.childSuites.length; i++) {
            const suite = this.childSuites[i];
            for (let j = 1; j <= beforeEachItSize; j++) {
                suite.beforeEachIt.splice(0, 0, this.beforeEachIt[beforeEachItSize - j]);
            }
            for (let j = 0; j < afterEachItSize; j++) {
                suite.afterEachIt.push(this.afterEachIt[j]);
            }
            suite.run(coreContext);
            suiteService.setCurrentRunningSuite(suite);
        }
    }
    this.runHookFunc('afterAll');
    if (this.description !== '') {
        coreContext.fireEvents('suite', 'suiteDone');
    }
};

function _executeHookFunc(func, hookName, suite) {
    try {
        func();
    } catch (e) {
        if (e instanceof AssertException) {
            suite.hookError = e;
        } else if (hookName === 'beforeAll') {
            suite.hookError = true;
        }
        console.error(TAG + (e.stack ? e.stack : String(e)));
    }
}

Suite.prototype.runHookFunc = function (hookName) {
    if (this[hookName] && this[hookName].length > 0) {
        for (let _hi = 0; _hi < this[hookName].length; _hi++) {
            _executeHookFunc(this[hookName][_hi], hookName, this);
        }
    }
};

SuiteService.Suite = Suite;

/* ========== SpecService ========== */

function SpecService(attr) {
    this.id = attr.id;
    this.totalTest = 0;
    this.hasError = false;
    this.skipSpecNum = 0;
    this.isSkipSpec = false;
    this.specSkipReason = '';
}

SpecService.prototype.init = function (coreContext) { this.coreContext = coreContext };
SpecService.prototype.setCurrentRunningSpec = function (spec) { this.currentRunningSpec = spec };
SpecService.prototype.setStatus = function (obj) { this.hasError = obj };
SpecService.prototype.getStatus = function () { return this.hasError };
SpecService.prototype.getTestTotal = function () { return this.totalTest };
SpecService.prototype.getCurrentRunningSpec = function () { return this.currentRunningSpec };
SpecService.prototype.getSkipSpecNum = function () { return this.skipSpecNum };
SpecService.prototype.initSpecService = function () { this.isSkipSpec = false; this.specSkipReason = '' };

SpecService.prototype.it = function (desc, filter, func, timeout, tag) {
    const suiteService = this.coreContext.getDefaultService('suite');
    const processedFunc = processFunc(this.coreContext, func, timeout);
    const spec = new Spec({ description: desc, fi: filter, fn: processedFunc });
    if (this.isSkipSpec) {
        spec.isSkip = true;
        spec.skipReason = this.specSkipReason;
    }
    this.initSpecService();
    this.totalTest++;
    suiteService.getCurrentRunningSuite().pushSpec(spec);
};

SpecService.prototype.xit = function (desc, filter, func, reason) {
    this.skipSpecNum++;
    this.isSkipSpec = true;
    this.specSkipReason = reason;
    this.it(desc, filter, func);
};

SpecService.prototype.apis = function () {
    const _this = this;
    return {
        it: function (desc, filter, func, timeout, tag) { return _this.it(desc, filter, func, timeout, tag) },
        xit: function (desc, filter, func, reason) { return _this.xit(desc, filter, func, reason) }
    };
};

/* ========== Spec ========== */

function Spec(attrs) {
    this.description = attrs.description || '';
    this.fn = attrs.fn || function() {};
    this.fail = undefined;
    this.error = undefined;
    this.pass = false;
    this.duration = 0;
    this.startTime = 0;
    this.isSkip = false;
    this.skipReason = '';
    this.expectMsg = '';
}

Spec.prototype.setResult = function () {
    if (this.isSkip) { this.pass = false; return }
    this.pass = !this.fail;
};

Spec.prototype.run = function (coreContext) {
    const specService = coreContext.getDefaultService('spec');
    specService.setCurrentRunningSpec(this);
    coreContext.fireEvents('spec', 'specStart');
    this.isExecuted = true;
    if (this.isSkip) {
        coreContext.fireEvents('spec', 'specDone');
        return;
    }
    try {
        this.fn();
        this.setResult();
    } catch (e) {
        if (e instanceof SkipError) {
            this.isSkip = true;
            this.skipReason = e.message;
        } else {
            this.error = e;
            specService.setStatus(true);
        }
    }
    coreContext.fireEvents('spec', 'specDone');
};

SpecService.Spec = Spec;

/* ========== ExpectService ========== */

function ExpectService(attr) {
    this.id = attr.id;
    this.matchers = {};
}

ExpectService.prototype.init = function (coreContext) {
    this.coreContext = coreContext;
    this.addMatchers(this._basicMatchers1());
    this.addMatchers(this._basicMatchers2());
        this.addMatchers(this._basicMatchers3());
};

ExpectService.prototype.addMatchers = function (matchers) {
    for (const name in matchers) {
        if (Object.prototype.hasOwnProperty.call(matchers, name)) {
            this.matchers[name] = matchers[name];
        }
    }
};

ExpectService.prototype._basicMatchers1 = function () {
    return {
        assertEqual: function(actual, args) {
            const pass = actual === args[0];
            return { pass: pass, message: pass ? '' : 'expected ' + actual + ' to equal ' + args[0] };
        },
        assertTrue: function(actual) {
            const pass = !!actual;
            return { pass: pass, message: pass ? '' : 'expected true' };
        },
        assertFalse: function(actual) {
            const pass = !actual;
            return { pass: pass, message: pass ? '' : 'expected false' };
        },
        assertContain: function(actual, args) {
            const pass = String(actual).indexOf(String(args[0])) !== -1;
            return { pass: pass, message: pass ? '' : 'expected to contain ' + args[0] };
        },
        assertNull: function(actual) {
            const pass = actual === null;
            return { pass: pass, message: pass ? '' : 'expected null' };
        },
        assertUndefined: function(actual) {
            const pass = actual === undefined;
            return { pass: pass, message: pass ? '' : 'expected undefined' };
        },
        assertLarger: function(actual, args) {
            const pass = actual > args[0];
            return { pass: pass, message: actual + ' is not larger than ' + args[0] };
        },
        assertLess: function(actual, args) {
            const pass = actual < args[0];
            return { pass: pass, message: actual + ' is not less than ' + args[0] };
        },
        assertClose: function(actual, args) {
            if (actual === null && args[0] === null) {
                throw new Error('actualValue and expected can not be both null');
            }
            const diff = Math.abs(args[0] - actual);
            const actualAbs = Math.abs(actual);
            let result;
            if (actualAbs === 0) {
                result = (diff === 0);
            } else {
                result = (diff / actualAbs < args[1]);
            }
            return { pass: result, message: '|' + actual + ' - ' + args[0] + '|/' + actual + ' is not less than ' + args[1] };
        }
    };
};

function _checkThrowResult(err, args) {
    if (err instanceof Error) {
        const type = typeof args[0];
        if (type === 'function') {
            const pass = err.constructor.name === args[0].name;
            return { pass: pass, message: 'expected throw ' + args[0].name + ', actual ' + err.constructor.name };
        } else if (type === 'string') {
            const pass2 = err.message.indexOf(args[0]) !== -1;
            return { pass: pass2, message: 'expected throw ' + args[0] + ', actual ' + err.message };
        }
    }
    return { pass: false, message: 'unknown throw' };
}

function _assertInstanceOf(actual, args) {
    const pass = Object.prototype.toString.call(actual) === '[object ' + args[0] + ']';
    return { pass: pass, message: pass ? '' : actual + ' is not ' + args[0] };
}

function _assertFail() {
    return { pass: false, message: 'fail' };
}

function _assertThrowError(actual, args) {
    if (typeof actual !== 'function') {
        return { pass: false, message: 'actualValue is not a function' };
    }
    let hasThrow = false;
    let err;
    try { actual() } catch (e) { hasThrow = true; err = e }
    if (!hasThrow) {
        return { pass: false, message: 'An error is not thrown while it is expected' };
    }
    return _checkThrowResult(err, args);
}

ExpectService.prototype._basicMatchers2 = function () {
    return {
        assertInstanceOf: _assertInstanceOf,
        assertFail: _assertFail,
        assertThrowError: _assertThrowError
    };
};

ExpectService.prototype._basicMatchers3 = function () {
    return {
        assertLargerOrEqual: function(actual, args) {
            const pass = actual >= args[0];
            return { pass: pass, message: actual + ' is not larger than or equal ' + args[0] };
        },
        assertLessOrEqual: function(actual, args) {
            const pass = actual <= args[0];
            return { pass: pass, message: actual + ' is not less than or equal ' + args[0] };
        },
        assertNaN: function(actual) {
            return { pass: actual !== actual, message: 'expect NaN, actualValue is ' + actual };
        },
        assertNegUnlimited: function(actual) {
            return { pass: actual === Number.NEGATIVE_INFINITY, message: 'expected -Infinity, actual ' + actual };
        },
        assertPosUnlimited: function(actual) {
            return { pass: actual === Number.POSITIVE_INFINITY, message: 'expected Infinity, actual ' + actual };
        }
    };
};

ExpectService.prototype.basicMatchers = function () {
    const m1 = this._basicMatchers1();
    const m2 = this._basicMatchers2();
    const m3 = this._basicMatchers3();
    for (const name in m2) {
        if (Object.prototype.hasOwnProperty.call(m2, name)) {
            m1[name] = m2[name];
        }
    }
    for (const name in m3) {
        if (Object.prototype.hasOwnProperty.call(m3, name)) {
            m1[name] = m3[name];
        }
    }
    return m1;
};

ExpectService.prototype.expect = function (actualValue) {
    const self = this;
    const specService = self.coreContext.getDefaultService('spec');
    const currentRunningSpec = specService ? specService.getCurrentRunningSpec() : null;
    const suiteService = self.coreContext.getDefaultService('suite');
    const currentRunningSuite = suiteService ? suiteService.getCurrentRunningSuite() : null;

    const wrapped = {
        isNot: false,
        not: function () {
            wrapped.isNot = true;
            return wrapped;
        },
        message: function (msg) {
            if (currentRunningSpec) {
                currentRunningSpec.expectMsg = msg;
            }
            return wrapped;
        }
    };

    for (const matcherName in self.matchers) {
        if (Object.prototype.hasOwnProperty.call(self.matchers, matcherName)) {
            wrapped[matcherName] = function (...args) {
                return _executeMatcher(self, matcherName, actualValue, args, wrapped, currentRunningSpec, currentRunningSuite);
            };
        }
    }
    return wrapped;
};

ExpectService.prototype.apis = function () {
    const self = this;
    return { expect: function (v) { return self.expect(v) } };
};

export {
    SuiteService,
    SpecService,
    ExpectService,
    AssertException,
    SkipError
};
