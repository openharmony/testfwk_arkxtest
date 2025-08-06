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
import hilog from '@ohos.hilog';
import AbilityDelegatorRegistry from '@ohos.app.ability.abilityDelegatorRegistry';
import TestRunner from '@ohos.application.testRunner';
import Want from '@ohos.app.ability.Want';
import UIAbility from '@ohos.app.ability.UIAbility';
import { BusinessError } from '@ohos.base';

let tag: string = 'testTag';
const UNLOCK_ERROR = -3;

function onAbilityCreateCallback(ability: UIAbility) {
  hilog.info(0x0000, 'testTag', '%{public}s', 'onAbilityCreateCallback');
}

function addAbilityMonitorCallback(err: BusinessError | null) {
  if(err) {
    hilog.info(0x0000, 'testTag', 'addAbilityMonitorCallback : %{public}s', JSON.stringify(err as BusinessError));
  } else {
    hilog.info(0x0000, 'testTag', 'addAbilityMonitorCallback : %{public}s', "");
  }
}

export default class OpenHarmonyTestRunner implements TestRunner {
  constructor() {
  }

  onPrepare() {
    hilog.info(0x0000, 'testTag', '%{public}s', "OpenHarmonyTestRunner onPrepare");
  }

  onRun() {
    hilog.info(0x0000, 'testTag', '%{public}s', "OpenHarmonyTestRunner onRun run");
    const abilityDelegatorArguments: AbilityDelegatorRegistry.AbilityDelegatorArgs = AbilityDelegatorRegistry.getArguments();
    const abilityDelegator: AbilityDelegatorRegistry.AbilityDelegator = AbilityDelegatorRegistry.getAbilityDelegator();
    const bundleName = abilityDelegatorArguments.bundleName;
    const testAbilityName = 'TestAbility';
    const parameters: Record<string, string> = abilityDelegatorArguments.parameters;
    const moduleName = parameters['-m'];
    const lMonitor: AbilityDelegatorRegistry.AbilityMonitor = {
      abilityName: testAbilityName,
      onAbilityCreate: onAbilityCreateCallback,
      moduleName: moduleName
    };
    abilityDelegator.addAbilityMonitor(lMonitor, addAbilityMonitorCallback);
    startAbility(abilityDelegator, bundleName, testAbilityName).then((res) => {
      hilog.info(0x0000, 'testTag', '%{public}s', "OpenHarmonyTestRunner onRun end");
    })
  }
}

async function startAbility(abilityDelegator: AbilityDelegatorRegistry.AbilityDelegator, bundleName: string, abilityName: string): Promise<void> {
  try {
    const result = await abilityDelegator.executeShellCommand('aa start -b ${bundleName} -a ${abilityName}');
    if (result.stdResult.includes("error: unlock screen failed in developer mode")) {
      hilog.info(0x0000, 'testTag', '%{public}s', "startAbility failed ${result.stdResult}");
      abilityDelegator.finishTest("", UNLOCK_ERROR, () => {});
    }
  } catch (err: Error) {
    hilog.info(0x0000, 'testTag', '%{public}s', "startAbility error: ${err.message}");
  }

}