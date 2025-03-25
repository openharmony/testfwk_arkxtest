/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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
import { DeepTypeUtils } from './deepEquals/DeepTypeUtils';
import { AnyType } from '../types/common';

export function assertEqual(actualValue: AnyType, args: AnyType[]): AssertResult {
  let msg = 'expect ' + actualValue + ' equals ' + args[0];
  if (actualValue === args[0]) {
    // 数值相同,提示数据类型
    const aClassName = DeepTypeUtils.getType(actualValue);
    const bClassName = DeepTypeUtils.getType(args[0]);
    msg = 'expect ' + actualValue + aClassName + ' equals ' + args[0] + bClassName + 'strict mode inspect type';
  }
  return {
    pass: actualValue === args[0],
    message: msg,
  };
}
