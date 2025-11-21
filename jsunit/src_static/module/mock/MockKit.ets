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

import hilog from '@ohos.hilog';
import {expect} from '../../index';
import { DeepTypeUtils } from '../assert/deepEquals/DeepTypeUtils'

let domain: int = 0x0000;
let tag: string = 'testTag';

interface MapKey {
  methodName: string,
  args: object[],
  classObj: object
}

interface MockFuncResult {
  methodName: string,
  mocker: MockKit,
  classObj: object
}

export class NoResult {
  message: string = ""
  code: number
}

export class MockKit {
  flag: boolean
  hashMap: Map<MapKey, string>
  resultMap: Map<string, object | null | undefined>
  propertyMap:Map<string, object | null | undefined>
  countMap: Map<string, Map<object, object[][]>>
  recordCalls: Map<string, int>
  functionHashCodeList: Array<string>
  constructor() {
    this.flag = false
    this.hashMap = new Map<MapKey, string>()
    this.resultMap = new Map<string, object | null | undefined>()
    this.propertyMap = new Map<string, object | null | undefined>()
    this.countMap = new Map<string, Map<object, object[][]>>()
    this.recordCalls = new Map<string, int>()
    this.functionHashCodeList = new Array<string>()
  }

  mockProperty(instance: object, name: string, value: object)
  {
    const that = this
    let isMocked = false;
    let hashKey = ""
    for (let key of this.hashMap.keys()) {
      const keyName = key.methodName
      const keyClassObj = key.classObj
      if (instance === keyClassObj && name === keyName) {
        isMocked = true;
        hashKey = this.hashMap.get(key) as string
        break;
      }
    }
    const getValueMethod = "__getProperty_" + name + "__";
    const setValueMethod = "__setProperty_" + name + "__";
    const classType = Type.of(instance) as ClassType
    if (!isMocked) {
      for (let i = 0; i < classType.getMethodsNum(); i++) {
        let method = classType.getMethod(i)
        if (method.getName() === getValueMethod) {
          const args:  FixedArray<object> =  []
          try {
            let oriValue = method.invoke(instance, args) as object
            const args0:  Array<object> =  []
            const key: MapKey = {
              methodName: name,
              args: args0,
              classObj: instance
            }
            const keyInfo = new AfterWhen(key, that)
            this.propertyMap.set(keyInfo.hashKey, oriValue)
          } catch (e) {
            console.info("catch error:" + e)
    }
          hilog.info(domain, tag, '%{public}s', 'invoke over')
        }
      }
    }
    for (let i = 0; i < classType.getMethodsNum(); i++) {
      let method = classType.getMethod(i)
      if (method.getName() === setValueMethod) {
        const args:  FixedArray<object> =  [value]
        try {
          method.invoke(instance, args)
        } catch (e) {
          console.info("catch error:" + e)
    }
        hilog.info(domain, tag, '%{public}s', 'invoke over')
      }
    }
  }
  mockPrivateFunc(instance: object, name: string) {
    if (!instance || !name) {
      throw Error("parameters is invalid")
    }
    const that = this
    this.setMockerOfUserClass(instance, that)
    return () => {
      const res: MockFuncResult = {
        methodName: name,
        mocker: that,
        classObj: instance
      }
      return res
    }
  }

  mockFunc(instance: object, method: Function) {
    if (!instance || !method) {
      throw Error("parameters is invalid")
    }
    const name = method.name
    return this.mockPrivateFunc(instance, name)
  }
  setMockerOfUserClass(instance: object, mocker: MockKit) {
    hilog.info(domain, tag, '%{public}s', 'start run setMockerOfUserClass')
    const classType = Type.of(instance) as ClassType
    for (let i = 0; i < classType.getMethodsNum(); i++) {
      let method = classType.getMethod(i)
      hilog.info(domain, tag, 'method %{public}s', method.getName())
      if (method.getName() === "__setMocker__") {
        hilog.info(domain, tag, '%{public}s', '__setMocker__ in')
        const args:  FixedArray<MockKit> =  [mocker]
        try {
          method.invoke(instance, args)
        } catch (e) {
            console.info("catch error:" + e)
        }
        hilog.info(domain, tag, '%{public}s', 'invoke over')
      }
    }
  }

  checkIsEqualOfArgs(args1: Object[], args2: Object[]) {
    hilog.info(domain, tag, '%{public}s', 'start run MockKit checkIsEqualOfArgs ')
    hilog.info(domain, tag, 'args1: %{public}s, args2: %{public}s', String(args1), String(args2))

    if (args1.length !== args2.length) {
      return false;
    }
    for (let i = 0; i < args1.length; i++) {
      if (args1[i] === ArgumentMatchers.any) {
        continue;
      } else if (args1[i] === ArgumentMatchers.anyString && typeof args2[i] === 'string') {
        continue;
      } else if (args1[i] === ArgumentMatchers.anyBoolean && typeof args2[i] === 'boolean') {
        continue;
      } else if (args1[i] === ArgumentMatchers.anyNumber && typeof args2[i] === 'number') {
        continue;
      } else if (args1[i] === ArgumentMatchers.anyObj && typeof args2[i] === 'object') {
        continue;
      } else if (args1[i] === ArgumentMatchers.anyFunction && typeof args2[i] === 'function') {
        continue;
      } else if (ArgumentMatchers.isRegExp(args1[i]) && typeof args2[i] === 'string') {
        let re = args1[i] as RegExp;
        let str = args2[i] as String
        let isMatch = re.test(str)
        if (isMatch) {
          continue;
        }
      }
      if (args1[i] !== args2[i]) {
        return false
      }
    }
    return true
  }

  setCountInfo(instance: object, className: string, methodName: string, args: Object[]) {
    hilog.info(domain, tag, '%{public}s', 'start run setCountInfo')
    const countMap = this.countMap
    const key = className + "--" + methodName
    if (countMap.has(key)) {
      const instanceMap = countMap.get(key)
      if (instanceMap === null) {
        return
      }
      if (instanceMap?.has(instance)) {
        const argsList = instanceMap?.get(instance)
        argsList?.push(args)
      } else {
        instanceMap?.set(instance, [args])
      }
    } else {
      const instanceMap = new Map<object, object[][]>()
      instanceMap.set(instance, [args])
      countMap.set(key, instanceMap)
    }
  }

  getReturnInfo(instance: object, className: string, methodName: string, args: Object[]) {
    hilog.info(domain, tag, '%{public}s', 'start run getReturnInfo')
    const hashMap = this.hashMap

    let methondCall : string = methodName + '#' + string(args)
    hilog.info(domain, tag, 'call %{public}s', methondCall)
    let recordTimes = this.recordCalls.get(methondCall);
    if (!recordTimes) {
      recordTimes = 0
    }
    recordTimes++;
    this.recordCalls.set(methondCall, recordTimes);

    for (let key of hashMap.keys()) {
      let hashKey: string = hashMap.get(key) as string
      const keyMethodName = key.methodName
      const keyArgs = key.args
      const keyClassObj = key.classObj
      if (!keyArgs || !keyClassObj) {
        continue
      }
      if (keyMethodName === methodName && this.checkIsEqualOfArgs(keyArgs, args) && keyClassObj === instance) {
        const returnValue = this.resultMap.get(hashKey)
        if (returnValue instanceof Error) {
          throw (returnValue as Error)
        }
        if (returnValue instanceof Function && this.functionHashCodeList.includes(hashKey)) {
          hilog.info(domain, tag, '%{public}s', 'start invoke action')
          let fun = returnValue as ()=>void
          fun()
        } else {
          return returnValue
        }
      }
      hilog.info(domain, tag, '%{public}s', 'Not found mock')
    }
    const noResult: NoResult = {
      message: "mocker result not found",
      code: -1
    }
    return noResult
  }

  verify(methodName: string, args: object[]) {
    hilog.info(domain, tag, '%{public}s', 'start run verify 1111')
    let record = methodName + '#' + string(args)
    let invokeTimes = this.recordCalls.get(record)
    if (!invokeTimes) {
      invokeTimes = 0
    }
    hilog.info(domain, tag, 'verify record %{public}s, has invoke %{public}d', record, invokeTimes)
    return new VerificationMode(invokeTimes? invokeTimes: 0)
  }

  clear(instance: object) {
    hilog.info(domain, tag, '%{public}s', 'start run clear  1111')
    for (const hashEntries of this.hashMap.entries()) {
      if (!hashEntries) {
        continue
      }
      const key = hashEntries[0]
      const keyObject = key.classObj
      if (instance === keyObject) {
        const hashValue = this.hashMap.get(key) as string
        if (this.propertyMap.has(hashValue)) {
          const oriValue = this.propertyMap.get(hashValue) as object
          let name = key.methodName
          const classType = Type.of(instance) as ClassType
          const setValueMethod = "__setProperty_" + name + "__";
          for (let i = 0; i < classType.getMethodsNum(); i++) {
            let method = classType.getMethod(i)
            if (method.getName() === setValueMethod) {
              const args: FixedArray<object> = [oriValue]
              try {
                method.invoke(instance, args)
              } catch (e) {
                console.info("catch error:" + e)
              }
              hilog.info(domain, tag, '%{public}s', 'invoke over')
              break;
            }
          }
        }
        this.hashMap.delete(key)
      }
    }
  }

  ignoreMockProperty(instance: object, name:string)
  {
    for (const hashEntries of this.hashMap.entries()) {
      if (!hashEntries) {
        continue
      }
      const key = hashEntries[0]
      const keyMethodName = key.methodName
      const keyObject = key.classObj
      if (instance === keyObject && keyMethodName === name) {
        const hashValue = this.hashMap.get(key) as string
        const oriValue = this.propertyMap.get(hashValue) as object
        const classType = Type.of(instance) as ClassType
        const setValueMethod = "__setProperty_" + name + "__";
        for (let i = 0; i < classType.getMethodsNum(); i++) {
          let method = classType.getMethod(i)
          if (method.getName() === setValueMethod) {
            const args: FixedArray<object> = [oriValue]
            try {
              method.invoke(instance, args)
            } catch (e) {
              console.info("catch error:" + e)
            }
            hilog.info(domain, tag, '%{public}s', 'invoke over')
          }
        }
      }
      return;
    }
  }

  ignoreMock(instance: object, method: Function | string) {
    if (!instance || !method) {
      throw Error("parameters is invalid")
    }
    if (typeof method === "string") {
      this.ignoreMockProperty(instance, method as string)
      return;
    }
    const name = (method as Function).name
    let hashMap = this.hashMap
    for (const hashEntries of hashMap.entries()) {
      if (!hashEntries) {
        continue
      }
      const key = hashEntries[0]
      const keyMethodName = key.methodName
      const keyObject = key.classObj
      if (instance === keyObject && keyMethodName === name) {
        hashMap.delete(key)
      }
    }
  }
}

class VerificationMode {
  doTimes:int
  constructor(invokeTimes: int) {
    this.doTimes = invokeTimes
  }

  once() {
    expect(1).assertEqual(this.doTimes);
  }

  times(count: int) {
    expect(count).assertEqual(this.doTimes);
  }

  never() {
    expect(0).assertEqual(this.doTimes);
  }

  atLeast(count: int) {
    if (count > this.doTimes) {
      throw Error('failed ' + count + ' greater than the actual execution times of method');
    }
  }

  atMost(count: int) {
    if (count < this.doTimes) {
      throw Error('failed ' + count + ' less than the actual execution times of method');
    }
  }
}

class AfterWhen{
  mocker: MockKit
  keyInfo: MapKey
  hashKey: string
  constructor(keyInfo: MapKey, mocker: MockKit) {
    this.keyInfo = keyInfo
    this.mocker = mocker
    this.hashKey = ""
    this.generateHashKey()
  }

  checkIsEqualOfArgs(args1: Object[], args2: Object[]) {
    if (args1.length !== args2.length) {
      return false;
    }
    for (let i = 0; i < args1.length; i++) {
      if (args1[i] !== args2[i]) {
        hilog.info(domain, tag, '%{public}d, args1: %{public}s, args2: %{public}s', i, String(args1[i]), String(args2[i]))
        return false
      }
    }
    return true
  }

  generateHashKey() {
    hilog.info(domain, tag, '%{public}s', 'start run AfterWhen  generateHashKey')
    const hashMap = this.mocker.hashMap
    let found = false
    for (const mapKeyEntries of hashMap.entries()) {
      if (!mapKeyEntries) {
        continue
      }
      const key = mapKeyEntries[0]
      const hashValue = mapKeyEntries[1]
      const keyMethodName = key.methodName
      const keyArgs = key.args
      const keyClassObj = key.classObj
      if (!keyMethodName || !keyArgs) {
        continue
      }
      const methodName = this.keyInfo.methodName
      const args = this.keyInfo.args
      const classObj = this.keyInfo.classObj
      if (keyMethodName === methodName && this.checkIsEqualOfArgs(keyArgs, args) && keyClassObj === classObj) {
        found = true
        this.hashKey = hashValue
        break
      }
    }
    if (!found) {
      const now = new Date().getTime()
      const random = Math.floor(Math.random() * 1000)
      this.hashKey = String(now) + String(random)
      hashMap.set(this.keyInfo, this.hashKey)
    }
    for (const mapKeyEntries of hashMap.entries()) {
      if (!mapKeyEntries) {
        continue
      }
      const key = mapKeyEntries[0]
      const hashValue = mapKeyEntries[1]
      const keyMethodName = key.methodName
      const keyArgs = key.args
      hilog.info(domain, tag, 'hashmap { keyClassName:%{public}s, keyMethodName:%{public}s, keyArgs:%{public}s}, hashValue: %{public}s',
        keyMethodName, String(keyArgs), hashValue)
    }

  }

  afterReturn(value: object) {
    hilog.info(domain, tag, '%{public}s', 'start run afterReturn, hashKey: ' + this.hashKey)
    hilog.info(domain, tag, '%{public}s', 'start run afterReturn, keyInfo: ' + string(this.keyInfo.args))
    let map = this.mocker.resultMap
    if (!map.has(this.hashKey)) {
      map.set(this.hashKey, value)
    }
    for (const m of map.entries()) {
      if (!m) {
        continue
      }
      const hash = m[0]
      const value = m[1]
      hilog.info(domain, tag, 'afterReturn resultMap hash:%{public}s, value:%{public}s',
        hash, JSON.stringify(value))
    }
  }

  afterReturnNothing() {
    const map = this.mocker.resultMap
    map.set(this.hashKey, undefined)
    let hashMap = this.mocker.hashMap
    hashMap.set(this.keyInfo, this.hashKey)
  }

  afterThrow(message: string) {
    let hashMap = this.mocker.hashMap
    hashMap.set(this.keyInfo, this.hashKey)
    const map = this.mocker.resultMap
    map.set(this.hashKey, new Error(message))
  }

  afterAction(action: Function) {
    let hashMap = this.mocker.hashMap
    hashMap.set(this.keyInfo, this.hashKey)
    this.mocker.functionHashCodeList.push(this.hashKey)
    const map = this.mocker.resultMap
    map.set(this.hashKey, action)
  }
}

export function when(mockFunc: Function) {
  hilog.info(domain, tag, '%{public}s', 'start run when')
  const mockFuncResult: MockFuncResult = (mockFunc as () => MockFuncResult) ()
  return (...args: object[]) => {
    const methodName = mockFuncResult.methodName
    const classObj = mockFuncResult.classObj
    const key:MapKey = {
      methodName,
      args,
      classObj
    }
    return new AfterWhen(key, mockFuncResult.mocker)
  }
}

export class ArgumentMatchers{

  MATCH_REGEXS = "<match regexs>";


  constructor() {
    hilog.info(domain, tag, '%{public}s ', "ArgumentMatchers constructor")
  }

  static any() {
  }

  static anyString() {
  }

  static anyBoolean() {
  }

  static anyNumber() {
  }

  static anyObj() {
  }

  static anyFunction() {
  }

  static matchRegexs(value: RegExp) {
    if (ArgumentMatchers.isRegExp(value)) {
      return value;
    }
    throw Error('not a regex');
  }

  static isRegExp<T>(value: T) : boolean {
    let ret  = DeepTypeUtils.getType(value) === 'RegExp';
    hilog.info(domain, tag, 'isRegExp %{public}d ', ret);
    return ret;
  }
}
