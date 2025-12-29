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
import { TAG, PrintTag } from './Constant';
import DataDriver from './module/config/DataDriver';
import { ExpectExtend } from './module/assert/ExpectExtend';
import { OhReport } from './module/report/OhReport';
import { SysTestKit } from './module/kit/SysTestKit';
import { DataDriverAttr, DataDriverData, DataDriverSuite, TestcaseSummaryIF, SendData, TestcaseRet, ItItemIF } from './interface';
import { AnyType } from './module/types/common';
import { ConfigService } from './module/config/configService';
import hilog from '@ohos.hilog';

let domain: int = 0x0000;
let tag: string = 'testTag';

export class Hypium {
  public static context = new Map<string, AnyType>();

  static parseValue(value: jsonx.JsonElement) {
    if (value.tryAsString()) {
      return value.tryAsString()
    } else if (value.tryAsInteger()) {
      return value.tryAsInteger()
    } else if (value.tryAsDouble()) {
      return value.tryAsDouble()
    } else if (value.tryAsBoolean()) {
      return value.tryAsBoolean()
    } else if (value.tryAsArray()) {
      const valueArray = value.tryAsArray() as Array<jsonx.JsonElement>
      const parsedValue: Array<object | undefined | null> = []
      for (const val of valueArray) {
        parsedValue.push(Hypium.parseValue(val))
      }
      return parsedValue;
    } else {
      try {
        return Hypium.parseParams(value)
      } catch (error) {
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

  static parseSuite(item: jsonx.JsonElement): DataDriverSuite {
    let describe: string[] | undefined = []
    if (item.tryGetElement("describe") !== undefined) {
      const describeValue = item.getArray("describe")
      for (const describeItem of describeValue) {
        const describeName = describeItem.asString()
        console.log("describeName" + describeName)
        describe?.push(describeName)
      }
    } else {
      describe = undefined
    }

    let params: Record<string, AnyType> = {};
    let paramsArray: Record<string, AnyType>[] = []
    if (item.tryGetElement("params") !== undefined) {
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
    } else {
      params = {}
    }

    let items: DataDriverSuite[] | undefined = [];
    if (item.tryGetElement("items") !== undefined) {
      const itemsValue = item.getArray("items");
      for (const childItem of itemsValue) {
        const parsedChild = Hypium.parseSuite(childItem);
        items?.push(parsedChild);
      }
    } else {
      items = undefined;
    }

    let stress: Double | undefined = 1;
    if (item.tryGetElement("stress") !== undefined) {
      stress = item.getElement("stress").asDouble()
    }

    let it: string | undefined = " ";
    if (item.tryGetElement("it") !== undefined) {
      it = item.getElement("it").asString();
    } else {
      it = undefined;
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
    let suitesValue: Array<jsonx.JsonElement> | undefined = [];
    if (jsonData.tryGetElement("suites") !== undefined) {
      suitesValue = jsonData.getArray("suites");
    } else {
      suitesValue = undefined;
      throw Error('suites is null');
    }
    const dataDriverSuite: DataDriverSuite[] = []
    for (const item of suitesValue) {
      const parseSuite = Hypium.parseSuite(item);
      dataDriverSuite.push(parseSuite);
    }
    const data: DataDriverData = {
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

  static hypiumTest(
    abilityDelegator: abilityDelegatorRegistry.AbilityDelegator,
    abilityDelegatorArguments: abilityDelegatorRegistry.AbilityDelegatorArgs,
    testsuite: () => void
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
        const testParameters = configService.translateParams(abilityDelegatorArguments.parameters);
        console.info(`${TAG}parameters:${JSON.stringify(testParameters)}`);
        configService.setConfig(testParameters);
      }
      testsuite();
      core.execute(abilityDelegator);
    }
  }

  static async hypiumInitEAWorkers(
    abilityDelegator: abilityDelegatorRegistry.AbilityDelegator,
    abilityDelegatorArguments: abilityDelegatorRegistry.AbilityDelegatorArgs,
    workerNum: int,
    testsuite: () => void
  ) {
    let workerPromiseArray: Promise<TestcaseSummaryIF>[] = [];
    let startTime = SysTestKit.getRealTime();
    Core.getInstance();
    for (let i = 0; i < workerNum; i++) {
      const workerPromise: Promise<TestcaseSummaryIF> = Hypium.createEAWorkerPromise(i, testsuite, abilityDelegator,
        abilityDelegatorArguments) as Promise<TestcaseSummaryIF>;
      workerPromiseArray.push(workerPromise);
    }
    const ret : TestcaseRet = {
      total: 0,
      failure: 0,
      error: 0,
      pass: 0,
      ignore: 0,
      duration: 0
    };
    Promise.all(workerPromiseArray).then(async (items: Array<TestcaseSummaryIF>) => {
      let allItemList = new Array<ItItemIF>();
      Hypium.handleWorkerTestResult(ret, allItemList, items);
      const retResult : TestcaseRet = {
        total: 0,
        failure: 0,
        error: 0,
        pass: 0,
        ignore: 0,
        duration: 0
      };
      Hypium.configWorkerItTestResult(retResult, allItemList);
      Hypium.printWorkerTestResult(abilityDelegator, allItemList);
      let endTime = SysTestKit.getRealTime();
      const taskConsuming = endTime - startTime;
      const message =
        `\n${PrintTag.OHOS_REPORT_ALL_RESULT}: stream=Test run: runTimes: ${ret.total},total: ${retResult.total}, Failure: ${retResult.failure}, Error: ${retResult.error}, Pass: ${retResult.pass}, Ignore: ${retResult.ignore}` +
        `\n${PrintTag.OHOS_REPORT_ALL_CODE}: ${retResult.failure > 0 || retResult.error > 0 ? -1 : 0}` +
        `\n${PrintTag.OHOS_REPORT_ALL_STATUS}: taskconsuming=${taskConsuming > 0 ? taskConsuming : ret.duration}`;
      abilityDelegator.printSync(message);
      console.info(`${TAG}, [end] you worker test`);
      abilityDelegator.finishTest('you worker test finished!!!', 0, () => {});
    }).catch((e) => {
      console.info(`${TAG}, [end] error you worker test, ${e}`);
      abilityDelegator.finishTest('you worker test error finished!!!', 0, () => {});
    }).finally(() => {
      console.info(`${TAG}, all promise finally end`);
    });
  }

  static createEAWorkerPromise(i: int, testsuite: ()=> void, abilityDelegator: abilityDelegatorRegistry.AbilityDelegator,
    abilityDelegatorArguments: abilityDelegatorRegistry.AbilityDelegatorArgs): Promise<TestcaseSummaryIF> {
    const workerPromise: Promise<TestcaseSummaryIF> = new Promise<TestcaseSummaryIF>((resolve, reject) => {
      let worker = new EAWorker(`EAWork_${i}`);
      worker.start();
      const currentCB = (msg: concurrency.Message) => {
        let data = msg.getObject() as SendData;
        let currentThreadName = data.currentThreadName;
        hilog.info(domain, tag, 'receview data from ' + currentThreadName +" " + JSON.stringify(data))
        worker.quit();
        let flag: boolean = worker.isAlive();
        resolve(data.summary);
      }
      let msgHandler = new concurrency.MessageHandler(currentCB, worker);
      worker.postTask(()=> {
        let core = new Core();
        core.msgHandle = msgHandler;
        Core.instance!.set(core);
        hilog.info(domain, tag, 'Run test in Worker ' + worker.getName())
        Hypium.hypiumTest(abilityDelegator, abilityDelegatorArguments, testsuite);
      })
      worker.setUncaughtExceptionHandler((e: Error) => {
        hilog.error(domain, tag, 'worker error: ' + JSON.stringify(e));
        try {
          worker.quit();
        } catch (quitError) {
          hilog.error(domain, tag, 'Failed to quit worker: ' + JSON.stringify(quitError));
        }
        reject(e);
        });
    });
    return workerPromise;
  }

  static handleWorkerTestResult(ret: TestcaseRet, allItemList: Array<ItItemIF>, items: Array<TestcaseSummaryIF>) {
    for (const item of items) {
      ret.total += item.total;
      ret.failure += item.failure;
      ret.error += item.error;
      ret.pass += item.pass;
      ret.ignore += item.ignore;
      ret.duration += item.duration;
      Hypium.handleItResult(allItemList, item.itItemList);
    }
  }

  static handleItResult(allItemList: Array<ItItemIF>, itItemList: Array<ItItemIF>) {
    for (const itItem of itItemList) {
      const description = itItem.description;
      const result = itItem.result;
      const currentThreadName = itItem.currentThreadName;
      const item = allItemList.find((it: ItItemIF) => (it.description === description));
      if (item) {
        if (item.result === 0 || (item.result !== 0 && result === 0)) {
          item.result = result;
          item.currentThreadName = currentThreadName;
        }
      } else {
        let it: ItItemIF = {
          description: description,
          currentThreadName: currentThreadName,
          result: result
        };
        allItemList.push(it);
      }
    }
  }

  static configWorkerItTestResult(retResult: TestcaseRet, allItemList: Array<ItItemIF>) {
    for (const item of allItemList) {
      const description = item.description;
      const result = item.result;
      console.info(`${TAG}, description, ${description}, result,${result}`);
      retResult.total ++;
      if (result === 0) {
        retResult.pass++;
      } else if (result === -1) {
        retResult.error++;
      } else if (result === -2) {
        retResult.failure++;
      } else {
        retResult.ignore++;
      }
    }
  }

  static printWorkerTestResult(abilityDelegator: abilityDelegatorRegistry.AbilityDelegator, allItemList:Array<ItItemIF>) {
    let index = 1;
    for (const itemValue of allItemList) {
      const description = itemValue.description;
      const result = itemValue.result;
      const itArray = description.split('#');
      let des = 'undefined';
      let itName = 'undefined';
      if (itArray.length > 1) {
        des = itArray[0];
        itName = itArray[1];
      } else if (itArray.length === 1) {
        des = itArray[0];
        itName = itArray[0];
      }
      let msg = `\n${PrintTag.OHOS_REPORT_WORKER_STATUS}: class=${des}`;
      msg += `\n${PrintTag.OHOS_REPORT_WORKER_STATUS}: test=${itName}`;
      msg += `\n${PrintTag.OHOS_REPORT_WORKER_STATUS}: current=${index}`;
      msg += `\n${PrintTag.OHOS_REPORT_WORKER_STATUS}: CODE=${result}`;
      abilityDelegator.printSync(msg);
      index++;
    }
  }
}
