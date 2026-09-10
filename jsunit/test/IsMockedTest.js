/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

import { describe, it, expect } from '@ohos/hypium';
import { MockKit, when } from '@ohos/hypium';

export default function isMockedTest() {
    describe('IsMockedTest', function () {
        class SampleClass {
            greet(name) {
                return 'hello ' + name;
            }
            count = 0;
        }

        it('AC-1_isMocked_mockedMethod_returnsTrue', 0, function () {
            const mocker = new MockKit();
            const instance = new SampleClass();
            const stub = mocker.mockFunc(instance, instance.greet);
            when(stub)('world').afterReturn('mocked');
            expect(mocker.isMocked(instance, 'greet')).assertTrue();
            mocker.clear(instance);
        });

        it('AC-2_isMocked_afterClear_returnsFalse', 0, function () {
            const mocker = new MockKit();
            const instance = new SampleClass();
            const stub = mocker.mockFunc(instance, instance.greet);
            when(stub)('world').afterReturn('mocked');
            expect(mocker.isMocked(instance, 'greet')).assertTrue();
            mocker.clear(instance);
            expect(mocker.isMocked(instance, 'greet')).assertFalse();

            const mocker2 = new MockKit();
            const instance2 = new SampleClass();
            const stub2 = mocker2.mockFunc(instance2, instance2.greet);
            when(stub2)('world').afterReturn('mocked');
            mocker2.ignoreMock(instance2, 'greet');
            expect(mocker2.isMocked(instance2, 'greet')).assertFalse();
        });

        it('AC-3_isMocked_systemMethod_returnsFalse', 0, function () {
            const mocker = new MockKit();
            const consoleObj = console;
            expect(mocker.isMocked(consoleObj, 'log')).assertFalse();
        });

        it('AC-4_isMocked_neverMockedObject_returnsFalse', 0, function () {
            const mocker = new MockKit();
            const instance = new SampleClass();
            expect(mocker.isMocked(instance, 'greet')).assertFalse();
            expect(mocker.isMocked(instance, 'count')).assertFalse();
        });

        it('AC-5_isMocked_multipleMocks_returnsTrueThenFalseAfterClear', 0, function () {
            const mocker = new MockKit();
            const instance = new SampleClass();
            const stub1 = mocker.mockFunc(instance, instance.greet);
            when(stub1)('a').afterReturn('1');
            const stub2 = mocker.mockFunc(instance, instance.count);
            when(stub2)().afterReturn(99);
            expect(mocker.isMocked(instance, 'greet')).assertTrue();
            expect(mocker.isMocked(instance, 'count')).assertTrue();
            mocker.clear(instance);
            expect(mocker.isMocked(instance, 'greet')).assertFalse();
            expect(mocker.isMocked(instance, 'count')).assertFalse();
        });

        it('AC-6_isMocked_mockedProperty_returnsTrue', 0, function () {
            const mocker = new MockKit();
            const instance = new SampleClass();
            mocker.mockProperty(instance, 'count', 999);
            expect(mocker.isMocked(instance, 'count')).assertTrue();
            mocker.clear(instance);
        });

        it('AC-7_isMocked_afterPropertyClear_returnsFalse', 0, function () {
            const mocker = new MockKit();
            const instance = new SampleClass();
            mocker.mockProperty(instance, 'count', 999);
            expect(mocker.isMocked(instance, 'count')).assertTrue();
            mocker.clear(instance);
            expect(mocker.isMocked(instance, 'count')).assertFalse();

            const mocker2 = new MockKit();
            const instance2 = new SampleClass();
            mocker2.mockProperty(instance2, 'count', 999);
            expect(mocker2.isMocked(instance2, 'count')).assertTrue();
            mocker2.ignoreMock(instance2, 'count');
            expect(mocker2.isMocked(instance2, 'count')).assertFalse();
        });

        it('AC-8_isMocked_mockFuncWithoutWhen_returnsFalse', 0, function () {
            const mocker = new MockKit();
            const instance = new SampleClass();
            mocker.mockFunc(instance, instance.greet);
            expect(mocker.isMocked(instance, 'greet')).assertFalse();
            mocker.clear(instance);
        });

        it('AC-9_isMocked_emptyOrNullName_returnsFalse', 0, function () {
            const mocker = new MockKit();
            const instance = new SampleClass();
            expect(mocker.isMocked(instance, '')).assertFalse();
            expect(mocker.isMocked(instance, null)).assertFalse();
        });
    });
}
