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
export class ArgumentMatchers {
    constructor() {
        this.ANY = '<any>';
        this.ANY_STRING = '<any String>';
        this.ANY_BOOLEAN = '<any Boolean>';
        this.ANY_NUMBER = '<any Number>';
        this.ANY_OBJECT = '<any Object>';
        this.ANY_FUNCTION = '<any Function>';
        this.MATCH_REGEXS = '<match regexs>';
    }
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
    static matchRegexs(regex) {
        if (ArgumentMatchers.isRegExp(regex)) {
            return regex;
        }
        throw Error('not a regex');
    }
    static isRegExp(value) {
        return Object.prototype.toString.call(value) === '[object RegExp]';
    }
    matcheReturnKey(...args) {
        let arg = args[0];
        let regex = args[1];
        let stubSetKey = args[2];
        if (stubSetKey && stubSetKey == this.ANY) {
            return this.ANY;
        }

        if (typeof arg === 'string' && !regex) {
            return this.ANY_STRING;
        }

        if (typeof arg === 'boolean' && !regex) {
            return this.ANY_BOOLEAN;
        }

        if (typeof arg === 'number' && !regex) {
            return this.ANY_NUMBER;
        }

        if (typeof arg === 'object' && !regex) {
            return this.ANY_OBJECT;
        }

        if (typeof arg === 'function' && !regex) {
            return this.ANY_FUNCTION;
        }

        if (typeof arg === 'string' && regex) {
            return regex.test(arg);
        }
        return null;
    }
    matcheStubKey(key) {
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
        if (ArgumentMatchers.isRegExp(key)) {
            return key;
        }
        return null;
    }
}
