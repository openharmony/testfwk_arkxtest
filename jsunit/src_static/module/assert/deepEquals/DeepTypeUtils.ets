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

interface MatchIF {
  asymmetricMatch: object;
}

interface DomNode {
  nodeType: byte;
  nodeName: string;
}
export class DeepTypeUtils {
  static getType<T>(value: T): string {
    const type = typeof value;
    if (type !== 'object') {
      return type.slice(0, 1).toUpperCase() + type.slice(1);
    } else if (value === null) {
      return 'Null';
    } else if (Array.isArray(value)) {
      return 'Array';
    } else if (value instanceof Date) {
      return 'Date';
    } else if (value instanceof RegExp) {
      return 'RegExp';
    } else if (value instanceof Promise) {
      return 'Promise';
    } else if (value instanceof Map) {
      return 'Map';
    } else if (value instanceof Set) {
      return 'Set';
    } else if (value instanceof WeakMap) {
      return 'WeakMap';
    } else if (value instanceof WeakSet) {
      return 'WeakSet';
    } else if (value instanceof ArrayBuffer) {
      return 'ArrayBuffer';
    } else if (value instanceof Error) {
      return 'Error';
    } else {
      return 'Object';
    }
  }
  static isA<T>(typeName: string, value: T): boolean {
    return DeepTypeUtils.getType(value).toLowerCase() === typeName.toLowerCase();
  }
  static isAsymmetricEqualityTester(obj: MatchIF | null): boolean {
    return obj ? DeepTypeUtils.isA('Function', obj.asymmetricMatch) : false;
  }

  /**
   * 是否是function
   * @param value
   */
  static isFunction<T>(value: T): boolean {
    return DeepTypeUtils.isA('Function', value);
  }

  /**
   * 是否是undefined
   * @param obj
   */
  static isUndefined<T>(obj: T): boolean {
    return obj === undefined;
  }

  /**
   * 是否是Node
   * @param obj
   */
  static isDomNoded(obj: DomNode): boolean {
    return (
      obj !== null && typeof obj === 'object' && typeof obj.nodeType === 'number' && typeof obj.nodeName === 'string'
    );
  }

  /**
   * 是否是promise对象
   * @param obj
   */
  static isPromise<T>(obj: T): boolean {
    return obj !== null && typeof obj === 'object' && obj instanceof Promise;
  }

  /**
   *  是否是map对象
   * @param obj
   */
  static isMap<T>(obj: T): boolean {
    return obj !== null && typeof obj !== 'undefined' && obj instanceof Map;
  }

  /**
   * 是否是set对象
   * @param obj 对象
   */
  static isSet<T>(obj: T): boolean {
    return obj !== null && typeof obj !== 'undefined' && obj instanceof Set;
  }

  /**
   * 对象是否有key属性
   * @param obj 对象
   * @param key 对象属性名称
   */
  static has(obj: object, key: string): boolean {
    if (obj === null) {
      return false;
    }
    return obj.hasOwnProperty(key);
  }

  /**
   * 获取对象的自有属性
   * @param obj 对象
   * @param isArray 是否是数组,[object Array]
   */
  static keys(obj: object, isArray: boolean): Array<string> {
    const extraKeys = new Array<string>();

    const allKeys = DeepTypeUtils.getAllKeys(obj);

    const keyList = new Array<string>();
    for (const key of allKeys) {
      keyList.push(key);
    }
    if (!isArray) {
      return keyList;
    }
    if (allKeys.length === 0) {
      return keyList;
    }
    for (const k of allKeys) {
      if (!new RegExp('^[0-9]+$').test(k)) {
        extraKeys.push(k);
      }
    }
    return extraKeys;
  }

  /**
   * 获取obj对象的所有属性
   * @param obj obj对象
   */
  static getAllKeys(obj: object): Array<string> {
    const keyList = new Array<string>();
    if (obj === null) {
      return keyList;
    }
    const keys = Object.keys(obj);
    for (const key of keys) {
      keyList.push(key);
    }
    return keyList;
  }
}
