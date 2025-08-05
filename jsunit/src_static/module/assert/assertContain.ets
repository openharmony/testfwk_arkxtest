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

export function assertContain(actualValue: AnyType, expect: AnyType[]): AssertResult {
  let result = false;
  const expectValue = expect[0];
  if (typeof actualValue === 'string' && typeof expectValue === 'string') {
    const acValue = actualValue as string;
    const epValue = expectValue as string;
    result = acValue.indexOf(epValue) >= 0;
  } else if (Array.isArray(actualValue)) {
    const acValue = actualValue as AnyType[];
    for (let value of acValue) {
      if (value == expectValue) {
        result = true;
      }
    }
  }

  return {
    pass: result,
    message: 'expect false, ' + actualValue + ' do not have  ' + expect[0],
  };
}
