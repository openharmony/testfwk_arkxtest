/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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

function exportUiTestLifeCycleMethods() {
  globalThis.setupUiTestEnvironment = async function () {
    let uitest = globalThis.requireInternal('uitest')
    if (uitest.uitestSetupDone === true) {
      return
    }
    // check 'persist.ace.testmode.enabled' property
    let parameter = globalThis.requireNapi('systemparameter')
    if (parameter == null) {
      console.error('UiTestKit_exporter: Failed to require systemparameter napi module!')
      return
    }
    if (parameter.getSync('persist.ace.testmode.enabled', '0') !== '1') {
      console.warn('UiTestKit_exporter: systemParameter "persist.ace.testmode.enabled" is not set!')
      return
    }
    let registry = globalThis.requireNapi('application.abilityDelegatorRegistry')
    if (registry == null) {
      console.error('UiTestKit_exporter: Failed to require AbilityDelegator napi module')
      return
    }
    // startup server_daemon
    let delegator = registry.getAbilityDelegator()
    if (delegator == null) {
      console.warn('UiTestKit_exporter: Cannot get AbilityDelegator, uitest_daemon need to be pre-started')
    } else {
      console.info('UiTestKit_lifecycle: Begin executing shell command to start server-daemon')
      try {
        let result = await delegator.executeShellCommand('uitest start-daemon 0123456789', 1)
        console.info(`UiTestKit_lifecycle: Start server-daemon finished: ${JSON.stringify(result)}`)
      } catch (error) {
        console.error(`UiTestKit_lifecycle: Start server-daemon failed: ${JSON.stringify(error)}`)
      }
    }
    uitest.setup('0123456789')
    uitest.uitestSetupDone = true
  }

  globalThis.teardownUiTestEnvironment = function () {
    if (uitest.uitestSetupDone === true) {
      console.info('UiTestKit_lifecycle, disposal uitest')
      uitestMod.teardown()
    }
  }
}

function requireUiTest() {
  console.info('UiTestKit_exporter: Start running uitest_exporter')
  exportUiTestLifeCycleMethods()
  console.info('UiTestKit_exporter: End running uitest_exporter')
  return globalThis.requireInternal('uitest')
}

export default requireUiTest()
