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

import { AssertResult } from '../modal/assertModel';
import { AnyType } from '../types/common';
import { ArgumentMatchers } from '../mock/ArgumentMatchers';
import { DeepTypeUtils } from './deepEquals/DeepTypeUtils';
import hilog from '@ohos.hilog';

let domain: int = 0x0000;
let tag: string = 'testTag';

export function assertMatchObj(actualValue: AnyType, expected: AnyType[]): AssertResult {
  const expectedObj = expected[0];
  let mismatchedFields: string[] = [];
  let mismatchedDetails: string[] = [];

  if (typeof actualValue !== 'object' || actualValue === null) {
    return {
      pass: false,
      message: 'expect actual value to be an object',
    };
  }

  if (typeof expectedObj !== 'object' || expectedObj === null) {
    return {
      pass: false,
      message: 'expect expected value to be an object',
    };
  }

  const expectedKeys = DeepTypeUtils.getAllKeys(expectedObj as object);
  const actualKeys = DeepTypeUtils.getAllKeys(actualValue as object);

  for (let i = 0; i < expectedKeys.length; i++) {
    const key = expectedKeys[i];
    let expectedValue: AnyType = null;
    let actualValueHasKey = false;
    let actualValueForProperty: AnyType = null;

    try {
      expectedValue = (reflect.Value.of(expectedObj as object) as ClassValue).getFieldByName(key).getData() as AnyType;
    } catch (e) {
      expectedValue = null;
    }

    if (expectedValue === undefined) {
      continue;
    }

    try {
      if (actualKeys.includes(key)) {
        actualValueHasKey = true;
        actualValueForProperty = (reflect.Value.of(actualValue as object) as ClassValue).getFieldByName(key).getData() as AnyType;
      }
    } catch (e) {
      actualValueHasKey = false;
    }
    if (!actualValueHasKey) {
      mismatchedFields.push(key);
      mismatchedDetails.push(`property '${key}' not found in actual object`);
      continue;
    }

    let matchResult = false;
    let matchDetail = '';
    const matcherKey = ArgumentMatchers.matcheStubKey(expectedValue as Object);
    hilog.info(domain, tag, '%{public}s', 'assertMatchObj matcherKey: ' + JSON.stringify(matcherKey));

    if (matcherKey !== null) {
      matchResult = ArgumentMatchers.matcheReturnKey(actualValueForProperty as Object, expectedValue as Object);
      hilog.info(domain, tag, '%{public}s', 'assertMatchObj matchResult: ' + matchResult);
      matchDetail = `${key}: actual=${JSON.stringify(actualValueForProperty)}, expected=matcher(${JSON.stringify(matcherKey)})`;
    } else if (typeof expectedValue === 'object' && expectedValue !== null && !Array.isArray(expectedValue)) {
      const nestedMatch = assertMatchObj(actualValueForProperty, [expectedValue as Object]);
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
      message: 'object matches expected structure',
    };
  } else {
    const errorMessage = mismatchedDetails.join('; ');
    return {
      pass: false,
      message: `object does not match expected structure: ${errorMessage}`,
    };
  }
}
