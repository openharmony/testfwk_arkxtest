/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

function SpecEvent(attr) {
    this.id = attr.id;
    this.coreContext = attr.coreContext;
    this.eventMonitors = [];
}

SpecEvent.prototype.subscribeEvent = function (service) {
    this.eventMonitors.push(service);
};

SpecEvent.prototype.specStart = function () {
    for (let _mi = 0; _mi < this.eventMonitors.length; _mi++) {
        this.eventMonitors[_mi].specStart();
    }
};

SpecEvent.prototype.specDone = function () {
    for (let _mi = 0; _mi < this.eventMonitors.length; _mi++) {
        this.eventMonitors[_mi].specDone();
    }
};

function SuiteEvent(attr) {
    this.id = attr.id;
    this.coreContext = attr.coreContext;
    this.eventMonitors = [];
}

SuiteEvent.prototype.subscribeEvent = function (service) {
    this.eventMonitors.push(service);
};

SuiteEvent.prototype.suiteStart = function () {
    for (let _mi = 0; _mi < this.eventMonitors.length; _mi++) {
        this.eventMonitors[_mi].suiteStart();
    }
};

SuiteEvent.prototype.suiteDone = function () {
    for (let _mi = 0; _mi < this.eventMonitors.length; _mi++) {
        this.eventMonitors[_mi].suiteDone();
    }
};

function TaskEvent(attr) {
    this.id = attr.id;
    this.coreContext = attr.coreContext;
    this.eventMonitors = [];
}

TaskEvent.prototype.subscribeEvent = function (service) {
    this.eventMonitors.push(service);
};

TaskEvent.prototype.taskStart = function () {
    for (let _mi = 0; _mi < this.eventMonitors.length; _mi++) {
        this.eventMonitors[_mi].taskStart();
    }
};

TaskEvent.prototype.taskDone = function () {
    for (let _mi = 0; _mi < this.eventMonitors.length; _mi++) {
        this.eventMonitors[_mi].taskDone();
    }
};

export { SpecEvent, TaskEvent, SuiteEvent };
