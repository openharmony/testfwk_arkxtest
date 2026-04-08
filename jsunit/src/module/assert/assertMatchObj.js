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

import ArgumentMatchers from '../mock/ArgumentMatchers';

const MAX_DEPTH = 6;

function assertMatchObj(actualValue, expected, depth = 0) {
    const expectedObj = expected[0];
    const matcher = new ArgumentMatchers();
    let mismatchedFields = [];
    let mismatchedDetails = [];

    if (depth >= MAX_DEPTH) {
        return {
            pass: false,
            message: `maximum recursion depth ${MAX_DEPTH} exceeded`
        };
    }

    if (typeof actualValue !== 'object' || actualValue === null) {
        return {
            pass: false,
            message: 'expect actual value to be an object'
        };
    }

    if (typeof expectedObj !== 'object' || expectedObj === null) {
        return {
            pass: false,
            message: 'expect expected value to be an object'
        };
    }


    const expectedKeys = Object.keys(expectedObj);

    for (const key of expectedKeys) {
        const expectedValue = expectedObj[key];
        const actualValueHasKey = actualValue.hasOwnProperty(key);
        const actualValueForProperty = actualValue[key];
        let matchResult = false;
        let matchDetail = '';

        if (!actualValueHasKey) {
            mismatchedFields.push(key);
            mismatchedDetails.push(`property '${key}' not found in actual object`);
            continue;
        }

        const matcherKey = matcher.matcheStubKey(expectedValue);
        if (matcherKey !== null) {
            matchResult = matcher.matcheReturnKey(actualValueForProperty, matcherKey);
            matchDetail = `${key}: actual=${JSON.stringify(actualValueForProperty)}, expected=matcher(${matcherKey})`;
        } else if (typeof expectedValue === 'object' && expectedValue !== null && !Array.isArray(expectedValue)) {
            const nestedMatch = assertMatchObj(actualValueForProperty, [expectedValue], depth + 1);
            matchResult = nestedMatch.pass;
            matchDetail = `${key}: ${nestedMatch.message}`;
        } else {
            matchResult = actualValueForProperty === expectedValue;
            matchDetail = `${key}: actual=${JSON.stringify(actualValueForProperty)}, expected=${JSON.stringify(expectedValue)}`;
        }

        if (!matchResult) {
            mismatchedFields.push(key);
            mismatchedDetails.push(matchDetail);
        }
    }

    if (mismatchedFields.length === 0) {
        return {
            pass: true,
            message: 'object matches expected structure'
        };
    } else {
        const errorMessage = mismatchedDetails.join('; ');
        return {
            pass: false,
            message: `object does not match expected structure: ${errorMessage}`
        };
    }
}

export default assertMatchObj;
