# 自动化测试框架使用介绍

## 简介
 OpenHarmony自动化测试框架代码部件仓arkXtest，包含单元测试框架(JsUnit)和Ui测试框架(UiTest)。

 单元测试框架(JsUnit)提供单元测试用例执行能力，提供用例编写基础接口，生成对应报告，用于测试系统或应用接口。

 Ui测试框架(UiTest)通过简洁易用的API提供查找和操作界面控件能力，支持用户开发基于界面操作的自动化测试脚本。

## 目录

```
arkXtest 
  |-----jsunit  单元测试框架
  |-----uitest  Ui测试框架
```
## 约束限制
本模块首批接口从API version 8开始支持。后续版本的新增接口，采用上角标单独标记接口的起始版本。

## 单元测试框架功能特性

| No.  | 特性     | 功能说明                                                     |
| ---- | -------- | ------------------------------------------------------------ |
| 1    | 基础流程 | 支持编写及执行基础用例。                                     |
| 2    | 断言库   | 判断用例实际期望值与预期值是否相符。                         |
| 3    | Mock能力 | 支持函数级mock能力，对定义的函数进行mock后修改函数的行为，使其返回指定的值或者执行某种动作。 |
| 4    | 数据驱动 | 提供数据驱动能力，支持复用同一个测试脚本，使用不同输入数据驱动执行。 |

### 使用说明

####  基础流程

测试用例采用业内通用语法，describe代表一个测试套， it代表一条用例。

| No.  | API        | 功能说明                                                     |
| ---- | ---------- | ------------------------------------------------------------ |
| 1    | describe   | 定义一个测试套，支持两个参数：测试套名称和测试套函数。       |
| 2    | beforeAll  | 在测试套内定义一个预置条件，在所有测试用例开始前执行且仅执行一次，支持一个参数：预置动作函数。 |
| 3    | beforeEach | 在测试套内定义一个单元预置条件，在每条测试用例开始前执行，执行次数与it定义的测试用例数一致，支持一个参数：预置动作函数。 |
| 4    | afterEach  | 在测试套内定义一个单元清理条件，在每条测试用例结束后执行，执行次数与it定义的测试用例数一致，支持一个参数：清理动作函数。 |
| 5    | afterAll   | 在测试套内定义一个清理条件，在所有测试用例结束后执行且仅执行一次，支持一个参数：清理动作函数。 |
| 6    | it         | 定义一条测试用例，支持三个参数：用例名称，过滤参数和用例函数。 |
| 7    | expect     | 支持bool类型判断等多种断言方法。                             |

示例代码：

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



####  断言库

断言功能列表：


| No.  | API                | 功能说明                                                     |
| :--- | :------------------| ------------------------------------------------------------ |
| 1    | assertClose        | 检验actualvalue和expectvalue(0)的接近程度是否是expectValue(1)。 |
| 2    | assertContain      | 检验actualvalue中是否包含expectvalue。                       |
| 3    | assertEqual        | 检验actualvalue是否等于expectvalue[0]。                      |
| 4    | assertFail         | 抛出一个错误。                                               |
| 5    | assertFalse        | 检验actualvalue是否是false。                                 |
| 6    | assertTrue         | 检验actualvalue是否是true。                                  |
| 7    | assertInstanceOf   | 检验actualvalue是否是expectvalue类型。                       |
| 8    | assertLarger       | 检验actualvalue是否大于expectvalue。                         |
| 9    | assertLess         | 检验actualvalue是否小于expectvalue。                         |
| 10   | assertNull         | 检验actualvalue是否是null。                                  |
| 11   | assertThrowError   | 检验actualvalue抛出Error内容是否是expectValue。              |
| 12   | assertUndefined    | 检验actualvalue是否是undefined。                             |
| 13   | assertNaN          | 检验actualvalue是否是一个NAN                                 |
| 14   | assertNegUnlimited | 检验actualvalue是否等于Number.NEGATIVE_INFINITY             |
| 15   | assertPosUnlimited | 检验actualvalue是否等于Number.POSITIVE_INFINITY             |
| 16   | assertDeepEquals   | 检验actualvalue和expectvalue是否完全相等               |
| 17   | assertPromiseIsPending | 判断promise是否处于Pending状态。                         |
| 18   | assertPromiseIsRejected | 判断promise是否处于Rejected状态。                       |
| 19   | assertPromiseIsRejectedWith | 判断promise是否处于Rejected状态，并且比较执行的结果值。|
| 20   | assertPromiseIsRejectedWithError | 判断promise是否处于Rejected状态并有异常，同时比较异常的类型和message值。                   |
| 21   | assertPromiseIsResolved | 判断promise是否处于Resolved状态。                       |
| 22   | assertPromiseIsResolvedWith | 判断promise是否处于Resolved状态，并且比较执行的结果值。|
| 23   | not                | 断言取反,支持上面所有的断言功能                                 |

示例代码：

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
    it('assertNaN success',0, function () {
      expect(Number.NaN).assertNaN(); // true
    })
    it('assertNegUnlimited success',0, function () {
      expect(Number.NEGATIVE_INFINITY).assertNegUnlimited(); // true
    })
    it('assertPosUnlimited success',0, function () {
      expect(Number.POSITIVE_INFINITY).assertPosUnlimited(); // true
    })
    it('not_number_true',0, function () {
      expect(1).not().assertLargerOrEqual(2)
    })
    it('not_number_true_1',0, function () {
      expect(3).not().assertLessOrEqual(2);
    })
    it('not_NaN_true',0, function () {
      expect(3).not().assertNaN();
    })
    it('not_contain_true',0, function () {
      let a = "abc";
      let b= "cdf"
      expect(a).not().assertContain(b);
    })
    it('not_large_true',0, function () {
      expect(3).not().assertLarger(4);
    })
    it('not_less_true',0, function () {
      expect(3).not().assertLess(2);
    })
    it('not_undefined_true',0, function () {
      expect(3).not().assertUndefined();
    })
    it('deepEquals_null_true',0, function () {
      // Defines a variety of assertion methods, which are used to declare expected boolean conditions.
      expect(null).assertDeepEquals(null)
    })
    it('deepEquals_array_not_have_true',0, function () {
      // Defines a variety of assertion methods, which are used to declare expected boolean conditions.
      const  a= []
      const  b= []
      expect(a).assertDeepEquals(b)
    })
    it('deepEquals_map_equal_length_success',0, function () {
      // Defines a variety of assertion methods, which are used to declare expected boolean conditions.
      const a =  new Map();
      const b =  new Map();
      a.set(1,100);
      a.set(2,200);
      b.set(1, 100);
      b.set(2, 200);
      expect(a).assertDeepEquals(b)
    })
    it("deepEquals_obj_success_1", 0, function () {
      const a = {x:1};
      const b = {x:1};
      expect(a).assertDeepEquals(b);
    })
    it("deepEquals_regExp_success_0", 0, function () {
      const a = new RegExp("/test/");
      const b = new RegExp("/test/");
      expect(a).assertDeepEquals(b)
    })
    it('test_isPending_pass_1', 0, function () {
      let p = new Promise(function () {
      });
      expect(p).assertPromiseIsPending();
    });
    it('test_isRejected_pass_1', 0, function () {
      let p = Promise.reject({
        bad: 'no'
      });
      expect(p).assertPromiseIsRejected();
    });
    it('test_isRejectedWith_pass_1', 0, function () {
      let p = Promise.reject({
        res: 'reject value'
      });
      expect(p).assertPromiseIsRejectedWith({
        res: 'reject value'
      });
    });
    it('test_isRejectedWithError_pass_1', 0, function () {
      let p1 = Promise.reject(new TypeError('number'));
      expect(p1).assertPromiseIsRejectedWithError(TypeError);
    });
    it('test_isResolved_pass_1', 0, function () {
      let p = Promise.resolve({
        res: 'result value'
      });
      expect(p).assertPromiseIsResolved();
    });
    it('test_isResolvedTo_pass_1', 0, function () {
      let p = Promise.resolve({
        res: 'result value'
      });
      expect(p).assertPromiseIsResolvedWith({
        res: 'result value'
      });
    });
    it('test_isPending_failed_1', 0, function () {
      let p = Promise.reject({
        bad: 'no1'
      });
      expect(p).assertPromiseIsPending();
    });
    it('test_isRejectedWithError_failed_1', 0, function () {
      let p = Promise.reject(new TypeError('number'));
      expect(p).assertPromiseIsRejectedWithError(TypeError, 'number one');
    });
  })
}
```


#### Mock能力

##### 约束限制

单元测试框架Mock能力从npm包[1.0.1版本](https://repo.harmonyos.com/#/cn/application/atomService/@ohos%2Fhypium)开始支持，需修改源码工程中package.info中配置依赖npm包版本号后使用。

-  **接口列表：**

| No. | API | 功能说明 |
| --- | --- | --- |
| 1 | mockFunc(obj: object, f：function()) | mock某个类的对象obj的函数f，那么需要传两个参数：obj和f，支持使用异步函数（说明：对mock而言原函数实现是同步或异步没太多区别，因为mock并不关注原函数的实现）。 |
| 2 | when(mockedfunc：function) | 对传入后方法做检查，检查是否被mock并标记过，返回的是一个方法声明。 |
| 3 | afterReturn(x：value) | 设定预期返回一个自定义的值value，比如某个字符串或者一个promise。 |
| 4 | afterReturnNothing() | 设定预期没有返回值，即 undefined。 |
| 5 | afterAction(x：action) | 设定预期返回一个函数执行的操作。 |
| 6 | afterThrow(x：msg) | 设定预期抛出异常，并指定异常msg。 |
| 7 | clear() | 用例执行完毕后，进行数据mocker对象的还原处理（还原之后对象恢复被mock之前的功能）。 |
| 8 | any | 设定用户传任何类型参数（undefined和null除外），执行的结果都是预期的值，使用ArgumentMatchers.any方式调用。 |
| 9 | anyString | 设定用户传任何字符串参数，执行的结果都是预期的值，使用ArgumentMatchers.anyString方式调用。 |
| 10 | anyBoolean | 设定用户传任何boolean类型参数，执行的结果都是预期的值，使用ArgumentMatchers.anyBoolean方式调用。 |
| 11 | anyFunction | 设定用户传任何function类型参数，执行的结果都是预期的值，使用ArgumentMatchers.anyFunction方式调用。 |
| 12 | anyNumber | 设定用户传任何数字类型参数，执行的结果都是预期的值，使用ArgumentMatchers.anyNumber方式调用。 |
| 13 | anyObj | 设定用户传任何对象类型参数，执行的结果都是预期的值，使用ArgumentMatchers.anyObj方式调用。 |
| 14 | matchRegexs(Regex) | 设定用户传任何正则表达式类型参数Regex，执行的结果都是预期的值，使用ArgumentMatchers.matchRegexs(Regex)方式调用。 |
| 15 | verify(methodName, argsArray) | 验证methodName（函数名字符串）所对应的函数和其参数列表argsArray的执行行为是否符合预期，返回一个VerificationMode：一个提供验证模式的类，它有times(count)、once()、atLeast(x)、atMost(x)、never()等函数可供选择。 |
| 16 | times(count) | 验证行为调用过count次。 |
| 17 | once() | 验证行为调用过一次。 |
| 18 | atLeast(count) | 验证行为至少调用过count次。 |
| 19 | atMost(count) | 验证行为至多调用过count次。 |
| 20 | never | 验证行为从未发生过。 |
| 21 | ignoreMock(obj, method) | 使用ignoreMock可以还原obj对象中被mock后的函数，对被mock后的函数有效。 |
| 22 | clearAll() | 用例执行完毕后，进行数据和内存清理。 |

-  **使用示例：**

用户可以通过以下方式进行引入mock模块进行测试用例编写：

- **须知：**
使用时候必须引入的mock能力模块： MockKit，when
根据自己用例需要引入断言能力api
例如：`import { describe, expect, it, MockKit, when} from '@ohos/hypium'`

**示例1：afterReturn 的使用**

```javascript
import {describe, expect, it, MockKit, when} from '@ohos/hypium';

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");

            //1.创建一个mock能力的对象MockKit
            let mocker = new MockKit();

            //2.定类ClassName，里面两个函数，然后创建一个对象claser
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

            //3.进行mock操作,比如需要对ClassName类的method_1函数进行mock
            let mockfunc = mocker.mockFunc(claser, claser.method_1);
            when(mockfunc)('test').afterReturn('1');

            //4.对mock后的函数进行断言，看是否符合预期
            //执行成功案例，参数为'test'
            expect(claser.method_1('test')).assertEqual('1'); //执行通过

            //执行失败案例，参数为 'abc'
            //expect(claser.method_1('abc')).assertEqual('1');//执行失败
        });
    });
}
```
- **须知：**
`when(mockfunc)('test').afterReturn('1');`
这句代码中的`('test')`是mock后的函数需要传递的匹配参数，目前支持一个参数
`afterReturn('1')`是用户需要预期返回的结果。
有且只有在参数是`('test')`的时候，执行的结果才是用户自定义的预期结果。

**示例2： afterReturnNothing 的使用**

```javascript
import {describe, expect, it, MockKit, when} from '@ohos/hypium';

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");

            //1.创建一个mock能力的对象MockKit
            let mocker = new MockKit();

            //2.定类ClassName，里面两个函数，然后创建一个对象claser
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

            //3.进行mock操作,比如需要对ClassName类的method_1函数进行mock
            let mockfunc = mocker.mockFunc(claser, claser.method_1);

            //4.根据自己需求进行选择 执行完毕后的动作，比如这里选择afterReturnNothing();即不返回任何值
            when(mockfunc)('test').afterReturnNothing();

            //5.对mock后的函数进行断言，看是否符合预期，注意选择跟第4步中对应的断言方法
            //执行成功案例，参数为'test'，这时候执行原对象claser.method_1的方法，会发生变化
            // 这时候执行的claser.method_1不会再返回'888888'，而是设定的afterReturnNothing()生效//不返回任何值;
            expect(claser.method_1('test')).assertUndefined(); //执行通过

            // 执行失败案例，参数传为 123
            // expect(method_1(123)).assertUndefined();//执行失败
        });
    });
}
```

**示例3： 设定参数类型为any ，即接受任何参数（undefine和null除外）的使用**


- **须知：**
需要引入ArgumentMatchers类，即参数匹配器，例如：ArgumentMatchers.any

```javascript
import {describe, expect, it, MockKit, when, ArgumentMatchers} from '@ohos/hypium';

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");

            //1.创建一个mock能力的对象MockKit
            let mocker = new MockKit();

            //2.定类ClassName，里面两个函数，然后创建一个对象claser
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

            //3.进行mock操作,比如需要对ClassName类的method_1函数进行mock
            let mockfunc = mocker.mockFunc(claser, claser.method_1);
            //根据自己需求进行选择参数匹配器和预期方法,
            when(mockfunc)(ArgumentMatchers.any).afterReturn('1');

            //4.对mock后的函数进行断言，看是否符合预期，注意选择跟第4步中对应的断言方法
            //执行成功的案例1，传参为字符串类型
            expect(claser.method_1('test')).assertEqual('1'); //用例执行通过。
            //执行成功的案例2，传参为数字类型123
            expect(claser.method_1(123)).assertEqual('1');//用例执行通过。
            //执行成功的案例3，传参为boolean类型true
            expect(claser.method_1(true)).assertEqual('1');//用例执行通过。

            //执行失败的案例，传参为数字类型空
            //expect(claser.method_1()).assertEqual('1');//用例执行失败。
        });
    });
}
```

**示例4： 设定参数类型为anyString,anyBoolean等的使用**

```javascript
import {describe, expect, it, MockKit, when, ArgumentMatchers} from '@ohos/hypium';

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");

            //1.创建一个mock能力的对象MockKit
            let mocker = new MockKit();

            //2.定类ClassName，里面两个函数，然后创建一个对象claser
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

            //3.进行mock操作,比如需要对ClassName类的method_1函数进行mock
            let mockfunc = mocker.mockFunc(claser, claser.method_1);
            //根据自己需求进行选择
            when(mockfunc)(ArgumentMatchers.anyString).afterReturn('1');

            //4.对mock后的函数进行断言，看是否符合预期，注意选择跟第4步中对应的断言方法
            //执行成功的案例，传参为字符串类型
            expect(claser.method_1('test')).assertEqual('1'); //用例执行通过。
            expect(claser.method_1('abc')).assertEqual('1'); //用例执行通过。

            //执行失败的案例，传参为数字类型
            //expect(claser.method_1(123)).assertEqual('1');//用例执行失败。
            //expect(claser.method_1(true)).assertEqual('1');//用例执行失败。
        });
    });
}
```

**示例5： 设定参数类型为matchRegexs（Regex）等 的使用**
```javascript
import {describe, expect, it, MockKit, when, ArgumentMatchers} from '@ohos/hypium';

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");

            //1.创建一个mock能力的对象MockKit
            let mocker = new MockKit();

            //2.定类ClassName，里面两个函数，然后创建一个对象claser
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

            //3.进行mock操作,比如需要对ClassName类的method_1函数进行mock
            let mockfunc = mocker.mockFunc(claser, claser.method_1);
            //根据自己需求进行选择，这里假设匹配正则，且正则为/123456/
            when(mockfunc)(ArgumentMatchers.matchRegexs(/123456/)).afterReturn('1');

            //4.对mock后的函数进行断言，看是否符合预期，注意选择跟第4步中对应的断言方法
            //执行成功的案例，传参为字符串 比如 '1234567898'
            expect(claser.method_1('1234567898')).assertEqual('1'); //用例执行通过。
            //因为字符串 '1234567898'可以和正则/123456/匹配上

            //执行失败的案例，传参为字符串'1234'
            //expect(claser.method_1('1234')).assertEqual('1');//用例执行失败。反之
        });
    });
}
```

**示例6： 验证功能 Verify函数的使用**
```javascript
import {describe, expect, it, MockKit, when} from '@ohos/hypium';

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");

            //1.创建一个mock能力的对象MockKit
            let mocker = new MockKit();

            //2.定类ClassName，里面两个函数，然后创建一个对象claser
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

            //3.进行mock操作,比如需要对ClassName类的method_1和method_2两个函数进行mock
            mocker.mockFunc(claser, claser.method_1);
            mocker.mockFunc(claser, claser.method_2);

            //4.方法调用如下
            claser.method_1('abc', 'ppp');
            claser.method_1('abc');
            claser.method_1('xyz');
            claser.method_1();
            claser.method_1('abc', 'xxx', 'yyy');
            claser.method_1();
            claser.method_2('111');
            claser.method_2('111', '222');

            //5.现在对mock后的两个函数进行验证，验证调用情况
            mocker.verify('method_1', []).atLeast(3); //结果为failed
            //解释：验证函数'method_1'，参数列表为空：[] 的函数，至少执行过3次，
            //执行结果为failed，因为'method_1'且无参数 在4中只执行过2次
            //mocker.verify('method_2',['111']).once();//执行success，原因同上
            //mocker.verify('method_2',['111',,'222']).once();//执行success，原因同上
        });
    });
}
```

**示例7：  ignoreMock(obj, method) 忽略函数的使用**
```javascript
import {describe, expect, it, MockKit, when, ArgumentMatchers} from '@ohos/hypium';

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");

            //1.创建一个mock能力的对象MockKit
            let mocker = new MockKit();

            //2.定类ClassName，里面两个函数，然后创建一个对象claser
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

            //3.进行mock操作,比如需要对ClassName类的method_1和method_2两个函数进行mock
            let func_1 = mocker.mockFunc(claser, claser.method_1);
            let func_2 = mocker.mockFunc(claser, claser.method_2);

            //4.对mock后的函数的行为进行修改
            when(func_1)(ArgumentMatchers.anyNumber).afterReturn('4');
            when(func_2)(ArgumentMatchers.anyNumber).afterReturn('5');

            //5.方法调用如下
            console.log(claser.method_1(123)); //执行结果是4，符合步骤4中的预期
            console.log(claser.method_2(456)); //执行结果是5，符合步骤4中的预期

            //6.现在对mock后的两个函数的其中一个函数method_1进行忽略处理（原理是就是还原）
            mocker.ignoreMock(claser, claser.method_1);
            //然后再去调用 claser.method_1函数，看执行结果
            console.log(claser.method_1(123)); //执行结果是888888，发现这时结果跟步骤4中的预期不一样了，执行了claser.method_1没被mock之前的结果
            //用断言测试
            expect(claser.method_1(123)).assertEqual('4'); //结果为failed 符合ignoreMock预期
            claser.method_2(456); //执行结果是5，因为method_2没有执行ignore忽略，所有也符合步骤4中的预期
        });
    });
}
```

**示例8：  clear（）函数的使用**

```javascript
import {describe, expect, it, MockKit, when, ArgumentMatchers} from '@ohos/hypium';

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");

            //1.创建一个mock能力的对象MockKit
            let mocker = new MockKit();

            //2.定类ClassName，里面两个函数，然后创建一个对象claser
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

            //3.进行mock操作,比如需要对ClassName类的method_1和method_2两个函数进行mock
            let func_1 = mocker.mockFunc(claser, claser.method_1);
            let func_2 = mocker.mockFunc(claser, claser.method_2);

            //4.对mock后的函数的行为进行修改
            when(func_1)(ArgumentMatchers.anyNumber).afterReturn('4');
            when(func_2)(ArgumentMatchers.anyNumber).afterReturn('5');

            //5.方法调用如下
            //expect(claser.method_1(123)).assertEqual('4');//ok 符合预期
            //expect(claser.method_2(456)).assertEqual('5');//ok 符合预期

            //6.清除mock操作（原理是就是还原）
            mocker.clear(claser);
            //然后再去调用 claser.method_1函数，看执行结果
            expect(claser.method_1(123)).assertEqual('4');//failed 符合预期
            expect(claser.method_2(456)).assertEqual('5');//failed 符合预期
        });
    });
}
```


**示例9：  afterThrow(msg) 函数的使用**

```javascript
import {describe, expect, it, MockKit, when} from '@ohos/hypium';

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");

            //1.创建一个mock能力的对象MockKit
            let mocker = new MockKit();

            //2.定类ClassName，里面两个函数，然后创建一个对象claser
            class ClassName {
                constructor() {
                }

                method_1(arg) {
                    return '888888';
                }
            }

            let claser = new ClassName();

            //3.进行mock操作,比如需要对ClassName类的method_1函数进行mock
            let mockfunc = mocker.mockFunc(claser, claser.method_1);

            //4.根据自己需求进行选择 执行完毕后的动作，比如这里选择afterReturnNothing();即不返回任何值
            when(mockfunc)('test').afterThrow('error xxx');

            //5.执行mock后的函数，捕捉异常并使用assertEqual对比msg否符合预期
            try {
                claser.method_1('test');
            } catch (e) {
                expect(e).assertEqual('error xxx');//执行通过
            }
        });
    });
}
```

**示例10：  mock异步 函数的使用**

```javascript
import {describe, expect, it, MockKit, when} from '@ohos/hypium';

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('testMockfunc', 0, function () {
            console.info("it1 begin");

            //1.创建一个mock能力的对象MockKit
            let mocker = new MockKit();

            //2.定类ClassName，里面两个函数，然后创建一个对象claser
            class ClassName {
                constructor() {
                }

                async method_1(arg) {
                    return new Promise((res, rej) => {
                        //做一些异步操作
                        setTimeout(function () {
                            console.log('执行');
                            res('数据传递');
                        }, 2000);
                    });
                }
            }

            let claser = new ClassName();

            //3.进行mock操作,比如需要对ClassName类的method_1函数进行mock
            let mockfunc = mocker.mockFunc(claser, claser.method_1);

            //4.根据自己需求进行选择 执行完毕后的动作，比如这里选择afterRetrun; 可以自定义返回一个promise
            when(mockfunc)('test').afterReturn(new Promise((res, rej) => {
                console.log("do something");
                res('success something');
            }));

            //5.执行mock后的函数，即对定义的promise进行后续执行
            claser.method_1('test').then(function (data) {
                //数据处理代码...
                console.log('result : ' + data);
            });
        });
    });
}
```

**示例11：mock 系统函数的使用**

```javascript
export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('test_systemApi', 0, function () {
            //1.创建MockKit对象
            let mocker = new MockKit();
            //2.mock app.getInfo函数
            let mockf = mocker.mockFunc(app, app.getInfo);
            when(mockf)('test').afterReturn('1');
            //执行成功案例
            expect(app.getInfo('test')).assertEqual('1');
        });
    });
}
```


**示例12：verify times函数的使用（验证函数调用次数）**

```javascript
import { describe, expect, it, MockKit, when } from '@ohos/hypium'

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('test_verify_times', 0, function () {
            //1.创建MockKit对象
            let mocker = new MockKit();
            //2.定义需要被mock的类
            class ClassName {
                constructor() {
                }

                method_1(...arg) {
                    return '888888';
                }
            }
            //3.创建类对象
            let claser = new ClassName();
            //4.mock 类ClassName对象的某个方法，比如method_1
            let func_1 = mocker.mockFunc(claser, claser.method_1);
            //5.期望被mock后的函数能够返回自己假设的结果
            when(func_1)('123').afterReturn('4');

            //6.随机执行几次函数，参数如下
            claser.method_1('123', 'ppp');
            claser.method_1('abc');
            claser.method_1('xyz');
            claser.method_1();
            claser.method_1('abc', 'xxx', 'yyy');
            claser.method_1('abc');
            claser.method_1();
            //7.验证函数method_1且参数为'abc'时，执行过的次数是否为2
            mocker.verify('method_1', ['abc']).times(2);
        });
    });
}
```


**示例13：  verify atLeast 函数的使用 （验证函数调用次数）**

```javascript
import { describe, expect, it, MockKit, when } from '@ohos/hypium'

export default function ActsAbilityTest() {
    describe('ActsAbilityTest', function () {
        it('test_verify_atLeast', 0, function () {
            //1.创建MockKit对象
            let mocker = new MockKit();
            //2.定义需要被mock的类
            class ClassName {
                constructor() {
                }

                method_1(...arg) {
                    return '888888';
                }
            }

            //3.创建类对象
            let claser = new ClassName();
            //4.mock  类ClassName对象的某个方法，比如method_1
            let func_1 = mocker.mockFunc(claser, claser.method_1);
            //5.期望被mock后的函数能够返回自己假设的结果
            when(func_1)('123').afterReturn('4');
            //6.随机执行几次函数，参数如下
            claser.method_1('123', 'ppp');
            claser.method_1('abc');
            claser.method_1('xyz');
            claser.method_1();
            claser.method_1('abc', 'xxx', 'yyy');
            claser.method_1();
            //7.验证函数method_1且参数为空时，是否至少执行过2次
            mocker.verify('method_1', []).atLeast(2);
        });
    });
}
```

#### 数据驱动

##### 约束限制

单元测试框架数据驱动能力从[hypium 1.0.2版本](https://repo.harmonyos.com/#/cn/application/atomService/@ohos%2Fhypium)开始支持。

- 数据参数传递 : 为指定测试套、测试用例传递测试输入数据参数。
- 压力测试 : 为指定测试套、测试用例设置执行次数。

数据驱动可以根据配置参数来驱动测试用例的执行次数和每一次传入的参数，使用时依赖data.json配置文件，文件内容如下：

>说明 : data.json与测试用例*.test.js|ets文件同目录

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

配置参数说明：

|      | 配置项名称 | 功能                                  | 必填 |
| :--- | :--------- | :------------------------------------ | ---- |
| 1    | "suite"    | 测试套配置 。                         | 是   |
| 2    | "items"    | 测试用例配置 。                       | 是   |
| 3    | "describe" | 测试套名称 。                         | 是   |
| 4    | "it"       | 测试用例名称 。                       | 是   |
| 5    | "params"   | 测试套 / 测试用例 可传入使用的参数 。 | 否   |
| 6    | "stress"   | 测试套 / 测试用例 指定执行次数 。     | 否   |

示例代码：

在TestAbility目录下app.js|ets文件中导入data.json，并在Hypium.hypiumTest() 方法执行前，设置参数数据

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

### 使用方式

单元测试框架以npm包（hypium）形式发布至[服务组件官网](https://repo.harmonyos.com/#/cn/application/atomService/@ohos%2Fhypium)，开发者可以下载Deveco Studio后，在应用工程中配置依赖后使用框架能力，测试工程创建及测试脚本执行使用指南请参见[IDE指导文档](https://developer.harmonyos.com/cn/docs/documentation/doc-guides/ohos-openharmony-test-framework-0000001267284568)。

## Ui测试框架功能特性

| No.  | 特性        | 功能说明                                                     |
| ---- | ----------- | ------------------------------------------------------------ |
| 1    | UiDriver    | Ui测试的入口，提供查找控件，检查控件存在性以及注入按键能力。 |
| 2    | By          | 用于描述目标控件特征(文本、id、类型等)，`UiDriver`根据`By`描述的控件特征信息来查找控件。 |
| 3    | UiComponent | UiDriver查找返回的控件对象，提供查询控件属性，滑动查找等触控和检视能力。 |
| 4    | UiWindow    | UiDriver查找返回的窗口对象，提供获取窗口属性、操作窗口的能力。 |

**使用者在测试脚本通过如下方式引入使用：**

```typescript
import {UiDriver,BY,UiComponent,Uiwindow,MatchPattern} from '@ohos.uitest'
```

> 须知
> 1. `By`类提供的接口全部是同步接口，使用者可以使用`builder`模式链式调用其接口构造控件筛选条件。
> 2. `UiDriver`和`UiComponent`类提供的接口全部是异步接口(`Promise`形式)，**需使用`await`语法**。
> 3. Ui测试用例均需使用**异步**语法编写用例，需遵循单元测试框架异步用例编写规范。

   

在测试用例文件中import `By/UiDriver/UiComponent`类，然后调用API接口编写测试用例。

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

### UiDriver使用说明

`UiDriver`类作为UiTest测试框架的总入口，提供查找控件，注入按键，单击坐标，滑动控件，手势操作，截图等能力。

| No.  | API                                                          | 功能描述                 |
| ---- | ------------------------------------------------------------ | ------------------------ |
| 1    | create():Promise<UiDriver>                                   | 静态方法，构造UiDriver。 |
| 2    | findComponent(b:By):Promise<UiComponent>                     | 查找匹配控件。           |
| 3    | pressBack():Promise<void>                                    | 单击BACK键。             |
| 4    | click(x:number, y:number):Promise<void>                      | 基于坐标点的单击。       |
| 5    | swipe(x1:number, y1:number, x2:number, y2:number):Promise<void> | 基于坐标点的滑动。       |
| 6    | assertComponentExist(b:By):Promise<void>                     | 断言匹配的控件存在。     |
| 7    | delayMs(t:number):Promise<void>                              | 延时。                   |
| 8    | screenCap(s:path):Promise<void>                              | 截屏。                   |
| 9    | findWindow(filter: WindowFilter): Promise<UiWindow>          | 查找匹配窗口。           |

其中assertComponentExist接口是断言API，用于断言当前界面存在目标控件；如果控件不存在，该API将抛出JS异常，使当前测试用例失败。

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

`UiDriver`完整的API列表请参考[API文档](https://gitee.com/openharmony/interface_sdk-js/blob/master/api/@ohos.uitest.d.ts)及[示例文档说明](https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis/js-apis-uitest.md#uidriver)。

### By使用说明

Ui测试框架通过`By`类提供了丰富的控件特征描述API，用来匹配查找要操作或检视的目标控件。`By`提供的API能力具有以下特点：

- 支持匹配单属性和匹配多属性组合，例如同时指定目标控件text和id。
- 控件属性支持多种匹配模式(等于，包含，`STARTS_WITH`，`ENDS_WITH`)。
- 支持相对定位控件，可通过`isBefore`和`isAfter`等API限定邻近控件特征进行辅助定位。

| No.  | API                                | 功能描述                                         |
| ---- | ---------------------------------- | ------------------------------------------------ |
| 1    | id(i:number):By                    | 指定控件id。                                     |
| 2    | text(t:string, p?:MatchPattern):By | 指定控件文本，可指定匹配模式。                   |
| 3    | type(t:string)):By                 | 指定控件类型。                                   |
| 4    | enabled(e:bool):By                 | 指定控件使能状态。                               |
| 5    | clickable(c:bool):By               | 指定控件可单击状态。                             |
| 6    | focused(f:bool):By                 | 指定控件获焦状态。                               |
| 7    | scrollable(s:bool):By              | 指定控件可滑动状态。                             |
| 8    | selected(s:bool):By                | 指定控件选中状态。                               |
| 9    | isBefore(b:By):By                  | **相对定位**，限定目标控件位于指定特征控件之前。 |
| 10   | isAfter(b:By):By                   | **相对定位**，限定目标控件位于指定特征控件之后。 |

其中，`text`属性支持{`MatchPattern.EQUALS`，`MatchPattern.CONTAINS`，`MatchPattern.STARTS_WITH`，`MatchPattern.ENDS_WITH`}四种匹配模式，缺省使用`MatchPattern.EQUALS`模式。

`By`完整的API列表请参考[API文档](https://gitee.com/openharmony/interface_sdk-js/blob/master/api/@ohos.uitest.d.ts)及[示例文档说明](https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis/js-apis-uitest.md#by)。

#### 控件绝对定位

**示例代码1**：查找id是`Id_button`的控件。

```javascript
let button = await driver.findComponent(BY.id(Id_button))
```

 **示例代码2**：查找id是`Id_button`并且状态是`enabled`的控件，适用于无法通过单一属性定位的场景。

```javascript
let button = await driver.findComponent(BY.id(Id_button).enabled(true))
```

通过`By.id(x).enabled(y)`来指定目标控件的多个属性。

**示例代码3**：查找文本中包含`hello`的控件，适用于不能完全确定控件属性取值的场景。

```javascript
let txt = await driver.findComponent(BY.text("hello", MatchPattern.CONTAINS))
```

通过向`By.text()`方法传入第二个参数`MatchPattern.CONTAINS`来指定文本匹配规则；默认规则是`MatchPattern.EQUALS`，即目标控件text属性必须严格等于给定值。

####  控件相对定位

**示例代码1**：查找位于文本控件`Item3_3`后面的，id是`ResourceTable.Id_switch`的Switch控件。

```javascript
let switch = await driver.findComponent(BY.id(Id_switch).isAfter(BY.text("Item3_3")))
```

通过`By.isAfter`方法，指定位于目标控件前面的特征控件属性，通过该特征控件进行相对定位。一般地，特征控件是某个具有全局唯一特征的控件(例如具有唯一的id或者唯一的text)。

类似的，可以使用`By.isBefore`控件指定位于目标控件后面的特征控件属性，实现相对定位。

### UiComponent使用说明

`UiComponent`类代表了Ui界面上的一个控件，一般是通过`UiDriver.findComponent(by)`方法查找到的。通过该类的实例，用户可以获取控件属性，单击控件，滑动查找，注入文本等操作。

`UiComponent`包含的常用API：

| No.  | API                               | 功能描述                                       |
| ---- | --------------------------------- | ---------------------------------------------- |
| 1    | click():Promise<void>             | 单击该控件。                                   |
| 2    | inputText(t:string):Promise<void> | 向控件中输入文本(适用于文本框控件)。           |
| 3    | scrollSearch(s:By):Promise<bool>  | 在该控件上滑动查找目标控件(适用于List等控件)。 |
| 4    | getText():Promise<string>         | 获取控件text。                                 |
| 5    | getId():Promise<number>           | 获取控件id。                                   |
| 6    | getType():Promise<string>         | 获取控件类型。                                 |
| 7    | isEnabled():Promise<bool>         | 获取控件使能状态。                             |

`UiComponent`完整的API列表请参考[API文档](https://gitee.com/openharmony/interface_sdk-js/blob/master/api/@ohos.uitest.d.ts)及[示例文档说明](https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis/js-apis-uitest.md#uicomponent)。

**示例代码1**：单击控件。

```javascript
let button = await driver.findComponent(BY.id(Id_button))
await button.click()
```

**示例代码2**：通过get接口获取控件属性后，可以使用单元测试框架提供的assert*接口做断言检查。

```javascript
let component = await driver.findComponent(BY.id(Id_title))
expect(component !== null).assertTrue()
```

**示例代码3**：在List控件中滑动查找text是`Item3_3`的子控件。

```javascript
let list = await driver.findComponent(BY.id(Id_list))
let found = await list.scrollSearch(BY.text("Item3_3"))
expect(found).assertTrue()
```

**示例代码4**：向输入框控件中输入文本。

```javascript
let editText = await driver.findComponent(BY.type('InputText'))
await editText.inputText("user_name")
```
### UiWindow使用说明

`UiWindow`类代表了Ui界面上的一个窗口，一般是通过`UiDriver.findWindow(by)`方法查找到的。通过该类的实例，用户可以获取窗口属性，并进行窗口拖动、调整窗口大小等操作。

`UiWindow`包含的常用API：

| No.  | API                                                          | 功能描述                                           |
| ---- | ------------------------------------------------------------ | -------------------------------------------------- |
| 1    | getBundleName(): Promise<string>                             | 获取窗口所属应用包名。                             |
| 2    | getTitle(): Promise<string>                                  | 获取窗口标题信息。                                 |
| 3    | focus(): Promise<bool>                                       | 使得当前窗口获取焦点。                             |
| 4    | moveTo(x: number, y: number): Promise<bool>;                 | 将当前窗口移动到指定位置（适用于支持移动的窗口）。 |
| 5    | resize(wide: number, height: number, direction: ResizeDirection): Promise<bool> | 调整窗口大小（适用于支持调整大小的窗口）。         |
| 6    | split(): Promise<bool>                                       | 将窗口模式切换为分屏模式(适用于支持分屏的窗口)。   |
| 7    | close(): Promise<bool>                                       | 关闭当前窗口。                                     |

`UiWindow`完整的API列表请参考[API文档](https://gitee.com/openharmony/interface_sdk-js/blob/master/api/@ohos.uitest.d.ts)及[示例文档说明](https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis/js-apis-uitest.md#uiwindow9)。

**示例代码1**：获取窗口属性。

```javascript
let window = await driver.findWindow({actived: true})
let bundelName = await window.getBundleName()
```

**示例代码2**：移动窗口。

```javascript
let window = await driver.findWindow({actived: true})
await window.moveTo(500,500)
```

**示例代码3**：关闭窗口。

```javascript
let window = await driver.findWindow({actived: true})
await window.close()
```

### 使用方式

  开发者可以下载Deveco Studio创建测试工程后，在其中调用框架提供接口进行相关测试操作，测试工程创建及测试脚本执行使用指南请参见[IDE指导文档](https://developer.harmonyos.com/cn/docs/documentation/doc-guides/ohos-openharmony-test-framework-0000001267284568)。
  UI测试框架使能需要执行如下命令。

>```shell
> hdc_std shell param set persist.ace.testmode.enabled 1
>```
### UI测试框架自构建方式

> Ui测试框架在OpenHarmony-3.1-Release版本中未随版本编译，需手动处理，请参考[3.1-Release版本使用指导](https://gitee.com/openharmony/arkXtest/blob/OpenHarmony-3.1-Release/README_zh.md#%E6%8E%A8%E9%80%81ui%E6%B5%8B%E8%AF%95%E6%A1%86%E6%9E%B6%E8%87%B3%E8%AE%BE%E5%A4%87)。

开发者如需自行编译Ui测试框架代码验证子修改内容，构建命令和推送位置请参考本章节内容。

#### 构建命令

```shell
./build.sh --product-name rk3568 --build-target uitestkit
```
#### 推送位置

```shell
hdc_std target mount
hdc_std shell mount -o rw,remount /
hdc_std file send uitest /system/bin/uitest
hdc_std file send libuitest.z.so /system/lib/module/libuitest.z.so
hdc_std shell chmod +x /system/bin/uitest
```

### 版本信息

| 版本号  | 功能说明                                                     |
| :------ | :----------------------------------------------------------- |
| 3.2.2.1 | 1、增加抛滑、获取/设置屏幕方向接口<br />2、窗口处理逻辑增加不支持场景处理逻辑 |
