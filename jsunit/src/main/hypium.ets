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

export class Hypium {
  public static context = new Map<string, AnyType>();
  static setData(data: DataDriverData) {
    const core = Core.getInstance();
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

    // 引入1.1的hypium
    const bundleName = abilityDelegatorArguments.bundleName;
    const parameters = abilityDelegatorArguments.parameters;
    const moduleName = parameters['-m'] || 'entry_test';
    const loadPath = `@normalized:N&${moduleName}&${bundleName}&@ohos/hypium/index&1.0.0`;
    const dynamicModule = ESValue.load(loadPath);
    let hypiumDynamic: ESValue | null = null;
    let coreDynamic: ESValue | null = null;
    if (dynamicModule) {
      hypiumDynamic = (dynamicModule as ESValue).getProperty('Hypium');
      coreDynamic = (hypiumDynamic as ESValue).invokeMethod(
        'init',
        ESObject.wrap(abilityDelegator),
        ESObject.wrap(abilityDelegatorArguments)
      );
    }
    testsuite();
    let staticTotal = 0;
    let specService: SpecService | null = null;
    if (core) {
      specService = (core as Core).getDefaultService('spec') as SpecService;
      staticTotal = specService.totalTest;
    }
    let dynamicTotalValue = 0;
    if (hypiumDynamic) {
      const dynamicTotal = (hypiumDynamic as ESValue).invokeMethod(
        'updateTotalTest',
        coreDynamic,
        ESObject.wrap(staticTotal)
      );
      dynamicTotalValue = dynamicTotal.toNumber().toInt();
    }
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
      setTimeout(() => {
        (hypiumDynamic as ESValue).invokeMethod(
          'execute',
          coreDynamic,
          ESObject.wrap(abilityDelegator),
          ESObject.wrap(summary)
        );
      }, 0);
    }
  }
}
