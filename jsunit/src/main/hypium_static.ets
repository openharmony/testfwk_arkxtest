'use static';
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

import abilityDelegatorRegistry from '@ohos.app.ability.abilityDelegatorRegistry';

import { Core } from './core';
import { TAG } from './Constant';
import DataDriver from './module/config/DataDriver';
import { ExpectExtend } from './module/assert/ExpectExtend';
import { OhReport } from './module/report/OhReport';
import { SysTestKit } from './module/kit/SysTestKit';
import { DataDriverData, TestcaseSummaryIF } from './interface';
import { AnyType } from './module/types/common';
import { ConfigService } from './module/config/configService';
import { SpecService } from './module/service/SpecService';
import { SuiteService } from './module/service/SuiteService';
import { jsonx } from "std/core"
import { DataDriverData, DataDriverSuite,DataDriverAttr} from './interface'



export class Hypium {
  public static context = new Map<string, AnyType>();

  static parseValue(value:jsonx.JsonElement) {
    if(value.tryAsString()) {
      return value.tryAsString()
    } else if(value.tryAsInteger()) {
      return value.tryAsInteger()
    } else if(value.tryAsDouble()) {
      return value.tryAsDouble()
    } else if(value.tryAsBoolean()) {
      return  value.tryAsBoolean()
    } else if(value.tryAsArray()) {
      const valueArray = value.tryAsArray() as Array<jsonx.JsonElement>
      const parsedValue: Array<object | undefined | null>= []
      for(const val of valueArray) {
        parsedValue.push(Hypium.parseValue(val))
      }
      return parsedValue;
    } else {
      try {
        return Hypium.parseParams(value)
      } catch(error) {
        let code = (error as BusinessError).code;
        let message = (error as BusinessError).message;
        hilog.error(domain, tag, `Hypium.parseParams failed, error code: ${code}, message: ${message}.`);
        return null;
      }

    }
  }

  static parseParams(paramsItem: jsonx.JsonElement) {
    const it = paramsItem.$_iterator()
    const tempRecord: Record<string, AnyType> = {}
    while (true) {
      const v = it.next()
      if (v.done) {
        break
      }
      const key = v.value![0]
      const value = v.value![1]

      tempRecord[key] = Hypium.parseValue(value)
    }

    return tempRecord;
  }

  static parseSuite(item:jsonx.JsonElement): DataDriverSuite {
    let describe: string[]|undefined = []
    if(item.tryGetElement("describe")!==undefined) {
      const describeValue = item.getArray("describe")
      for (const describeItem of describeValue) {
        const describeName = describeItem.asString()
        console.log("describeName" + describeName)
        describe?.push(describeName)
      }
    }else{
      describe=undefined
    }

    let params:  Record<string, AnyType> = {};
    let paramsArray: Record<string, AnyType>[] = []
    if(item.tryGetElement("params")!==undefined) {
      const paramsElement = item.getElement("params");
      const paramsValue = paramsElement.tryAsArray();
      if (Array.isArray(paramsValue)) {
        const paramArray = paramsValue as Array<jsonx.JsonElement>
        for (const paramsItem of paramArray) {
          const tempRecord = Hypium.parseParams(paramsItem)
          paramsArray.push(tempRecord)
        }
      } else {
        params = Hypium.parseParams(paramsElement)
      }
    }else{
      params = {}
      console.error(" params in data cannot be undefined");
    }

    let items: DataDriverSuite[]|undefined= [];
    if (item.tryGetElement("items")!==undefined) {
      const itemsValue =item.getArray("items");
      for (const childItem of itemsValue) {
        const parsedChild =Hypium.parseSuite(childItem);
        items?.push(parsedChild);
      }
    }else{
      items=undefined;
    }

    let stress:Double|undefined=1;
    if(item.tryGetElement("stress")!==undefined) {
      stress = item.getElement("stress").asDouble()
    }

    let it:string|undefined=" ";
    if(item.tryGetElement("it")!==undefined) {
      it = item.getElement("it").asString();
    }else{
      it=undefined;
    }

    return {
      describe,
      stress,
      it,
      params: paramsArray.length ? paramsArray : params,
      items
    }
  }

  static setData(JsonData: string) {
    const core = Core.getInstance();
    let jsonData = JSON.parseJsonElement(JsonData);
    let suitesValue: Array<jsonx.JsonElement>|undefined =[];
    if(jsonData.tryGetElement("suites")!==undefined){
      suitesValue = jsonData.getArray("suites");
    }else{
      suitesValue=undefined;
      throw Error('suites is null');
    }
    const dataDriverSuite:DataDriverSuite[] = []
    for(const item of suitesValue) {
      const parseSuite=Hypium.parseSuite(item);
      dataDriverSuite.push(parseSuite);
    }
    const data:DataDriverData = {
      suites: dataDriverSuite
    }
    const dataDriver = new DataDriver({ data });
    if (core) {
      core.addService('dataDriver', dataDriver);
    } else {
      throw Error('core is not created');
    }
  }
  static setTimeConfig(systemTime: number) {
    SysTestKit.systemTime = systemTime;
  }

  static set(key: string, value: AnyType) {
    Hypium.context.set(key, value);
  }

  static get(key: string) {
    return Hypium.context.get(key);
  }

  static init(
    abilityDelegator: abilityDelegatorRegistry.AbilityDelegator,
    abilityDelegatorArguments: abilityDelegatorRegistry.AbilityDelegatorArgs
  ) {
    const core = Core.getInstance();
    const expectExtend = new ExpectExtend({
      id: 'extend',
    });
    const ohReport = new OhReport({
      delegator: abilityDelegator,
      abilityDelegatorArguments: abilityDelegatorArguments,
    });
    SysTestKit.delegator = abilityDelegator;
    if (core) {
      core.addService('expect', expectExtend);
      core.addService('report', ohReport);
      core.init();
      core.subscribeEvent('spec', ohReport);
      core.subscribeEvent('suite', ohReport);
      core.subscribeEvent('task', ohReport);
      const cService = core.getDefaultService('config');
      if (cService !== null && abilityDelegatorArguments !== null) {
        const configService = cService as ConfigService;
        const testParameters = configService.translateParams(
          abilityDelegatorArguments.parameters
        );
        console.info(`${TAG}parameters:${JSON.stringify(testParameters)}`);
        configService.setConfig(testParameters);
      }
    }
    return core;
  }

  static async execute(
    core: Core,
    abilityDelegator: abilityDelegatorRegistry.AbilityDelegator,
    dynamicCount: int
  ) {
    await core.execute(abilityDelegator, dynamicCount);
  }

  static async hypiumTest(
    abilityDelegator: abilityDelegatorRegistry.AbilityDelegator,
    abilityDelegatorArguments: abilityDelegatorRegistry.AbilityDelegatorArgs,
    testsuite: () => void
  ) {
    const core = Hypium.init(abilityDelegator, abilityDelegatorArguments);

    testsuite();
    let staticTotal = 0;
    let specService: SpecService | null = null;
    if (core) {
      specService = (core as Core).getDefaultService('spec') as SpecService;
      staticTotal = specService.totalTest;
    }
    let dynamicTotalValue = 0;
    const totalTest = staticTotal + dynamicTotalValue;
    if (staticTotal > 0) {
      (specService as SpecService).totalTest = totalTest;
      await Hypium.execute(core as Core, abilityDelegator, dynamicTotalValue);
    }
    if (dynamicTotalValue > 0) {
      let summary: TestcaseSummaryIF = {
        total: 0,
        failure: 0,
        error: 0,
        pass: 0,
        ignore: 0,
        duration: 0,
        itItemList: [],
      };
      if (staticTotal > 0) {
        const suiteService = (core as Core).getDefaultService(
          'suite'
        ) as SuiteService;
        summary = suiteService.getSummary();
      }
    }
  }
}
