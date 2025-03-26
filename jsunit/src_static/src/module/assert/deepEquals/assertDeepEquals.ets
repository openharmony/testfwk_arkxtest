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

import { DeepTypeUtils } from './DeepTypeUtils';
import { AnyType } from '../../types/common';
import { AssertResult } from '../../modal/assertModel';

export function assertDeepEquals(actualValue: AnyType, expected: AnyType[]): AssertResult {
  let result = eq(actualValue, expected[0]);
  let msg = result ? '' : logMsg(actualValue, expected[0]);
  return {
    pass: result,
    message: msg,
  };
}

/**
 * 获取失败显示日志
 * @param actualValue 实际对象
 * @param expected 期待比较对象
 */
function logMsg(actualValue: AnyType, expected: AnyType): string {
  const actualMsg = getObjectDesc(actualValue);
  const expectMsg = getObjectDesc(expected);
  return actualMsg + ' is not deep equal ' + expectMsg;
}

function getObjectDesc(value: AnyType) {
  const aClassName = DeepTypeUtils.getType(value);
  let message = '';
  if (aClassName === 'Function' || aClassName === 'Promise') {
    message = 'actualValue ' + aClassName;
  } else if (aClassName === 'Set') {
    const typeValue = value as Set<AnyType>;
    message = 'SET(' + typeValue.size + ') ' + JSON.stringify(Array.from(typeValue));
  } else if (aClassName === 'Map') {
    const typeValue = value as Map<AnyType, AnyType>;
    message = 'Map(' + typeValue.size + ') ';
  } else if (aClassName === 'RegExp') {
    message = 'RegExp ' + JSON.stringify((value as RegExp).source.replace('\\', ''));
  } else if (aClassName === 'Bigint') {
    message = 'Bigint ' + String(value as BigInt);
  } else if (aClassName == 'Error') {
    message = 'Error ' + (value as Error).message;
  } else if (aClassName == 'ArrayBuffer') {
  } else {
    message = 'Bigint ' + JSON.stringify(value);
  }
  return message;
}

function eq(a: AnyType, b: AnyType): boolean {
  const aType = DeepTypeUtils.getType(a);
  const bType = DeepTypeUtils.getType(b);

  if (aType !== bType) {
    return false;
  }
  if (!a || !b) {
    return a === b;
  }

  if (aType === 'Date') {
    return eqOfDate(a as Date, b as Date);
  } else if (aType === 'ArrayBuffer') {
    return eqOfArrayBuffer(a as ArrayBuffer, b as ArrayBuffer);
  } else if (aType === 'RegExp') {
    return eqOfRegExp(a as RegExp, b as RegExp);
  } else if (aType === 'Error') {
    return eqOfError(a as Error, b as Error);
  } else if (aType === 'Map') {
    return eqOfMap(a as Map<AnyType, AnyType>, b as Map<AnyType, AnyType>);
  } else if (aType === 'Set') {
    return eqOfSet(a as Set<AnyType>, b as Set<AnyType>);
  } else if (aType === 'Array') {
    return eqOfArray(a as Array<AnyType>, b as Array<AnyType>);
  } else if (aType === 'Object') {
    return eqOfObj(a as object, b as object);
  } else {
    return a === b;
  }
}

function eqOfDate(a: Date, b: Date): boolean {
  return a.getTime() === b.getTime();
}

function eqOfArrayBuffer(a: ArrayBuffer, b: ArrayBuffer): boolean {
  if (a.byteLength !== b.byteLength) {
    return false;
  }
  const view1 = new Uint8Array(a);
  const view2 = new Uint8Array(b);
  for (let i = 0; i < view1.length; i++) {
    if (view1[i] !== view2[i]) {
      return false;
    }
  }
  return true;
}

function eqOfRegExp(a: RegExp, b: RegExp): boolean {
  return a.source === b.source && a.global === b.global && a.multiline === b.multiline && a.ignoreCase === b.ignoreCase;
}

function eqOfError(a: Error, b: Error): boolean {
  return a.message === b.message;
}

function eqOfMap<k, v>(a: Map<k, v>, b: Map<k, v>): boolean {
  if (a.size !== b.size) {
    return false;
  }
  for (const key of a.keys()) {
    if (!b.has(key) || !eq(b.get(key) as AnyType, a.get(key) as AnyType)) {
      return false;
    }
  }

  return true;
}

function eqOfSet<T>(a: Set<T>, b: Set<T>): boolean {
  if (a.size !== b.size) {
    return false;
  }
  for (const item of a) {
    if (!b.has(item)) {
      return false;
    }
  }
  return true;
}

function eqOfArray<T>(a: Array<T>, b: Array<T>): boolean {
  const aLength = a.length;
  const bLength = b.length;
  if (aLength !== bLength) {
    return false;
  }
  for (let i = 0; i < aLength; i++) {
    if (!eq(a[i] as AnyType, b[i] as AnyType)) {
      return false;
    }
  }
  return true;
}

function eqOfObj(a: object, b: object): boolean {
  const aKeys = DeepTypeUtils.getAllKeys(a);
  const bKeys = DeepTypeUtils.getAllKeys(b);
  if (aKeys.length !== bKeys.length) {
    return false;
  }
  for (const key of aKeys) {
  }
  return true;
}
