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

function requireModule(name) {
  let mod = globalThis.requireNapi(name)
  if (mod) {
    return mod
  }
  mod = globalThis.ohosplugin
  name.split('.').foreach(element => {
    if (mod) {
      mod = mod[element]
    }
  })
  if (mod) {
    return mod
  }
  mod = globalThis.systemplugin
  name.split('.').foreach(element => {
    if (mod) {
      mod = mod[element]
    }
  })
  return mod
}

async function scheduleConnectionAsync(uitest) {
  let procInfo = requireModule('process')
  if (procInfo == null) {
    console.error('UiTestKit_exporter: Failed to require process napi module')
    return
  }
  let registry = requireModule('application.abilityDelegatorRegistry')
  if (registry == null) {
    console.error('UiTestKit_exporter: Failed to require AbilityDelegator napi module')
    return
  }
  let delegator = registry.getAbilityDelegator()
  if (delegator == null) {
    console.warn('UiTestKit_exporter: Cannot get AbilityDelegator, uitest_daemon need to be pre-started')
    uitest.scheduleEstablishConnection('default')
  } else {
    let appContext = delegator.getAppContext()
    let connToken = `${appContext.applicationInfo.name}@${procInfo.pid}@${procInfo.uid}@${appContext.area}`
    console.info(`UiTestKit_exporter: ScheduleProbeAndEstablishConnection, token=${connToken}`)
    uitest.scheduleEstablishConnection(connToken)
    console.info('UiTestKit_exporter: Begin executing shell command to start server-daemon')
    delegator.executeShellCommand(`uitest start-daemon ${connToken}`, 3).then((value) => {
      console.info(`UiTestKit_exporter: Start server-daemon finished: ${JSON.stringify(value)}`)
    }).catch((error) => {
      console.error(`UiTestKit_exporter: Start server-daemon failed: ${JSON.stringify(error)}`)
    })
  }
}

function loadAndSetupUiTest() {
  let uitest = globalThis.requireInternal('UiTest')
  if (uitest.uitestSetupCalled === true) {
    return
  }
  uitest.uitestSetupCalled = true
  // check 'persist.ace.testmode.enabled' property
  let parameter = requireModule('systemparameter')
  if (parameter == null) {
    console.error('UiTestKit_exporter: Failed to require systemparameter napi module!')
    return
  }
  if (parameter.getSync('persist.ace.testmode.enabled', '0') !== '1') {
    console.warn('UiTestKit_exporter: systemParameter "persist.ace.testmode.enabled" is not set!')
  }
  // schedule startup server_daemon and establish connection
  scheduleConnectionAsync(uitest).catch((error) => {
    console.error(`UiTestKit_exporter: ScheduleConnectionAsync failed: ${JSON.stringify(error)}`)
  })
  return uitest
}

export default loadAndSetupUiTest()
