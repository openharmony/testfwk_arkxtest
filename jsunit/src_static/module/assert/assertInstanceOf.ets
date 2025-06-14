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

export function assertInstanceOf(actualValue: AnyType, expected: string[]): AssertResult {
  const type = typeof actualValue;
  let pass = false;
  let message = '';
  const expectType = expected[0];
  if (type !== 'object' && type === expectType.toLowerCase()) {
    pass = true;
  } else if (actualValue === null && expectType === 'Null') {
    pass = true;
  } else if (Array.isArray(actualValue) && expectType === 'Array') {
    pass = true;
  } else if (actualValue instanceof Date && expectType === 'Date') {
    pass = true;
  } else if (actualValue instanceof RegExp && expectType === 'RegExp') {
    pass = true;
  } else if (actualValue instanceof Promise && expectType === 'Promise') {
    pass = true;
  } else if (actualValue instanceof Map && expectType === 'Map') {
    pass = true;
  } else if (actualValue instanceof Set && expectType === 'Set') {
    pass = true;
  } else if (actualValue instanceof WeakMap && expectType === 'WeakMap') {
    pass = true;
  } else if (actualValue instanceof WeakSet && expectType === 'WeakSet') {
    pass = true;
  } else if (actualValue instanceof Object && expectType === 'Object') {
    pass = true;
  } else {
    return {
      pass: false,
      message: actualValue + ' is not  ' + expectType,
    };
  }
  return {
    pass,
    message,
  };
}
