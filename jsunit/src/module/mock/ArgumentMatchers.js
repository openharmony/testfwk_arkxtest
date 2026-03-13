/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

class ArgumentMatchers {
    ANY = '<any>';
    ANY_STRING = '<any String>';
    ANY_BOOLEAN = '<any Boolean>';
    ANY_NUMBER = '<any Number>';
    ANY_OBJECT = '<any Object>';
    ANY_FUNCTION = '<any Function>';
    MATCH_REGEXS = '<match regexs>';
    NOT_STRING = '<not String>';
    NOT_BOOLEAN = '<not boolean>';
    NOT_NUMBER = '<not Number>';
    NOT_OBJECT = '<not Object>';
    NOT_FUNCTION = '<not Function>';
    static CONTAIN_ELEMENT = '<contain element>';
    static CONTAIN_SUBSTRING = '<contain subString>';
    static CONTAIN_SUBARRAY = '<contain subArray>';

    static any() {
    }

    static anyString() {
    }

    static anyBoolean() {
    }

    static anyNumber() {
    }

    static anyObj() {
    }

    static anyFunction() {
    }

    static notString() {
    }

    static notBoolean() {
    }

    static notNumber() {
    }

    static notObj() {
    }

    static notFunction() {
    }

    static matchRegexs() {
        let regex = arguments[0];
        if (ArgumentMatchers.isRegExp(regex)) {
            regex.needMatch = true;
            return regex;
        }
        throw Error('not a regex');
    }

    static notMatchRegexs() {
        let regex = arguments[0];
        if (ArgumentMatchers.isRegExp(regex)) {
            regex.needMatch = false;
            return regex;
        }
        throw Error('not a regex');
    }

    static containSubstring() {
        let substring = arguments[0];
        if (typeof (substring) === "string") {
            return { type_ : ArgumentMatchers.CONTAIN_SUBSTRING, value: substring, needMatch: true };
        }
        throw Error('not a string');
    }

    static notContainSubstring() {
        let substring = arguments[0];
        if (typeof (substring) === "string") {
            return { type_: ArgumentMatchers.CONTAIN_SUBSTRING, value: substring, needMatch: false };
        }
        throw Error('not a string');
    }

    static containElement() {
        let element = arguments[0];
        return { type_: ArgumentMatchers.CONTAIN_ELEMENT, value: element, needMatch: true };
    }

    static notContainElement() {
        let element = arguments[0];
        return { type_: ArgumentMatchers.CONTAIN_ELEMENT, value: element, needMatch: false };
    }

    static containSubArray() {
        let subArray = arguments[0];
        if (subArray?.constructor?.name === "Array") {
            return { type_: ArgumentMatchers.CONTAIN_SUBARRAY, value: subArray, needMatch: true };
        }
        throw Error('not a Array');
    }

    static notContainSubArray() {
        let subArray = arguments[0];
        if (subArray?.constructor?.name === "Array") {
            return { type_: ArgumentMatchers.CONTAIN_SUBARRAY, value: subArray, needMatch: false };
        }
        throw Error('not a Array');
    }

    static isRegExp(value) {
        return Object.prototype.toString.call(value) === '[object RegExp]';
    }

    matcheReturnKey() {
        let arg = arguments[0];
        let stubSetKey = arguments[1];

        if (stubSetKey && stubSetKey === this.any && arg) {
            return true;
        }

        if (typeof arg === 'string' && stubSetKey === this.anyString) {
            return true;
        }

        if (typeof arg === 'boolean' && stubSetKey === this.anyBoolean) {
            return true;
        }

        if (typeof arg === 'number' && stubSetKey === this.anyNumber) {
            return true;
        }

        if (typeof arg === 'object' && stubSetKey === this.anyObj) {
            return true;
        }

        if (typeof arg === 'function' && stubSetKey === this.anyFunction) {
            return true;
        }

        if (typeof arg !== 'string' && stubSetKey === this.notString) {
            return true;
        }

        if (typeof arg !== 'boolean' && stubSetKey === this.notBoolean) {
            return true;
        }

        if (typeof arg !== 'number' && stubSetKey === this.notNumber) {
            return true;
        }

        if (typeof arg !== 'object' && stubSetKey === this.notObj) {
            return true;
        }

        if (typeof arg !== 'function' && stubSetKey === this.notFunction) {
            return true;
        }

        if (Array.isArray(arg) && stubSetKey && stubSetKey?.type_ === ArgumentMatchers.CONTAIN_ELEMENT) {
            let needMatch = stubSetKey.needMatch !== false;
            return arg.includes(stubSetKey.value) === needMatch;
        }

        if (Array.isArray(arg) && stubSetKey && stubSetKey?.type_ === ArgumentMatchers.CONTAIN_SUBARRAY) {
            let needMatch = stubSetKey.needMatch !== false;
            return stubSetKey.value.every(item => arg.includes(item)) === needMatch;
        }

        if (typeof(arg) == "string" && stubSetKey && stubSetKey?.type_ === ArgumentMatchers.CONTAIN_SUBSTRING) {
            let needMatch = stubSetKey.needMatch !== false;
            return arg.includes(stubSetKey.value) === needMatch;
        }

        if (ArgumentMatchers.isRegExp(stubSetKey) && typeof arg === 'string') {
            let needMatch = stubSetKey.needMatch !== false;
            return stubSetKey.test(arg) === needMatch;
        }

        return false;
    }

    matcheStubKey() {
        let key = arguments[0];

        if (key === ArgumentMatchers.any) {
            return this.ANY;
        }

        if (key === ArgumentMatchers.anyString) {
            return this.ANY_STRING;
        }
        if (key === ArgumentMatchers.anyBoolean) {
            return this.ANY_BOOLEAN;
        }
        if (key === ArgumentMatchers.anyNumber) {
            return this.ANY_NUMBER;
        }
        if (key === ArgumentMatchers.anyObj) {
            return this.ANY_OBJECT;
        }
        if (key === ArgumentMatchers.anyFunction) {
            return this.ANY_FUNCTION;
        }

        if (key === ArgumentMatchers.notString) {
            return this.NOT_STRING;
        }
        if (key === ArgumentMatchers.notBoolean) {
            return this.NOT_BOOLEAN;
        }
        if (key === ArgumentMatchers.notNumber) {
            return this.NOT_NUMBER;
        }
        if (key === ArgumentMatchers.notObj) {
            return this.NOT_OBJECT;
        }
        if (key === ArgumentMatchers.notFunction) {
            return this.NOT_FUNCTION;
        }

        if (ArgumentMatchers.isRegExp(key)) {
            return key;
        }

        if (key && key.type_ === ArgumentMatchers.CONTAIN_ELEMENT) {
            return key;
        }

        if (key && key.type_ === ArgumentMatchers.CONTAIN_SUBARRAY) {
            return key;
        }

        if (key && key.type_ === ArgumentMatchers.CONTAIN_SUBSTRING) {
            return key;
        }
        return null;
    }
}

export default ArgumentMatchers;