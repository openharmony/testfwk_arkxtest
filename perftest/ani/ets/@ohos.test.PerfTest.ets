/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License,Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,software
 * distributed under the License is distributed on an "ASIS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
import abilityDelegatorRegistry from '@ohos.app.ability.abilityDelegatorRegistry';
import hilog from '@ohos.hilog'
import { Callback, BusinessError } from '@ohos.base';
let domain: int = 0x3120;
let tag: string = 'perftestTag';

let executeTime = 3;

export default namespace PerfTestInit {
  loadLibraryWithPermissionCheck("perftest_ani", "@ohos.test.PerfTest");
  export native function ScheduleEstablishConnection(token: String): boolean;
  export native function GetConnectionStat(): int;
}

export function loadAndSetUpPerfTest(): boolean {
    let connectedState = 1;
    hilog.info(domain, 'perftestTag', 'loadAndSetUpPerfTest');
    if (PerfTestInit.GetConnectionStat() == connectedState) {
        hilog.info(domain, 'perftestTag', "Perftest is already connected: "+ PerfTestInit.GetConnectionStat())
        return true;
    } else {
        let delegator = abilityDelegatorRegistry.getAbilityDelegator();
        try{
            if (delegator != null) {
                hilog.info(domain, 'perftestTag', 'PerfTest_exporter: Got AbilityDelegator');
                let result = PerfTestInit.ScheduleEstablishConnection("default");
                hilog.info(domain, 'perftestTag', "PerfTestInit.ScheduleEstablishConnection(): " + result)
                delegator.executeShellCommand(`perftest start-daemon default`, executeTime).then((value: NullishType) => {
                    hilog.info(domain, 'perftestTag', `PerfTest_exporter: Start server-daemon finished`);
                }).catch((error: Error) : void=> {
                    hilog.error(domain, 'perftestTag', "PerfTest_exporter: Start server-daemon failed:" + error)
                });
                return true;
            } else {
                hilog.error(domain, 'perftestTag', 'PerfTest_exporter: Cannot get AbilityDelegator, perftest_daemon need to be pre-started');
                return false;
            }
        } catch(error){
            hilog.error(domain, 'perftestTag', "loadAndSetUpPerfTest failed" + (error as Error).stack);
            return false;
        }
    }
}

enum PerfMetric {
    DURATION = 0,
    CPU_LOAD = 1,
    CPU_USAGE = 2,
    MEMORY_RSS = 3,
    MEMORY_PSS = 4,
    APP_START_RESPONSE_TIME = 5,
    APP_START_COMPLETE_TIME = 6,
    PAGE_SWITCH_COMPLETE_TIME = 7,
    LIST_SWIPE_FPS = 8
}
  
interface PerfTestStrategy {
    metrics: Array<PerfMetric>;
    actionCode: Callback<Callback<boolean>>;
    resetCode?: Callback<Callback<boolean>>;
    bundleName?: string;
    iterations?: int;
    timeout?: int;
}

interface PerfMeasureResult {
    metric: PerfMetric;
    roundValues: Array<double>;
    maximum: double;
    minimum: double;
    average: double;
}

class PerfMeasureResultInner implements PerfMeasureResult {
    metric: PerfMetric = PerfMetric.DURATION;
    roundValues: Array<double> = {};
    maximum: double = -1.0;
    minimum: double = -1.0;
    average: double = -1.0;
}

export class PerfTest {
    static {
        if (PerfTestInit.GetConnectionStat() == 0) {
            hilog.info(domain, 'perftestTag', 'On static loadAndSetUpPerfTest: ' + loadAndSetUpPerfTest());
        }
    }
    nativePerfTest: string = '';
    public constructor(perfTest: string) {
        if (this.nativePerfTest == '') {
            this.nativePerfTest = perfTest;
        }
    }
    native static create(perfTestStrategy: PerfTestStrategy): PerfTest;

    private native runSync(): boolean;
    run(): Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Error) => void) => {
        let promise1 = taskpool.execute((): boolean => this.runSync());
            promise1.then((e: NullishType) => {
                resolve(Promise.resolve());
            }, (err: Error): void => {
                let error = err as BusinessError<void>;
                reject(error);
            });
        });
        return promise;
    }

    native getMeasureResult(metric: PerfMetric): PerfMeasureResult;

    native destroy(): void;
}

class FinishCallback {
    finish: Callback<boolean> = (res: boolean) => {
        this.finishCallback(res);
    };

    native finishCallback(res: boolean): void;
}