/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

function assertThrowError(actualValue, expected) {
    let result = false;
    let message = '';
    let err;
    if (typeof actualValue !== 'function') {
        throw new Error('actualValue is not a function');
    }
    try {
        actualValue();
        return {
            pass: result,
            message: ' An error is not thrown while it is expected!'
        };
    } catch (e) {
        err = e;
    }
    if (err instanceof Error) {
        let type = typeof expected[0];
        if (type === 'function') {
            result = err.constructor.name === expected[0].name;
            message = 'Expected to throw an error of type ' + expected[0].name + ', but the actual error thrown is of type ' + err.constructor.name + '.'
        } else if (type === 'string') {
            result = err.message.includes(expected[0]);
            message = 'Expected to throw an error with message ' + expected[0] + ', but the actual error message thrown is ' + err.message + '.'
        }
    }
    return {
        pass: result,
        message: message
    };
}

export default assertThrowError;
