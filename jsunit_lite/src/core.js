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

import {SuiteService, SpecService, ExpectService} from './service';
import {ConfigService} from './module/config/configService';
import {SpecEvent, TaskEvent, SuiteEvent} from './event';

const Core = (function () {
    let instance = null;

    function Core() {
        this.services = {
            suite: {},
            spec: {},
            config: {},
            expect: {},
            log: {},
            report: {}
        };
        this.events = {
            suite: {},
            spec: {},
            task: {}
        };
    }

    Core.getInstance = function () {
        if (!instance) {
            instance = new Core();
        }
        return instance;
    };

    Core.prototype.addService = function (name, service) {
        if (!this.services[name]) {
            this.services[name] = {};
        }
        this.services[name][service.id] = service;
    };

    Core.prototype.getDefaultService = function (name) {
        return this.services[name].default;
    };

    Core.prototype.getServices = function (name) {
        return this.services[name];
    };

    Core.prototype.registerEvent = function (serviceName, event) {
        if (!this.events[serviceName]) {
            this.events[serviceName] = {};
        }
        this.events[serviceName][event.id] = event;
    };

    Core.prototype.unRegisterEvent = function (serviceName, eventID) {
        const eventObj = this.events[serviceName];
        if (eventObj) {
            delete eventObj[eventID];
        }
    };

    Core.prototype.subscribeEvent = function (serviceName, serviceObj) {
        if (!serviceObj) { return }
        const eventObj = this.events[serviceName];
        if (eventObj) {
            for (const attr in eventObj) {
                if (Object.prototype.hasOwnProperty.call(eventObj, attr)) {
                    eventObj[attr].subscribeEvent(serviceObj);
                }
            }
        }
    };

    Core.prototype.fireEvents = function (serviceName, eventName) {
        const eventObj = this.events[serviceName];
        if (!eventObj) {
            return;
        }
        for (const attr in eventObj) {
            if (Object.prototype.hasOwnProperty.call(eventObj, attr)) {
                const ev = eventObj[attr];
                if (ev && typeof ev[eventName] === 'function') {
                    ev[eventName]();
                }
            }
        }
    };

    Core.prototype.addToGlobal = function (apis) {
        for (const api in apis) {
            if (Object.prototype.hasOwnProperty.call(apis, api)) {
                this[api] = apis[api];
            }
        }
    };

    Core.prototype.init = function () {
        if (!this.services.suite.default) {
            this.addService('suite', new SuiteService({id: 'default'}));
        }
        if (!this.services.spec.default) {
            this.addService('spec', new SpecService({id: 'default'}));
        }
        if (!this.services.expect.default) {
            this.addService('expect', new ExpectService({id: 'default'}));
        }
        if (!this.services.config.default) {
            this.addService('config', new ConfigService({id: 'default'}));
        }
        if (!this.events.task.default) {
            this.registerEvent('task', new TaskEvent({id: 'default', coreContext: this}));
        }
        if (!this.events.suite.default) {
            this.registerEvent('suite', new SuiteEvent({id: 'default', coreContext: this}));
        }
        if (!this.events.spec.default) {
            this.registerEvent('spec', new SpecEvent({id: 'default', coreContext: this}));
        }
        this.subscribeEvent('spec', this.getDefaultService('report'));
        this.subscribeEvent('suite', this.getDefaultService('report'));
        this.subscribeEvent('task', this.getDefaultService('report'));

        const context = this;
        for (const sname in this.services) {
            if (!Object.prototype.hasOwnProperty.call(this.services, sname)) { continue }
            const serviceObj = this.services[sname];
            for (const serviceID in serviceObj) {
                if (!Object.prototype.hasOwnProperty.call(serviceObj, serviceID)) { continue }
                const service = serviceObj[serviceID];
                service.init(context);
                if (typeof service.apis !== 'function') {
                    continue;
                }
                const apis = service.apis();
                if (apis) {
                    this.addToGlobal(apis);
                }
            }
        }
    };

    Core.prototype.execute = function () {
        const suiteService = this.getDefaultService('suite');
        suiteService.execute();
    };

    return Core;
})();

export default Core;
