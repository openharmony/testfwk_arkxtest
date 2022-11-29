# arkXtest

## Introduction
arkXtest, the automated test framework of OpenHarmony, consists of the JS unit test framework (JsUnit) and UI test framework (UiTest).

JsUnit provides APIs for writing test cases for system or application interfaces, executing unit test cases, and generating test reports.

UiTest provides simple and easy-to-use APIs for locating and operating UI components, helping users to develop automatic test scripts based on UI operations.

## Directory Structure

```
arkXtest 
  |-----jsunit  Unit test framework
  |-----uitest UI test framework
```
## Constraints
The initial APIs of this module are supported since API version 8. Newly added APIs will be marked with a superscript to indicate their earliest API version.

## JsUnit Features

| No.  | Feature    | Description                                                    |
| ---- | -------- | ------------------------------------------------------------ |
| 1    | Basic process support| Provides APIs for writing and executing test cases.                                    |
| 2    | Assertion library  | Provides APIs for checking whether the actual value of a test case is the same as the expected value.                        |
| 3    | Mock| Provides APIs for mocking functions to return the specified value or perform the specified action.|
| 4    | Data driving| Provides APIs for reusing a test script with different input data.|

### How to Use

####  Basic Process Support

Test cases use the common syntax in the industry. **describe** defines a test suite, and **it** specifies a test case.

| No.  | API        | Description                                                    |
| ---- | ---------- | ------------------------------------------------------------ |
| 1    | describe   | Defines a test suite. This API supports two parameters: test suite name and test suite function.      |
| 2    | beforeAll  | Presets an action, which is performed only once before all test cases of the test suite start. This API supports only one parameter: preset action function.|
| 3    | beforeEach | Presets an action, which is performed before each unit test case starts. The number of execution times is the same as the number of test cases defined by **it**. This API supports only one parameter: preset action function.|
| 4    | afterEach  | Presets a clear action, which is performed after each unit test case ends. The number of execution times is the same as the number of test cases defined by **it**. This API supports only one parameter: clear action function.|
| 5    | afterAll   | Presets a clear action, which is performed after all test cases of the test suite end. This API supports only one parameter: clear action function.|
| 6    | it         | Defines a test case. This API supports three parameters: test case name, filter parameter, and test case function.|
| 7    | expect     | Defines a variety of assertion methods, which are used to declare expected Boolean conditions.                            |

The sample code is as follows:

```javascript
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium'
import demo from '@ohos.bundle'

export default async function abilityTest() {
  describe('ActsAbilityTest', function () {
    it('String_assertContain_success', 0, function () {
      let a = 'abc'
      let b = 'b'
      expect(a).assertContain(b)
      expect(a).assertEqual(a)
    })
    it('getBundleInfo_0100', 0, async function () {
      const NAME1 = "com.example.MyApplicationStage"
      await demo.getBundleInfo(NAME1,
        demo.BundleFlag.GET_BUNDLE_WITH_ABILITIES | demo.BundleFlag.GET_BUNDLE_WITH_REQUESTED_PERMISSION)
        .then((value) => {
          console.info(value.appId)
        })
        .catch((err) => {
          console.info(err.code)
        })
    })
  })
}
```



####  Assertion Library

Available APIs:


| No.  | API              | Description                                                    |
| :--- | :--------------- | ------------------------------------------------------------ |
| 1    | assertClose      | Checks whether the proximity between the actual value and the expected value (0) is the expected value (1).|
| 2    | assertContain    | Checks whether the actual value contains the expected value.                      |
| 3    | assertEqual      | Checks whether the actual value is equal to the expected value.                     |
| 4    | assertFail       | Throws an error.                                              |
| 5    | assertFalse      | Check whether the actual value is **false**.                                |
| 6    | assertTrue       | Checks whether the actual value is **true**.                                 |
| 7    | assertInstanceOf | Checks whether the actual value is of the type specified by the expected value.                      |
| 8    | assertLarger     | Checks whether the actual value is greater than the expected value.                        |
| 9    | assertLess       | Checks whether the actual value is less than the expected value.                        |
| 10   | assertNull       | Checks whether the actual value is null.                                 |
| 11   | assertThrowError | Checks whether the error thrown by the actual value is the expected value.             |
| 12   | assertUndefined  | Checks whether the actual value is **undefined**.                            |

The sample code is as follows:

```javascript
import { describe, it, expect } from '@ohos/hypium'
export default async function abilityTest() {
  describe('assertClose', function () {
    it('assertBeClose success', 0, function () {
      let a = 100
      let b = 0.1
      expect(a).assertClose(99, b)
    })
    it('assertBeClose fail', 0, function () {
      let a = 100
      let b = 0.1
      expect(a).assertClose(1, b)
    })
    it('assertBeClose fail', 0, function () {
      let a = 100
      let b = 0.1
      expect(a).assertClose(null, b)
    })
    it('assertBeClose fail', 0, function () {
      expect(null).assertClose(null, 0)
    })
  })
}
```


#### Mock

##### Constraints

JsUnit provides the mock capability since npm [1.0.1](https://repo.harmonyos.com/#/en/application/atomService/@ohos%2Fhypium). You must modify the npm version in **package.info** of the source code project before using the mock capability.

-  **Available APIs**

| No. | API | Description|
| --- | --- | --- |
| 1 | mockFunc(obj: object, f: function())| Mocks a function in the object of a class. The parameters **obj** and **f** must be passed in. This API supports asynchronous functions. <br>**NOTE**: This API does not focus on the implementation of the original function. Therefore, it does not matter whether the original function is implemented synchronously or asynchronously.|
| 2 | when(mockedfunc: function)| Checks whether the input function is mocked and marked. A function declaration is returned.|
| 3 | afterReturn(x: value)| Sets an expected return value, for example, a string or promise.|
| 4 | afterReturnNothing() | Sets the expected return value to **undefined**, that is, no value will be returned.|
| 5 | afterAction(x: action)| Sets the expected return value to be an action executed by a function.|
| 6 | afterThrow(x: msg)| Sets an exception to throw and the error message.|
| 7 | clear() | Restores the mocked object after the test case is complete (restores the original features of the object).|
| 8 | any | Returns the expected value if a parameter of any type (except **undefined** and **null**) is passed in. This API must be called by **ArgumentMatchers.any**.|
| 9 | anyString | Returns the expected value if a string is passed in. This API must be called by **ArgumentMatchers.anyString**.|
| 10 | anyBoolean | Returns the expected value if a Boolean value is passed in. This API must be called by **ArgumentMatchers.anyBoolean**.|
| 11 | anyFunction | Returns the expected value if a function is passed in. This API must be called by **ArgumentMatchers.anyFunction**.|
| 12 | anyNumber | Returns the expected value if a number is passed in. This API must be called by **ArgumentMatchers.anyNumber**.|
| 13 | anyObj | Returns the expected value if an object is passed in. This API must be called by **ArgumentMatchers.anyObj**.|
| 14 | matchRegexs(Regex) | Returns the expected value if a regular expression is passed in. This API must be called by **ArgumentMatchers.matchRegexs(Regex)**.|
| 15 | verify(methodName, argsArray) | Verifies whether a function and its parameters are processed as expected. This API returns a **VerificationMode**, which is a class that provides the verification mode. This class provides functions such as **times(count)**, **once()**, **atLeast(x)**, **atMost(x)**, and **never()**.|
| 16 | times(count) | Verifies whether the function was executed the specified number of times.|
| 17 | once() | Verifies whether the function was executed only once.|
| 18 | atLeast(count) | Verifies whether the function was executed at least **count** times.|
| 19 | atMost(count) | Verifies whether the function was executed at most **count** times.|
| 20 | never | Verifies whether the function has never been executed.|
| 21 | ignoreMock(obj, method) | Restores the mocked function in the object. This API is valid for mocked functions.|
| 22 | clearAll() | Clears all data and memory after the test cases are complete.|

-  **Examples**

You can refer to the following examples to import the mock module and write test cases.

- **NOTICE**<br>
You must import **MockKit** and **when**. You can import other assertion APIs based on the test cases.
Example: `import {describe, expect, it, MockKit, when} from '@ohos/hypium'`

Example 1: Use **afterReturn**.

```javascript
import {describe, expect, it, MockKit, when} from '@ohos/hypium';

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");

            // 1. Create a MockKit object.
            let mocker = new MockKit();

            // 2. Define the ClassName class, which contains two functions, and then create a claser object.
            class ClassName {
                constructor() {
                }

                method_1(arg) {
                    return '888888';
                }

                method_2(arg) {
                    return '999999';
                }
            }

            let claser = new ClassName();

            // 3. Mock method_1 of the ClassName class.
            let mockfunc = mocker.mockFunc(claser, claser.method_1);
            when(mockfunc)('test').afterReturn('1');

            // 4. Assert whether the mocked function is implemented as expected.
            // The operation is successful if 'test' is passed in.
            expect(claser.method_1('test')).assertEqual('1'); // The operation is successful.

            // The operation fails if 'abc' is passed in.
            //expect(claser.method_1('abc')).assertEqual('1'); // The operation fails.
        });
    });
}
```
- **NOTICE**<br>
In `when(mockfunc)('test').afterReturn('1');`, `('test')` is the value to pass in the mocked function. Currently, only one parameter is supported. `afterReturn('1')` is the expected return value. The expected value is returned only when `('test')` is passed in.

Example 2: Use **afterReturnNothing**.

```javascript
import {describe, expect, it, MockKit, when} from '@ohos/hypium';

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");

            // 1. Create a MockKit object.
            let mocker = new MockKit();

            // 2. Define the ClassName class, which contains two functions, and then create a claser object.
            class ClassName {
                constructor() {
                }

                method_1(arg) {
                    return '888888';
                }

                method_2(arg) {
                    return '999999';
                }
            }

            let claser = new ClassName();

            // 3. Mock method_1 of the ClassName class.
            let mockfunc = mocker.mockFunc(claser, claser.method_1);

            // 4. Set the action to be performed when the test case ends. For example, set it to afterReturnNothing(), which returns no value.
            when(mockfunc)('test').afterReturnNothing();

            // 5. Assert whether the mocked function is implemented as expected. Use the assertion APIs corresponding to Step 4.
            // The operation is successful if 'test' is passed in.
            // The mocked claser.method_1 does not return '888888'. Instead, afterReturnNothing() takes effect, that is, no value is returned.
            expect(claser.method_1('test')).assertUndefined(); // The operation is successful.

            // The operation fails if '123' is passed in.
            // expect(method_1(123)).assertUndefined();// The operation fails.
        });
    });
}
```

Example 3: Set the parameter type to **any**, that is, allow any parameter value except **undefine** and **null**.


- **NOTICE**<br>
The **ArgumentMatchers** class, for example, **ArgumentMatchers.any**, must be imported.

```javascript
import {describe, expect, it, MockKit, when, ArgumentMatchers} from '@ohos/hypium';

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");

            // 1. Create a MockKit object.
            let mocker = new MockKit();

            // 2. Define the ClassName class, which contains two functions, and then create a claser object.
            class ClassName {
                constructor() {
                }

                method_1(arg) {
                    return '888888';
                }

                method_2(arg) {
                    return '999999';
                }
            }

            let claser = new ClassName();

            // 3. Mock method_1 of the ClassName class.
            let mockfunc = mocker.mockFunc(claser, claser.method_1);
            // Set the parameter matcher and expected return value as required.
            when(mockfunc)(ArgumentMatchers.any).afterReturn('1');

            // 4. Assert whether the mocked function is implemented as expected.
            // The operation is successful if a string is passed in.
            expect(claser.method_1('test')).assertEqual('1'); // The operation is successful.
            // The operation is successful if a number (for example '123') is passed in.
            expect(claser.method_1(123)).assertEqual('1'); // The operation is successful.
            // The operation is successful if a Boolean value (for example 'true') is passed in.
            expect(claser.method_1(true)).assertEqual('1');// The operation is successful.

            // The operation fails if an empty value is passed in.
            //expect(claser.method_1()).assertEqual('1');// The operation fails.
        });
    });
}
```

Example 4: Set the parameter type to **anyString**.

```javascript
import {describe, expect, it, MockKit, when, ArgumentMatchers} from '@ohos/hypium';

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");

            // 1. Create a MockKit object.
            let mocker = new MockKit();

            // 2. Define the ClassName class, which contains two functions, and then create a claser object.
            class ClassName {
                constructor() {
                }

                method_1(arg) {
                    return '888888';
                }

                method_2(arg) {
                    return '999999';
                }
            }

            let claser = new ClassName();

            // 3. Mock method_1 of the ClassName class.
            let mockfunc = mocker.mockFunc(claser, claser.method_1);
            // Set the following parameters as required. 
            when(mockfunc)(ArgumentMatchers.anyString).afterReturn('1');

            // 4. Assert whether the mocked function is implemented as expected.
            // The operation is successful if a string is passed in.
            expect(claser.method_1('test')).assertEqual('1'); // The operation is successful.
            expect(claser.method_1('abc')).assertEqual('1'); // The operation is successful.

            // The operation fails if a number or a Boolean value is passed in.
            //expect(claser.method_1(123)).assertEqual('1'); // The operation fails.
            //expect(claser.method_1(true)).assertEqual('1'); // The operation fails.
        });
    });
}
```

Example 5: Set the parameter type to **matchRegexs (Regex)**.
```javascript
import {describe, expect, it, MockKit, when, ArgumentMatchers} from '@ohos/hypium';

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");

            // 1. Create a MockKit object.
            let mocker = new MockKit();

            // 2. Define the ClassName class, which contains two functions, and then create a claser object.
            class ClassName {
                constructor() {
                }

                method_1(arg) {
                    return '888888';
                }

                method_2(arg) {
                    return '999999';
                }
            }

            let claser = new ClassName();

            // 3. Mock method_1 of the ClassName class.
            let mockfunc = mocker.mockFunc(claser, claser.method_1);
            // Set a regular expression to match, for example, /123456/.
            when(mockfunc)(ArgumentMatchers.matchRegexs(/123456/)).afterReturn('1');

            // 4. Assert whether the mocked function is implemented as expected.
            // The operation is successful if a string, for example, '1234567898', is passed in.
            expect(claser.method_1('1234567898')).assertEqual('1'); // The operation is successful.
            // The string '1234567898' matches the regular expression /123456/.

            // The operation fails if '1234' is passed in.
            //expect(claser.method_1('1234').assertEqual('1'); // The operation fails because '1234' does not match the regular expression /123456/.
        });
    });
}
```

Example 6: Use **verify()**.
```javascript
import {describe, expect, it, MockKit, when} from '@ohos/hypium';

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");

            // 1. Create a MockKit object.
            let mocker = new MockKit();

            // 2. Define the ClassName class, which contains two functions, and then create a claser object.
            class ClassName {
                constructor() {
                }

                method_1(...arg) {
                    return '888888';
                }

                method_2(...arg) {
                    return '999999';
                }
            }

            let claser = new ClassName();

            // 3. Mock method_1 and method_2 of the ClassName class.
            mocker.mockFunc(claser, claser.method_1);
            mocker.mockFunc(claser, claser.method_2);

            // 4. Call the following methods.
            claser.method_1('abc', 'ppp');
            claser.method_1('abc');
            claser.method_1('xyz');
            claser.method_1();
            claser.method_1('abc', 'xxx', 'yyy');
            claser.method_1();
            claser.method_2('111');
            claser.method_2('111', '222');

            //5. Verify the mocked functions.
            mocker.verify('method_1', []).atLeast(3); // The result is "failed".
            // Verify whether 'method_1' with an empty parameter list was executed at least three times.
            // The result is "failed" because 'method_1' with an empty parameter list was executed only twice in Step 4.
            //mocker.verify('method_2',['111']).once(); // The result is "success".
            //mocker.verify('method_2',['111',,'222']).once(); // The result is "success".
        });
    });
}
```

Example 7: Use **ignoreMock(obj, method)**.
```javascript
import {describe, expect, it, MockKit, when, ArgumentMatchers} from '@ohos/hypium';

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");

            // 1. Create a MockKit object.
            let mocker = new MockKit();

            // 2. Define the ClassName class, which contains two functions, and then create a claser object.
            class ClassName {
                constructor() {
                }

                method_1(...arg) {
                    return '888888';
                }

                method_2(...arg) {
                    return '999999';
                }
            }

            let claser = new ClassName();

            // 3. Mock method_1 and method_2 of the ClassName class.
            let func_1 = mocker.mockFunc(claser, claser.method_1);
            let func_2 = mocker.mockFunc(claser, claser.method_2);

            // 4. Modify the mocked functions.
            when(func_1)(ArgumentMatchers.anyNumber).afterReturn('4');
            when(func_2)(ArgumentMatchers.anyNumber).afterReturn('5');

            // 5. Call the following methods.
            console.log(claser.method_1(123)); // The return value is 4, which is the same as the expected value in Step 4.
            console.log(claser.method_2(456)); // The return value is 5, which is the same as the expected value in Step 4.

            // 6. Restore method_1 using ignoreMock(). 
            mocker.ignoreMock(claser, claser.method_1);
            // Call claser.method_1 and check the execution result.
            console.log(claser.method_1(123)); // The return value is 888888, which is the same as that returned by the original function.
            // Test with assertions.
            expect(claser.method_1(123)).assertEqual('4'); // The return value is "failed", which meets the expected value of ignoreMock().
            claser.method_2(456); // The return value is 5, which is the same as the expected value in Step 4 because method_2 is not restored.
        });
    });
}
```

Example 8: Use **clear()**.

```javascript
import {describe, expect, it, MockKit, when, ArgumentMatchers} from '@ohos/hypium';

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");

            // 1. Create a MockKit object.
            let mocker = new MockKit();

            // 2. Define the ClassName class, which contains two functions, and then create a claser object.
            class ClassName {
                constructor() {
                }

                method_1(...arg) {
                    return '888888';
                }

                method_2(...arg) {
                    return '999999';
                }
            }
            let claser = new ClassName();

            // 3. Mock method_1 and method_2 of the ClassName class.
            let func_1 = mocker.mockFunc(claser, claser.method_1);
            let func_2 = mocker.mockFunc(claser, claser.method_2);

            // 4. Modify the mocked functions.
            when(func_1)(ArgumentMatchers.anyNumber).afterReturn('4');
            when(func_2)(ArgumentMatchers.anyNumber).afterReturn('5');

            // 5. Call the following methods.
            //expect(claser.method_1(123)).assertEqual('4'); // The return value is the same as the expected value.
            //expect(claser.method_2(456)).assertEqual('5'); // The return value is the same as the expected value.

            // 6. Clear the mock operation.
            mocker.clear(claser);
            // Call claser.method_1 and check the execution result.
            expect(claser.method_1(123)).assertEqual('4'); // The return value is "failed", which meets the expectation.
            expect(claser.method_2(456)).assertEqual('5'); // The return value is "failed", which meets the expectation.
        });
    });
}
```


Example 9: Use **afterThrow(msg)**.

```javascript
import {describe, expect, it, MockKit, when} from '@ohos/hypium';

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");

            // 1. Create a MockKit object.
            let mocker = new MockKit();

            // 2. Define the ClassName class, which contains two functions, and then create a claser object.
            class ClassName {
                constructor() {
                }

                method_1(arg) {
                    return '888888';
                }
            }

            let claser = new ClassName();

            // 3. Mock method_1 of the ClassName class.
            let mockfunc = mocker.mockFunc(claser, claser.method_1);

            // 4. Set the action to be performed when the test case ends. For example, set it to afterReturnNothing(), which returns no value.
            when(mockfunc)('test').afterThrow('error xxx');

            // 5. Execute the mocked function, capture the exception, and use assertEqual() to check whether message meets the expectation.
            try {
                claser.method_1('test');
            } catch (e) {
                expect(e).assertEqual('error xxx'); // The execution is successful.
            }
        });
    });
}
```

Example 10: Mock asynchronous functions.

```javascript
import {describe, expect, it, MockKit, when} from '@ohos/hypium';

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");

            // 1. Create a MockKit object.
            let mocker = new MockKit();

            // 2. Define the ClassName class, which contains two functions, and then create a claser object.
            class ClassName {
                constructor() {
                }

                async method_1(arg) {
                    return new Promise((res, rej) => {
                        // Perform asynchronous operations.
                        setTimeout(function () {
                            console.log ('Execute');
                            res('Pass data');
                        }, 2000);
                    });
                }
            }

            let claser = new ClassName();

            // 3. Mock method_1 of the ClassName class.
            let mockfunc = mocker.mockFunc(claser, claser.method_1);

            // 4. Set the action to be performed after the test case ends. For example, set it to afterRetrun(), which returns a custom promise.
            when(mockfunc)('test').afterReturn(new Promise((res, rej) => {
                console.log("do something");
                res('success something');
            }));

            // 5. Execute the mocked function, that is, execute the promise.
            claser.method_1('test').then(function (data) {
                // Code for data processing
                console.log('result : ' + data);
            });
        });
    });
}
```

Example 11: Mock a system function.

```javascript
export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('test_systemApi', 0, function () {
            // 1. Create a MockKit object.
            let mocker = new MockKit();
            // 2. Mock the app.getInfo() function.
            let mockf = mocker.mockFunc(app, app.getInfo);
            when(mockf)('test').afterReturn('1');
            // The operation is successful.
            expect(app.getInfo('test')).assertEqual('1');
        });
    });
}
```


Example 12: Verify **times(count)**.

```javascript
import { describe, expect, it, MockKit, when } from '@ohos/hypium'

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('test_verify_times', 0, function () {
            // 1. Create a MockKit object.
            let mocker = new MockKit();
            // 2. Define the class to be mocked.
            class ClassName {
                constructor() {
                }

                method_1(...arg) {
                    return '888888';
                }
            }
            // 3. Create an object of the class.
            let claser = new ClassName();
            // 4. Mock a function, for example, method_1, of the object.
            let func_1 = mocker.mockFunc(claser, claser.method_1);
            // 5. Set the expected value to be returned by the mocked function.
            when(func_1)('123').afterReturn('4');

            // 6. Execute the function several times, with parameters set as follows:
            claser.method_1('123', 'ppp');
            claser.method_1('abc');
            claser.method_1('xyz');
            claser.method_1();
            claser.method_1('abc', 'xxx', 'yyy');
            claser.method_1('abc');
            claser.method_1();
            // 7. Check whether method_1 with the parameter of 'abc' was executed twice.
            mocker.verify('method_1', ['abc']).times(2);
        });
    });
}
```


Example 13: Verify **atLeast(count)**.

```javascript
import { describe, expect, it, MockKit, when } from '@ohos/hypium'

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('test_verify_atLeast', 0, function () {
            // 1. Create a MockKit object.
            let mocker = new MockKit();
            // 2. Define the class to be mocked.
            class ClassName {
                constructor() {
                }

                method_1(...arg) {
                    return '888888';
                }
            }

            // 3. Create an object of the class.
            let claser = new ClassName();
            // 4. Mock a function, for example, method_1, of the object.
            let func_1 = mocker.mockFunc(claser, claser.method_1);
            // 5. Set the expected value to be returned by the mocked function.
            when(func_1)('123').afterReturn('4');
            // 6. Execute the function several times, with parameters set as follows:
            claser.method_1('123', 'ppp');
            claser.method_1('abc');
            claser.method_1('xyz');
            claser.method_1();
            claser.method_1('abc', 'xxx', 'yyy');
            claser.method_1();
            // 7. Check whether method_1 with an empty value was executed at least twice.
            mocker.verify('method_1', []).atLeast(2);
        });
    });
}
```

#### Data Driving

##### Constraints

JsUnit provides the following data driving capability since [Hypium 1.0.2](https://repo.harmonyos.com/#/en/application/atomService/@ohos%2Fhypium):

- Passes parameters for the specified test suite and test case.
- Specifies the number of times that the test suite and test case are executed.

The execution times of test cases and the parameters passed in each time are determined by the settings in **data.json**. The file content is as follows:

>**NOTE**<br>The **data.json** file is in the same directory as the **.test.js** or **.test.ets** file.

```json
{
	"suites": [{
		"describe": ["actsAbilityTest"],
		"stress": 2,
		"params": {
			"suiteParams1": "suiteParams001",
			"suiteParams2": "suiteParams002"
		},
		"items": [{
			"it": "testDataDriverAsync",
			"stress": 2,
			"params": [{
				"name": "tom",
				"value": 5
			}, {
				"name": "jerry",
				"value": 4
			}]
		}, {
			"it": "testDataDriver",
			"stress": 3
		}]
	}]
}
```

Parameter description:

|      | Name| Description                                 | Mandatory|
| :--- | :--------- | :------------------------------------ | ---- |
| 1    | "suite"    | Test suite configuration.                        | Yes  |
| 2    | "items"    | Test case configuration.                      | Yes  |
| 3    | "describe" | Test suite name.                        | Yes  |
| 4    | "it"       | Test case name.                      | Yes  |
| 5    | "params"   | Parameters to be passed to the test suite or test case.| No  |
| 6    | "stress"   | Number of times that the test suite or test case is executed.    | No  |

The sample code is as follows:

Import the **data.json** file to the **app.js** or **app.ets** file in the **TestAbility** directory, and set parameters before executing the **Hypium.hypiumTest()** method.

```javascript
import AbilityDelegatorRegistry from '@ohos.application.abilityDelegatorRegistry'
import { Hypium } from '@ohos/hypium'
import testsuite from '../test/List.test'
import data from '../test/data.json';

...
Hypium.setData(data);
Hypium.hypiumTest(abilityDelegator, abilityDelegatorArguments, testsuite)
...
```

```javascript
import {describe, beforeAll, beforeEach, afterEach, afterAll, it, expect} from '@ohos/hypium';

export default function abilityTest() {
    describe('actsAbilityTest', function () {
        it('testDataDriverAsync', 0, async function (done, data) {
            console.info('name: ' + data.name);
            console.info('value: ' + data.value);
            done();
        });

        it('testDataDriver', 0, function () {
            console.info('stress test');
        });
    });
}
```

### How to Use

JsUnit is released as an npm (hypium) package at the [service component official website](https://repo.harmonyos.com/#/en/application/atomService/@ohos%2Fhypium). You can download Deveco Studio, configure dependencies in the application project, and use JsUnit. For details about how to create a test project and execute test scripts, see the [OpenHarmony Test Framework](https://developer.harmonyos.com/en/docs/documentation/doc-guides/ohos-openharmony-test-framework-0000001267284568).

## UiTest Features

| No.  | Feature       | Description                                                    |
| ---- | ----------- | ------------------------------------------------------------ |
| 1    | UiDriver    | Provides the UI test entry. It provides APIs for locating a component, checking whether a component exists, and injecting a key.|
| 2    | By          | Describes the attributes (such as text, ID, and type) of UI components. `UiDriver` locates the component based on the attributes described by `By`.|
| 3    | UiComponent | Provides the UI component object, which provides APIs for obtaining component attributes, clicking a component, and scrolling to search for a component.|
| 4    | UiWindow    | Provides the window object, which provides APIs for obtaining window attributes, dragging a window, and adjusting the window size.|

Import the following to the test script:

```typescript
import {UiDriver,BY,UiComponent,Uiwindow,MatchPattern} from '@ohos.uitest'
```

> **NOTICE**<br>
> - All APIs provided by the `By` class are synchronous. You can use `builder` to call the APIs in chain mode to construct component filtering conditions.
> - All the APIs provided by the `UiDriver` and `UiComponent` classes are asynchronous (in `Promise` mode), and `await` must be used.
> - All UI test cases must be written in the asynchronous syntax and comply with the asynchronous test case specifications of JsUnit.

   

Import the `By`, `UiDriver`, and `UiComponent` classes to the test case file, and then call the APIs to write test cases.

```javascript
import {describe, beforeAll, beforeEach, afterEach, afterAll, it, expect} from '@ohos/hypium'
import {BY, UiDriver, UiComponent, MatchPattern} from '@ohos.uitest'

export default async function abilityTest() {
  describe('uiTestDemo', function() {
    it('uitest_demo0', 0, async function() {
      // create UiDriver
      let driver = await UiDriver.create()
      // find component by text
      let button = await driver.findComponent(BY.text('hello').enabled(true))
      // click component
      await button.click()
      // get and assert component text
      let content = await button.getText()
      expect(content).assertEquals('clicked!')
    })
  })
}
```

### Using UiDriver

As the main entry to UiTest, the `UiDriver` class provides APIs for component matching/search, key injection, coordinate clicking/swiping, and screenshot.

| No.  | API                                                          | Description                |
| ---- | ------------------------------------------------------------ | ------------------------ |
| 1    | create():Promise<UiDriver>                                   | Creates a **UiDriver** object. This API is a static method.|
| 2    | findComponent(b:By):Promise<UiComponent>                     | Searches for a component.          |
| 3    | pressBack():Promise<void>                                    | Presses the BACK button.            |
| 4    | click(x:number, y:number):Promise<void>                      | Clicks a specific point based on the given coordinates.      |
| 5    | swipe(x1:number, y1:number, x2:number, y2:number):Promise<void> | Swipes based on the given coordinates.      |
| 6    | assertComponentExist(b:By):Promise<void>                     | Asserts that the component matching the given attributes exists on the current page.    |
| 7    | delayMs(t:number):Promise<void>                              | Delays this **UiDriver** object for the specified duration.                  |
| 8    | screenCap(s:path):Promise<void>                              | Captures the current screen.                  |
| 9    | findWindow(filter: WindowFilter): Promise<UiWindow>          | Searches for a window.          |

**assertComponentExist()** is an assertion API, which is used to assert that the target component exists on the current page. If the component does not exist, a JS exception will be thrown, causing the test case to fail.

```javascript
import {BY,UiDriver,UiComponent} from '@ohos.uitest'

export default async function abilityTest() {
  describe('UiTestDemo', function() {
    it('Uitest_demo0', 0, async function(done) {
      try{
        // create UiDriver
        let driver = await UiDriver.create()
        // assert text 'hello' exists on current Ui
        await assertComponentExist(BY.text('hello'))
      } finally {
        done()
      }
    })
  })
}
```

For details about the APIs of `UiDriver`, see [@ohos.uitest.d.ts](https://gitee.com/openharmony/interface_sdk-js/blob/master/api/@ohos.uitest.d.ts) and [UiDriver](https://gitee.com/openharmony/docs/blob/master/en/application-dev/reference/apis/js-apis-uitest.md#uidriver).

### Using By

UiTest provides a wide range of UI component feature description APIs in the `By` class to filter and match components. The APIs provided by `By` exhibit the following features:

- Allow one or more attributes as the match conditions. For example, you can specify both the text and id attributes to find a component.
- Provide a variety of match patterns (`EQUALS`, `CONTAINS`, `STARTS_WITH`, and `ENDS_WITH`) for component attributes.
- Support relative positioning for components. APIs such as `isBefore` and `isAfter` can be used to specify the features of adjacent components to assist positioning.

| No.  | API                                | Description                                        |
| ---- | ---------------------------------- | ------------------------------------------------ |
| 1    | id(i:number):By                    | Specifies the component ID.                                    |
| 2    | text(t:string, p?:MatchPattern):By | Specifies the component text. You can specify the match pattern.                  |
| 3    | type(t:string)):By                 | Specifies the component type.                                  |
| 4    | enabled(e:bool):By                 | Specifies the component state, which can be enabled or disabled.                              |
| 5    | clickable(c:bool):By               | Specifies the clickable status of the component.                            |
| 6    | focused(f:bool):By                 | Specifies the focused status of the component.                              |
| 7    | scrollable(s:bool):By              | Specifies the scrollable status of the component.                            |
| 8    | selected(s:bool):By                | Specifies the selected status of the component.                              |
| 9    | isBefore(b:By):By                  | Specifies the attributes of the component that locates before the target component.|
| 10   | isAfter(b:By):By                   | Specifies the attributes of the component that locates after the target component.|

The `text` attribute supports match patterns `MatchPattern.EQUALS`, `MatchPattern.CONTAINS`, `MatchPattern.STARTS_WITH`, and `MatchPattern.ENDS_WITH`. The default match pattern is `MatchPattern.EQUALS`.

For details about the APIs of `By`, see [@ohos.uitest.d.ts](https://gitee.com/openharmony/interface_sdk-js/blob/master/api/@ohos.uitest.d.ts) and [By](https://gitee.com/openharmony/docs/blob/master/en/application-dev/reference/apis/js-apis-uitest.md#by).

#### Absolute Positioning of a Component

Example 1: Search for component `Id_button`.

```javascript
let button = await driver.findComponent(BY.id(Id_button))
```

 Example 2: Search for component `Id_button` in the `enabled` state. Use this API when the component cannot be located based on a single attribute.

```javascript
let button = await driver.findComponent(BY.id(Id_button).enabled(true))
```

Use `By.id(x).enabled(y)` to specify multiple attributes of the target component.

Example 3: Search for the component whose text contains `hello`. Use this API when the component attribute values cannot be completely determined.

```javascript
let txt = await driver.findComponent(BY.text("hello", MatchPattern.CONTAINS))
```

`By.text()` passes the second parameter `MatchPattern.CONTAINS` to specify the matching rule. The default rule is `MatchPattern.EQUALS`, that is, the text attribute of the target component must be equal to the specified value.

####  Relative Positioning of a Component

Example 1: Search for the switch component `ResourceTable.Id_switch` following the text component `Item3_3`.

```javascript
let switch = await driver.findComponent(BY.id(Id_switch).isAfter(BY.text("Item3_3")))
```

Use `By.isAfter` to specify the attributes of the feature component located before the target component for relative positioning. Generally, a feature component is a component that has a globally unique feature (for example, a unique ID or a unique text).

Similarly, you can use `By.isBefore` to specify the attributes of the feature component located after the target component to implement relative positioning.

### Using UiComponent

The `UiComponent` class represents a UI component, which can be located by using `UiDriver.findComponent(by)`. It provides APIs for obtaining component attributes, clicking a component, scrolling to search for a component, and text injection.

`UiComponent` provides the following APIs:

| No.  | API                               | Description                                      |
| ---- | --------------------------------- | ---------------------------------------------- |
| 1    | click():Promise<void>             | Clicks the component.                                  |
| 2    | inputText(t:string):Promise<void> | Inputs text into the component. This API is applicable to text box components.          |
| 3    | scrollSearch(s:By):Promise<bool>  | Scrolls on this component to search for the target component. This API is applicable to the **List** components.|
| 4    | getText():Promise<string>         | Obtains the component text.                                |
| 5    | getId():Promise<number>           | Obtains the component ID.                                  |
| 6    | getType():Promise<string>         | Obtains the component type.                                |
| 7    | isEnabled():Promise<bool>         | Obtains the component state, which can be enabled or disabled.                            |

For details about the APIs of `UiComponent`, see [@ohos.uitest.d.ts](https://gitee.com/openharmony/interface_sdk-js/blob/master/api/@ohos.uitest.d.ts) and [UiComponent](https://gitee.com/openharmony/docs/blob/master/en/application-dev/reference/apis/js-apis-uitest.md#uicomponent).

Example 1: Click a component.

```javascript
let button = await driver.findComponent(BY.id(Id_button))
await button.click()
```

Example 2: After obtaining component attributes, use **assert()** to make assertion.

```javascript
let component = await driver.findComponent(BY.id(Id_title))
expect(component !== null).assertTrue()
```

Example 3: Scroll on the **List** component to locate the child component with text `Item3_3`.

```javascript
let list = await driver.findComponent(BY.id(Id_list))
let found = await list.scrollSearch(BY.text("Item3_3"))
expect(found).assertTrue()
```

Example 4: Input text in a text box.

```javascript
let editText = await driver.findComponent(BY.type('InputText'))
await editText.inputText("user_name")
```
### Using UiWindow

The `UiWindow` class represents a UI window, which can be located by using `UiDriver.findWindow(by)`. You can use the instance provided by this class to obtain window attributes, drag a window, and adjust the window size.

`UiWindow` provides the following APIs:

| No.  | API                                                          | Description                                          |
| ---- | ------------------------------------------------------------ | -------------------------------------------------- |
| 1    | getBundleName(): Promise<string>                             | Obtains the bundle name of the window.                            |
| 2    | getTitle(): Promise<string>                                  | Obtains the window title.                                |
| 3    | focus(): Promise<bool>                                       | Gives focus to the current window.                            |
| 4    | moveTo(x: number, y: number): Promise<bool>;                 | Moves the current window to the specified position. This API is applicable to the windows that can be moved.|
| 5    | resize(wide: number, height: number, direction: ResizeDirection): Promise<bool> | Adjusts the window size. This API is applicable to the windows that can be resized.        |
| 6    | split(): Promise<bool>                                       | Splits the current window. This API is applicable to the windows that support split-screen mode.  |
| 7    | close(): Promise<bool>                                       | Closes the current window.                                    |

For details about the APIs of `UiWindow`, see [@ohos.uitest.d.ts](https://gitee.com/openharmony/interface_sdk-js/blob/master/api/@ohos.uitest.d.ts) and [UiWindow](https://gitee.com/openharmony/docs/blob/master/en/application-dev/reference/apis/js-apis-uitest.md#uiwindow9).

Example 1: Obtain the window attributes.

```javascript
let window = await driver.findWindow({actived: true})
let bundelName = await window.getBundleName()
```

Example 2: Move the window.

```javascript
let window = await driver.findWindow({actived: true})
await window.moveTo(500,500)
```

Example 3: Close the window.

```javascript
let window = await driver.findWindow({actived: true})
await window.close()
```

### How to Use

  Download Deveco Studio, create a test project, and call the APIs provided by UiTest to perform related tests. For details about how to create a test project and execute test scripts, see [OpenHarmony Test Framework](https://developer.harmonyos.com/en/docs/documentation/doc-guides/ohos-openharmony-test-framework-0000001267284568).
  Run the following command to enable UiTest:

>```shell
> hdc_std shell param set persist.ace.testmode.enabled 1
>```
### Building UiTest

> UiTest is not built with OpenHarmony 3.1 Release and needs to be manually built. For details, see [Pushing UiTest to a Device](https://gitee.com/openharmony/arkXtest/blob/OpenHarmony-3.1-Release/README_en.md#%E6%8E%A8%E9%80%81ui%E6%B5%8B%E8%AF%95%E6%A1%86%E6%9E%B6%E8%87%B3%E8%AE%BE%E5%A4%87).

If you want to modify UiTest code and verify the modification, use the following commands.

#### Building UiTest

```shell
./build.sh --product-name rk3568 --build-target uitestkit
```
#### Sending UiTest

```shell
hdc_std target mount
hdc_std shell mount -o rw,remount /
hdc_std file send uitest /system/bin/uitest
hdc_std file send libuitest.z.so /system/lib/module/libuitest.z.so
hdc_std shell chmod +x /system/bin/uitest
```

### Version Information

| Version | Description                                                  |
| :------ | :----------------------------------------------------------- |
| 3.2.2.1 | 1. Added the APIs for obtaining and setting the screen orientation and flinging.<br>2. Added the window processing logic for unsupported scenarios. |
