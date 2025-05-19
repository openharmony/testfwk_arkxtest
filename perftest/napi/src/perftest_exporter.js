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

function requireModule(name) {
  let mod = globalThis.requireNapi(name);
  if (mod) {
    return mod;
  }
  mod = globalThis.ohosplugin;
  name.split('.').foreach(element => {
    if (mod) {
      mod = mod[element];
    }
  });
  if (mod) {
    return mod;
  }
  mod = globalThis.systemplugin;
  name.split('.').foreach(element => {
    if (mod) {
      mod = mod[element];
    }
  });
  return mod;
}

function delayMs(ms) {
  const startTime = Date.now();
  while (Date.now() - startTime < ms) {}
}

async function scheduleConnectionAsync(perftest) {
  let procInfo = requireModule('process');
  if (procInfo == null) {
    console.error('C03120 PerfTestKit_exporter: Failed to require process napi module');
    return;
  }
  let registry = requireModule('application.abilityDelegatorRegistry');
  if (registry == null) {
    console.error('C03120 PerfTestKit_exporter: Failed to require AbilityDelegator napi module');
    return;
  }
  let delegator = registry.getAbilityDelegator();
  if (delegator == null) {
    console.warn('C03120 PerfTestKit_exporter: Cannot get AbilityDelegator, perftest_daemon need to be pre-started');
    return;
  }
  let appContext = delegator.getAppContext();
  let connToken = `${appContext.applicationInfo.name}@${procInfo.pid}@${procInfo.uid}@${appContext.area}`;
  console.info(`C03120 PerfTestKit_exporter: ScheduleProbeAndEstablishConnection, token=${connToken}`);
  perftest.scheduleEstablishConnection(connToken);
  console.info(`C03120 PerfTestKit_exporter command: perftest start-daemon ${connToken}`);
  delegator.executeShellCommand(`perftest start-daemon ${connToken}`, 3).then((value) => {
    console.info(`C03120 PerfTestKit_exporter: Start perftest_daemon finished: ${JSON.stringify(value)}`);
  }).catch((error) => {
    console.error(`C03120 PerfTestKit_exporter: Start perftest_daemon failed: ${JSON.stringify(error)}`);
  });
}

function loadPerfTest() {
  let perftest = globalThis.requireInternal('test.PerfTest');
  if (perftest.perftestSetupCalled === true) {
    return;
  }
  perftest.perftestSetupCalled = true;
  // schedule startup server_daemon and establish connection
  scheduleConnectionAsync(perftest).catch((error) => {
    console.error(`C03120 PerfTestKit_exporter: ScheduleConnectionAsync failed: ${JSON.stringify(error)}`);
  });
  let delayTime = 500;
  delayMs(delayTime);
  return perftest;
}

export default loadPerfTest();
