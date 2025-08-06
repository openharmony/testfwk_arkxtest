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

export function assertThrowError(actualValue: () => void, expected: string[]): AssertResult {
  let result = false;
  let message = '';
  if (typeof actualValue !== 'function') {
    throw new Error('actualValue is not a function');
  }
  try {
    actualValue();
    return {
      pass: result,
      message: ' An error is not thrown while it is expected!',
    };
  } catch (err: Error) {
    const length = expected.length;
    const errorType = Class.of(err).getName();
    const errorTypeArr = errorType.split('.');
    const errType = errorTypeArr[errorTypeArr.length - 1];

    if (length === 1) {
      if (expected[0] !== '' && errType === expected[0]) {
        result = true;
      } else {
        result = false;
        message = 'expected throw failed , ' + errorType + ' is not ' + expected[0];
      }
    } else if (length === 2) {
      if ((expected[0] === '' || errType === expected[0]) && err.message === expected[1]) {
        result = true;
      } else if (expected[0] !== '' && errType !== expected[0]) {
        result = false;
        message = 'expected throw failed , ' + errorType + ' is not ' + expected[0];
      } else {
        result = false;
        message = 'expected throw failed , ' + err.message + ' is not ' + expected[1];
      }
    } else {
      result = false;
      message = 'expect one or two parameters!';
    }
  }

  return {
    pass: result,
    message: message,
  };
}
