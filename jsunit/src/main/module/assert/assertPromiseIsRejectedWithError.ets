'use static'
/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

export function assertPromiseIsRejectedWithError(
  actualPromise: Promise<AnyType>,
  expectedValue: string[]
): Promise<AssertResult> {
  return actualPromise.then<AssertResult, AssertResult>(
    (): AssertResult => {
      return {
        pass: false,
        message: 'Expected a promise to be rejected but actualValue is resolve',
      };
    },
    (actualValue: AnyType): AssertResult => {
      return matchError(actualValue, expectedValue);
    }
  );
}

function matchError(actualValue: AnyType, expectedValue: string[]): AssertResult {
  const errorType = Class.of(actualValue as object).getName();
  const errorTypeArr = errorType.split('.');
  const errType = errorTypeArr[errorTypeArr.length - 1];
  if (expectedValue.length == 1) {
    if (errType === expectedValue[0]) {
      return { pass: true, message: 'actual error type is ' + errorType + '.' };
    } else {
      return { pass: false, message: `except error type is ${expectedValue[0]},but actual is ${errorType}.` };
    }
  } else if (expectedValue.length == 2) {
    if (
      (expectedValue[0] === '' || errType === expectedValue[0]) &&
      (actualValue as Error).message === expectedValue[1]
    ) {
      return { pass: true, message: 'actual error message is ' + (actualValue as Error).message + '.' };
    } else if (
      expectedValue[0] !== '' &&
      errType !== expectedValue[0] &&
      (actualValue as Error).message === expectedValue[1]
    ) {
      return { pass: false, message: `expect error type is ${expectedValue[0]},but actual is ${errorType}.` };
    } else if (
      (expectedValue[0] === '' || errType === expectedValue[0]) &&
      (actualValue as Error).message !== expectedValue[1]
    ) {
      return {
        pass: false,
        message: `expect error message is ${expectedValue[1]},but actual is ${(actualValue as Error).message}.`,
      };
    } else {
      return { pass: false, message: 'expect error type and message are incorrect.' };
    }
  } else {
    return { pass: false, message: 'Up to two parameters are supported.' };
  }
}
