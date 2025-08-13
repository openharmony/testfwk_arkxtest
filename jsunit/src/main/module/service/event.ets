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
import { Core } from '../../core';
import { OhReport } from '../report/OhReport';
import { ReportService } from './ReportService';
import { EventAttrIF } from '../../interface';
class SpecEvent {
  public id: string;
  private eventMonitors: Array<ReportService>;
  private eventOhMonitors: Array<OhReport>;
  constructor(attr: EventAttrIF) {
    this.id = attr.id;
    this.eventMonitors = new Array<ReportService>();
    this.eventOhMonitors = new Array<OhReport>();
  }

  subscribeEvent(service: ReportService) {
    this.eventMonitors.push(service);
  }

  subscribeEvent(service: OhReport) {
    this.eventOhMonitors.push(service);
  }

  async specStart() {
    for (const monitor of this.eventMonitors) {
      monitor.specStart();
    }
    for (const ohMonitor of this.eventOhMonitors) {
      await ohMonitor.specStart();
    }
  }

  async specDone() {
    for (const monitor of this.eventMonitors) {
      monitor.specDone();
    }
    for (const ohMonitor of this.eventOhMonitors) {
      await ohMonitor.specDone();
    }
  }
}

class SuiteEvent {
  public id: string;
  private eventMonitors: Array<ReportService>;
  private eventOhMonitors: Array<OhReport>;
  constructor(attr: EventAttrIF) {
    this.id = attr.id;
    this.eventMonitors = new Array<ReportService>();
    this.eventOhMonitors = new Array<OhReport>();
  }

  subscribeEvent(service: ReportService) {
    this.eventMonitors.push(service);
  }

  subscribeEvent(service: OhReport) {
    this.eventOhMonitors.push(service);
  }

  async suiteStart() {
    for (const monitor of this.eventMonitors) {
      monitor.suiteStart();
    }
    for (const ohMonitor of this.eventOhMonitors) {
      await ohMonitor.suiteStart();
    }
  }

  async suiteDone() {
    for (const monitor of this.eventMonitors) {
      monitor.suiteDone();
    }
    for (const ohMonitor of this.eventOhMonitors) {
      await ohMonitor.suiteDone();
    }
  }
}

class TaskEvent {
  public id: string;
  private eventMonitors: Array<ReportService>;
  private eventOhMonitors: Array<OhReport>;
  constructor(attr: EventAttrIF) {
    this.id = attr.id;
    this.eventMonitors = new Array<ReportService>();
    this.eventOhMonitors = new Array<OhReport>();
  }

  subscribeEvent(service: ReportService) {
    this.eventMonitors.push(service);
  }

  subscribeEvent(service: OhReport) {
    this.eventOhMonitors.push(service);
  }

  async taskStart() {
    for (const monitor of this.eventMonitors) {
      monitor.taskStart();
    }
    for (const ohMonitor of this.eventOhMonitors) {
      ohMonitor.taskStart();
    }
  }

  async taskDone() {
    for (const monitor of this.eventMonitors) {
      monitor.taskDone();
    }
    for (const ohMonitor of this.eventOhMonitors) {
      await ohMonitor.taskDone();
    }
  }

  incorrectFormat() {
    for (const monitor of this.eventMonitors) {
      monitor.incorrectFormat();
    }
    for (const monitor of this.eventOhMonitors) {
      monitor.incorrectFormat();
    }
  }

  incorrectTestSuiteFormat() {
    for (const monitor of this.eventMonitors) {
      monitor.incorrectTestSuiteFormat();
    }
    for (const monitor of this.eventOhMonitors) {
      monitor.incorrectTestSuiteFormat();
    }
  }
}

export { SpecEvent, TaskEvent, SuiteEvent };
