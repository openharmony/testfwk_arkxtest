/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

import { DeepTypeUtils } from '../assert/deepEquals/DeepTypeUtils';
import { AnyType } from '../types/common';
class LogExpectError {
  static getErrorMsg(matcherName: string, actualValue: AnyType, expect: AnyType, originMsg: string): string {
    if (matcherName === 'assertNull') {
      return 'expect not null, actualValue is ' + String(actualValue);
    }
    if (matcherName === 'assertTrue') {
      return 'expect not true, actualValue is ' + String(actualValue);
    }
    if (matcherName === 'assertFalse') {
      return 'expect not false, actualValue is ' + String(actualValue);
    }
    if (matcherName === 'assertEqual') {
      return 'expect not Equal, actualValue is ' + String(actualValue) + ' equals ' + String(expect);
    }
    if (matcherName === 'assertContain') {
      return 'expect not have, ' + String(actualValue) + ' have ' + String(expect);
    }
    if (matcherName === 'assertInstanceOf') {
      return (
        'expect not InstanceOf, ' + String(actualValue) + ' is ' + DeepTypeUtils.getType(actualValue) + String(expect)
      );
    }
    if (matcherName === 'assertLarger') {
      return 'expect not Larger, ' + String(actualValue) + ' is larger than ' + String(expect);
    }
    if (matcherName === 'assertLargerOrEqual') {
      return 'expect not LargerOrEqual, ' + String(actualValue) + ' larger than ' + String(expect);
    }
    if (matcherName === 'assertLess') {
      return 'expect not Less, ' + String(actualValue) + ' less than ' + String(expect);
    }
    if (matcherName === 'assertLessOrEqual') {
      return 'expect not LessOrEqual, ' + String(actualValue) + ' is less than ' + String(expect);
    }
    if (matcherName === 'assertNaN') {
      return 'expect not NaN, actualValue is ' + String(actualValue);
    }
    if (matcherName === 'assertNegUnlimited') {
      return 'expect not NegUnlimited, actualValue is ' + String(actualValue);
    }
    if (matcherName === 'assertPosUnlimited') {
      return 'expect not PosUnlimited, actualValue is ' + String(actualValue);
    }
    if (matcherName === 'assertUndefined') {
      return 'expect not Undefined, actualValue is ' + String(actualValue);
    }
    return originMsg;
  }
}

export { LogExpectError };
