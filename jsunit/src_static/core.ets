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
import { ConfigService } from './module/config/configService';
import DataDriver from './module/config/DataDriver';
import { SuiteService } from './module/service/SuiteService';
import { SpecService } from './module/service/SpecService';
import { ExpectService, AssertMatcher } from './module/service/ExpectService';
import { ReportService } from './module/service/ReportService';
import { OhReport } from './module/report/OhReport';
import { ExpectExtend } from './module/assert/ExpectExtend';
import { SpecEvent, TaskEvent, SuiteEvent } from './module/service/event';
import { ApiIF } from './interface';
import { AnyType, ItfnType } from './module/types/common';
/**
 * core service for execute testcase.
 */
type ServiceType = SuiteService
  | SpecService
  | ConfigService
  | ExpectService
  | ReportService
  | ExpectExtend
  | OhReport
  | DataDriver;
type EventType = SuiteEvent | SpecEvent | TaskEvent;
class Core {
  public static instance: Core | null = null;
  public static getInstance() {
    if (!Core.instance) {
      Core.instance = new Core();
    }
    return Core.instance;
  }

  public services: Map<string, Map<string, ServiceType>>;
  public events: Map<string, Map<string, EventType>>;
  public describe: (desc: string, func: () => void) => Promise<undefined>;
  public xdescribe: (desc: string, func: () => void, reason: string) => Promise<void>;
  public beforeItSpecified: (itDescs: string | string[], func: HookFuncType) => void;
  public afterItSpecified: (itDescs: string | string[], func: HookFuncType) => void;
  public beforeAll: (func: HookFuncType) => void;
  public beforeEach: (func: HookFuncType) => void;
  public afterAll: (func: HookFuncType) => void;
  public afterEach: (func: HookFuncType) => void;
  public it: (desc: string, filter: int, func: ItfnType) => void;
  public xit: (desc: string, filter: int, func: ItfnType, reason: string) => void;
  public expect: ((actualValue?: AnyType) => AssertMatcher) | null;
  public specEvent: SpecEvent;
  public suiteEvent: SuiteEvent;
  public taskEvent: TaskEvent;
  public sService: SuiteService | null;
  constructor() {
    this.services = new Map<string, Map<string, ServiceType>>();
    this.events = new Map<string, Map<string, EventType>>();
    this.describe = (desc: string, func: () => void): Promise<undefined> => {
      return Promise.resolve(undefined);
    };
    this.xdescribe = (desc: string, func: () => void, reason: string) => {
      return Promise.resolve();
    };
    this.beforeItSpecified = (itDescs: string | string[], func: (() => void) | (() => Promise<void>)) => {};
    this.afterItSpecified = (itDescs: string | string[], func: (() => void) | (() => Promise<void>)) => {};
    this.beforeAll = (func: (() => void) | (() => Promise<void>)) => {};
    this.beforeEach = (func: (() => void) | (() => Promise<void>)) => {};
    this.afterAll = (func: (() => void) | (() => Promise<void>)) => {};
    this.afterEach = (func: (() => void) | (() => Promise<void>)) => {};
    this.it = (desc: string, filter: int, func: ItfnType) => {};
    this.xit = (desc: string, filter: int, func: ItfnType, reason: string) => {};
    this.expect = null;
    this.specEvent = new SpecEvent({ id: '' });
    this.suiteEvent = new SuiteEvent({ id: '' });
    this.taskEvent = new TaskEvent({ id: '' });
    this.sService = null;
  }

  addService(name: string, service: ServiceType) {
    let serviceMap = this.services.get(name);
    if (!serviceMap) {
      serviceMap = new Map<string, ServiceType>();
      this.services.set(name, serviceMap);
    }
    serviceMap.set(service.id, service);
  }

  getDefaultService(name: string): ServiceType | null {
    const serviceMap = this.services.get(name);
    if (serviceMap) {
      return (serviceMap as Map<string, ServiceType>).get('default') as ServiceType;
    } else {
      return null;
    }
  }

  getServices(name: string) {
    return this.services.get(name);
  }

  registerEvent(serviceName: string, event: EventType) {
    let eventMap = this.events.get(serviceName);
    if (!eventMap) {
      eventMap = new Map<string, EventType>();
      this.events.set(serviceName, eventMap);
    }
    eventMap.set(event.id, event);
  }

  unRegisterEvent(serviceName: string, eventID: string) {
    const eventObj = this.events.get(serviceName);
    if (eventObj) {
      eventObj.delete(eventID);
    }
  }

  subscribeEvent(serviceName: string, serviceObj: ReportService) {
    const eventMap = this.events.get(serviceName);
    if (eventMap) {
      eventMap.forEach((value: EventType) => {
        if (value instanceof SuiteEvent) {
          (value as SuiteEvent).subscribeEvent(serviceObj);
        } else if (value instanceof SpecEvent) {
          (value as SpecEvent).subscribeEvent(serviceObj);
        } else if (value instanceof TaskEvent) {
          (value as TaskEvent).subscribeEvent(serviceObj);
        }
      });
    }
  }

  subscribeEvent(serviceName: string, serviceObj: OhReport) {
    const eventMap = this.events.get(serviceName);
    if (eventMap) {
      eventMap.forEach((value: EventType) => {
        if (value instanceof SuiteEvent) {
          (value as SuiteEvent).subscribeEvent(serviceObj);
        } else if (value instanceof SpecEvent) {
          (value as SpecEvent).subscribeEvent(serviceObj);
        } else if (value instanceof TaskEvent) {
          (value as TaskEvent).subscribeEvent(serviceObj);
        }
      });
    }
  }

  async fireEvents(serviceName: string, eventName: string) {
    const eventMap = this.events.get(serviceName);
    if (eventMap) {
      for (const event of eventMap.values()) {
        switch (eventName) {
          case 'specStart':
            this.specEvent = event as SpecEvent;
            await this.specEvent.specStart();
            break;
          case 'specDone':
            this.specEvent = event as SpecEvent;
            await this.specEvent.specDone();
            break;
          case 'suiteStart':
            this.suiteEvent = event as SuiteEvent;
            await this.suiteEvent.suiteStart();
            break;
          case 'suiteDone':
            this.suiteEvent = event as SuiteEvent;
            await this.suiteEvent.suiteDone();
            break;
          case 'taskStart':
            this.taskEvent = event as TaskEvent;
            await this.taskEvent.taskStart();
            break;
          case 'taskDone':
            this.taskEvent = event as TaskEvent;
            await this.taskEvent.taskDone();
            break;
          case 'incorrectFormat':
            (event as TaskEvent).incorrectFormat();
            break;
          case 'incorrectTestSuiteFormat':
            (event as TaskEvent).incorrectTestSuiteFormat();
            break;
        }
      }
    }
  }

  addToGlobal(apis: ApiIF) {
    const describe = apis.describe;
    if (describe) {
      this.describe = describe;
    }
    const xdescribe = apis.xdescribe;
    if (xdescribe) {
      this.xdescribe = xdescribe;
    }
    const beforeItSpecified = apis.beforeItSpecified;
    if (beforeItSpecified) {
      this.beforeItSpecified = beforeItSpecified;
    }
    const afterItSpecified = apis.afterItSpecified;
    if (afterItSpecified) {
      this.afterItSpecified = afterItSpecified;
    }
    const beforeAll = apis.beforeAll;
    if (beforeAll) {
      this.beforeAll = beforeAll;
    }
    const beforeEach = apis.beforeEach;
    if (beforeEach) {
      this.beforeEach = beforeEach;
    }
    const afterAll = apis.afterAll;
    if (afterAll) {
      this.afterAll = afterAll;
    }
    const afterEach = apis.afterEach;
    if (afterEach) {
      this.afterEach = afterEach;
    }
    const it = apis.it;
    if (it) {
      this.it = it;
    }
    const xit = apis.xit;
    if (xit) {
      this.xit = xit;
    }
    const expect = apis.expect;
    if (expect) {
      this.expect = expect as (actualValue?: AnyType) => AssertMatcher;
    }
  }

  init() {
    this.addService('suite', new SuiteService({ id: 'default' }));
    this.addService('spec', new SpecService({ id: 'default' }));
    this.addService('expect', new ExpectService({ id: 'default' }));
    this.addService('report', new ReportService({ id: 'default' }));
    this.addService('config', new ConfigService({ id: 'default' }));
    this.registerEvent('task', new TaskEvent({ id: 'default', coreContext: this }));
    this.registerEvent('suite', new SuiteEvent({ id: 'default', coreContext: this }));
    this.registerEvent('spec', new SpecEvent({ id: 'default', coreContext: this }));
    const report = this.getDefaultService('report');
    if (report !== null) {
      this.subscribeEvent('spec', report as ReportService);
      this.subscribeEvent('suite', report as ReportService);
      this.subscribeEvent('task', report as ReportService);
    }

    const context = this;
    for (const serviceMap of this.services.values()) {
      for (const service of serviceMap.values()) {
        let apis: ApiIF = {};
        if (service instanceof SuiteService) {
          const typeService = service as SuiteService;
          typeService.init(context);
          apis = typeService.apis();
        } else if (service instanceof SpecService) {
          const typeService = service as SpecService;
          typeService.init(context);
          apis = typeService.apis();
        } else if (service instanceof ConfigService) {
          const typeService = service as ConfigService;
          typeService.init(context);
        } else if (service instanceof ExpectService) {
          const typeService = service as ExpectService;
          typeService.init(context);
          apis = typeService.apis();
        } else if (service instanceof ReportService) {
          const typeService = service as ReportService;
          typeService.init(context);
        } else if (service instanceof ExpectExtend) {
          const typeService = service as ExpectExtend;
          typeService.init(context);
          apis = typeService.apis();
        } else if (service instanceof OhReport) {
          const typeService = service as OhReport;
          typeService.init(context);
        }
        this.addToGlobal(apis);
      }
    }
  }

  async execute(abilityDelegator: abilityDelegatorRegistry.AbilityDelegator) {
    const suiteService = this.getDefaultService('suite');
    const configService = this.getDefaultService('config');
    if (suiteService !== null && configService !== null) {
      this.sService = suiteService as SuiteService;
      const cService = configService as ConfigService;
      const service = this.sService;
      if (service !== null) {
        if (cService.dryRun === 'true') {
          await service.dryRun(abilityDelegator);
        } else {
          setTimeout(() => {
            if (service !== null) {
              service.execute();
            }
          }, 0);
        }
      }
    }
  }
}
export { Core };
