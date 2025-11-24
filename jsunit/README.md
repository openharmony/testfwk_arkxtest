
| No.  | 特性     | 功能说明                                               |
| ---- | -------- |----------------------------------------------------|
| 1    | 基础流程 | 支持编写及异步执行基础用例。                                     |
| 2    | 断言库   | 判断用例实际结果值与预期值是否相符。                                 |
| 3    | Mock能力 | 支持函数级Mock能力，对定义的函数进行Mock后修改函数的行为，使其返回指定的值或者执行某种动作。 |
| 4    | 数据驱动 | 提供数据驱动能力，支持复用同一个测试脚本，使用不同输入数据驱动执行。                 |
| 5    | 专项能力 | 支持测试套与用例筛选、随机执行、压力测试、超时设置、遇错即停模式，跳过，支持测试套嵌套等。      |

### 使用说明

####  基础流程

测试用例采用业内通用语法，describe代表一个测试套， it代表一条用例。

| No. | API               | 功能说明                                                                   |
|-----| ----------------- |------------------------------------------------------------------------|
| 1   | describe          | 定义一个测试套，支持两个参数：测试套名称和测试套函数。其中测试套函数不能是异步函数。                             |
| 2   | beforeAll         | 在测试套内定义一个预置条件，在所有测试用例开始前执行且仅执行一次，支持一个参数：预置动作函数。                        |
| 3   | beforeEach        | 在测试套内定义一个单元预置条件，在每条测试用例开始前执行，执行次数与it定义的测试用例数一致，支持一个参数：预置动作函数。          |
| 4 | beforeEachIt | @since1.0.25 在测试套内定义一个单元预置条件，在每条测试用例开始前执行，支持一个参数：预置动作函数。<br />外层测试套定义的beforeEachIt会在内部测试套中的测试用例执行前执行。 |
| 5   | afterEach         | 在测试套内定义一个单元清理条件，在每条测试用例结束后执行，执行次数与it定义的测试用例数一致，支持一个参数：清理动作函数。          |
| 6 | afterEachIt | @since1.0.25 在测试套内定义一个单元预置条件，在每条测试用例结束后执行，支持一个参数：预置动作函数。<br />外层测试套定义的afterEachIt会在内部测试套中的测试用例执行结束后执行。 |
| 7   | afterAll          | 在测试套内定义一个清理条件，在所有测试用例结束后执行且仅执行一次，支持一个参数：清理动作函数。                        |
| 8   | beforeItSpecified | @since1.0.15在测试套内定义一个单元预置条件，仅在指定测试用例开始前执行，支持两个参数：单个用例名称或用例名称数组、预置动作函数。 |
| 9   | afterItSpecified  | @since1.0.15在测试套内定义一个单元清理条件，仅在指定测试用例结束后执行，支持两个参数：单个用例名称或用例名称数组、清理动作函数。 |
| 10  | it                | 定义一条测试用例，支持三个参数：用例名称，过滤参数和用例函数。                                        |
| 11  | expect            | 支持bool类型判断等多种断言方法。                                                     |
| 12 | xdescribe    | @since1.0.17定义一个跳过的测试套，支持两个参数：测试套名称和测试套函数。                             |
| 13 | xit                | @since1.0.17定义一条跳过的测试用例，支持三个参数：用例名称，过滤参数和用例函数。                         |


beforeItSpecified, afterItSpecified 示例代码：

```javascript
import { describe, it, expect, beforeItSpecified, afterItSpecified } from '@ohos/hypium';
export default function beforeItSpecifiedTest() {
  describe('beforeItSpecifiedTest', () => {
    beforeItSpecified(['String_assertContain_success'], () => {
      const num:number = 1;
      expect(num).assertEqual(1);
    })
    afterItSpecified(['String_assertContain_success'], async (done: Function) => {
      const str:string = 'abc';
      setTimeout(()=>{
        try {
          expect(str).assertContain('b');
        } catch (error) {
          console.error(`error message ${JSON.stringify(error)}`);
        }
        done();
      }, 1000)
    })
    it('String_assertContain_success', 0, () => {
      let a: string = 'abc';
      let b: string = 'b';
      expect(a).assertContain(b);
      expect(a).assertEqual(a);
    })
  })
}
```

beforeEachIt, afterEachIt 示例代码：

```javascript
import { describe, beforeEach, afterEach, beforeEachIt, afterEachIt, it, expect } from '@ohos/hypium';
let str = "";
export default function test() {
  describe('test0', () => {
    beforeEach(async () => {
      str += "A"
    })
    beforeEachIt(async () => {
      str += "B"
    })
    afterEach(async () => {
      str += "C"
    })
    afterEachIt(async () => {
      str += "D"
    })
    it('test0000', 0, () => {
      expect(str).assertEqual("BA");
    })
    describe('test1', () => {
      beforeEach(async () => {
        str += "E"
      })
      beforeEachIt(async () => {
        str += "F"
      })
      it('test1111', 0, async () => {
        expect(str).assertEqual("BACDBFE");
      })
    })
  })
}
```

####  断言库

##### 断言功能列表


| No. | API                | 功能说明                                                        |
|:----| :------------------|-------------------------------------------------------------|
| 1   | assertClose        | 检验actualvalue和expectvalue(0)的接近程度是否是expectValue(1)。         |
| 2   | assertContain      | 检验actualvalue中是否包含expectvalue。                              |
| 3   | assertEqual        | 检验actualvalue是否等于expectvalue[0]。                            |
| 4   | assertFail         | 抛出一个错误。                                                     |
| 5   | assertFalse        | 检验actualvalue是否是false。                                      |
| 6   | assertTrue         | 检验actualvalue是否是true。                                       |
| 7   | assertInstanceOf   | 检验actualvalue是否是expectvalue类型，支持基础类型。                       |
| 8   | assertLarger       | 检验actualvalue是否大于expectvalue。                               |
| 9   | assertLargerOrEqual       | 检验actualvalue是否大于等于expectvalue。                             |
| 10  | assertLess         | 检验actualvalue是否小于expectvalue。                               |
| 11  | assertLessOrEqual         | 检验actualvalue是否小于等于expectvalue。                             |
| 12  | assertNull         | 检验actualvalue是否是null。                                       |
| 13  | assertThrowError   | 检验actualvalue抛出Error内容是否是expectValue。                       |
| 14  | assertUndefined    | 检验actualvalue是否是undefined。                                  |
| 15  | assertNaN          | @since1.0.4 检验actualvalue是否是一个NaN。                          |
| 16  | assertNegUnlimited | @since1.0.4 检验actualvalue是否等于Number.NEGATIVE_INFINITY。      |
| 17  | assertPosUnlimited | @since1.0.4 检验actualvalue是否等于Number.POSITIVE_INFINITY。      |
| 18  | assertDeepEquals   | @since1.0.4 检验actualvalue和expectvalue是否完全相等。                |
| 19  | assertPromiseIsPending | @since1.0.4 判断promise是否处于Pending状态。                         |
| 20  | assertPromiseIsRejected | @since1.0.4 判断promise是否处于Rejected状态。                        |
| 21  | assertPromiseIsRejectedWith | @since1.0.4 判断promise是否处于Rejected状态，并且比较执行的结果值。             |
| 22  | assertPromiseIsRejectedWithError | @since1.0.4 判断promise是否处于Rejected状态并有异常，同时比较异常的类型和message值。 |
| 23  | assertPromiseIsResolved | @since1.0.4 判断promise是否处于Resolved状态。                        |
| 24  | assertPromiseIsResolvedWith | @since1.0.4 判断promise是否处于Resolved状态，并且比较执行的结果值。             |
| 25  | not                | @since1.0.4 断言取反,支持上面所有的断言功能。                               |
| 26  | message                | @since1.0.17自定义断言异常信息。                                      |

expect断言示例代码：

```javascript
import { describe, it, expect } from '@ohos/hypium';

export default function expectTest() {
  describe('expectTest', () => {
    it('assertCloseTest', 0, () => {
      let a: number = 100;
      let b: number = 0.1;
      expect(a).assertClose(99, b);
    })
    it('assertContain_1', 0, () => {
      let a = "abc";
      expect(a).assertContain('b');
    })
    it('assertContain_2', 0, () => {
      let a = [1, 2, 3];
      expect(a).assertContain(1);
    })
    it('assertEqualTest', 0, () => {
      expect(3).assertEqual(3);
    })
    it('assertFailTest', 0, () => {
      expect().assertFail() // 用例失败;
    })
    it('assertFalseTest', 0, () => {
      expect(false).assertFalse();
    })
    it('assertTrueTest', 0, () => {
      expect(true).assertTrue();
    })
    it('assertInstanceOfTest', 0, () => {
      let a: string = 'strTest';
      expect(a).assertInstanceOf('String');
    })
    it('assertLargerTest', 0, () => {
      expect(3).assertLarger(2);
    })
    it('assertLessTest', 0, () => {
      expect(2).assertLess(3);
    })
    it('assertNullTest', 0, () => {
      expect(null).assertNull();
    })
    it('assertThrowErrorTest', 0, () => {
      expect(() => {
        throw new Error('test')
      }).assertThrowError('test');
    })
    it('assertUndefinedTest', 0, () => {
      expect(undefined).assertUndefined();
    })
    it('assertLargerOrEqualTest', 0, () => {
      expect(3).assertLargerOrEqual(3);
    })
    it('assertLessOrEqualTest', 0, () => {
      expect(3).assertLessOrEqual(3);
    })
    it('assertNaNTest', 0, () => {
      expect(Number.NaN).assertNaN(); // true
    })
    it('assertNegUnlimitedTest', 0, () => {
      expect(Number.NEGATIVE_INFINITY).assertNegUnlimited(); // true
    })
    it('assertPosUnlimitedTest', 0, () => {
      expect(Number.POSITIVE_INFINITY).assertPosUnlimited(); // true
    })
    it('deepEquals_null_true', 0, () => {
      // Defines a variety of assertion methods, which are used to declare expected boolean conditions.
      expect(null).assertDeepEquals(null);
    })
    it('deepEquals_array_not_have_true', 0, () => {
      // Defines a variety of assertion methods, which are used to declare expected boolean conditions.
      const a: Array<number> = [];
      const b: Array<number> = [];
      expect(a).assertDeepEquals(b);
    })
    it('deepEquals_map_equal_length_success', 0, () => {
      // Defines a variety of assertion methods, which are used to declare expected boolean conditions.
      const a: Map<number, number> = new Map();
      const b: Map<number, number> = new Map();
      a.set(1, 100);
      a.set(2, 200);
      b.set(1, 100);
      b.set(2, 200);
      expect(a).assertDeepEquals(b);
    })
    it("deepEquals_obj_success_1", 0, () => {
      const a: SampleTest = {x: 1};
      const b: SampleTest = {x: 1};
      expect(a).assertDeepEquals(b);
    })
    it("deepEquals_regExp_success_0", 0, () => {
      const a: RegExp = new RegExp("/test/");
      const b: RegExp = new RegExp("/test/");
      expect(a).assertDeepEquals(b);
    })
    it('assertPromiseIsPendingTest', 0, async () => {
      let p: Promise<void> = new Promise<void>(() => {});
      await expect(p).assertPromiseIsPending();
    })
    it('assertPromiseIsRejectedTest', 0, async () => {
      let info: PromiseInfo = {res: "no"};
      let p: Promise<PromiseInfo> = Promise.reject(info);
      await expect(p).assertPromiseIsRejected();
    })
    it('assertPromiseIsRejectedWithTest', 0, async () => {
      let info: PromiseInfo = {res: "reject value"};
      let p: Promise<PromiseInfo> = Promise.reject(info);
      await expect(p).assertPromiseIsRejectedWith(info);
    })
    it('assertPromiseIsRejectedWithErrorTest', 0, async () => {
      let p1: Promise<TypeError> = Promise.reject(new TypeError('number'));
      await expect(p1).assertPromiseIsRejectedWithError(TypeError);
    })
    it('assertPromiseIsResolvedTest', 0, async () => {
      let info: PromiseInfo = {res: "result value"};
      let p: Promise<PromiseInfo> = Promise.resolve(info);
      await expect(p).assertPromiseIsResolved();
    })
    it('assertPromiseIsResolvedWithTest', 0, async () => {
      let info: PromiseInfo = {res: "result value"};
      let p: Promise<PromiseInfo> = Promise.resolve(info);
      await expect(p).assertPromiseIsResolvedWith(info);
    })
    it("test_message", 0, () => {
      expect(1).message('1 is not equal 2!').assertEqual(2); // fail
    })
  })
}

interface SampleTest {
  x: number;
}

interface PromiseInfo {
  res: string;
}
```

##### 自定义断言@since1.0.18

示例代码：

```javascript
import { describe, Assert, beforeAll, expect, Hypium, it } from '@ohos/hypium';

// custom.ets
interface customAssert extends Assert {
  // 自定义断言声明
  myAssertEqual(expectValue: boolean): void;
}

//自定义断言实现
let myAssertEqual = (actualValue: boolean, expectValue: boolean) => {
  interface R {
  pass: boolean,
  message: string
}

let result: R = {
  pass: true,
  message: 'just is a msg'
}

let compare = () => {
  if (expectValue === actualValue) {
    result.pass = true;
    result.message = '';
  } else {
    result.pass = false;
    result.message = 'expectValue !== actualValue!';
  }
  return result;
}
result = compare();
return result;
}

export default function customAssertTest() {
  describe('customAssertTest', () => {
    beforeAll(() => {
      //注册自定义断言，只有先注册才可以使用
      Hypium.registerAssert(myAssertEqual);
    })
    it('assertContain1', 0, () => {
      let a = true;
      let b = true;
      (expect(a) as customAssert).myAssertEqual(b);
      Hypium.unregisterAssert(myAssertEqual);
    })
    it('assertContain2', 0, () => {
      Hypium.registerAssert(myAssertEqual);
      let a = true;
      let b = true;
      (expect(a) as customAssert).myAssertEqual(b);
      // 注销自定义断言，注销以后就无法使用
      Hypium.unregisterAssert(myAssertEqual);
      try {
        (expect(a) as customAssert).myAssertEqual(b);
      }catch(e) {
        expect(e.message).assertEqual("myAssertEqual is unregistered");
      }
    })
  })
}
```

#### 异步代码测试

 **异步测试错误示例代码：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

async function callBack(fn: Function) {
   setTimeout(fn, 5000, 'done')
}

export default function callBackErrorTest() {
  describe('callBackErrorTest', () => {
    it('callBackErrorTest', 0, () => {
      callBack((result: string) => {
        try {
          // 用例失败
          expect().assertFail();
        } catch (e) {
        } finally {
        }
      })
    })
  })
}
```
> - 上述测试用例中,测试函数结束后, 回调函数才执行, 导致用例结果错误。
> - 当使用框架测试异步代码时,框架需要知道它测试的代码何时完成。以确保测试用例结果正常统计,测试框架用以下两种方式来处理这个问题。

##### Async/Await
> 使用 Async关键字定义一个异步测试函数,在测试函数中使用await等待测试函数完成。

**Promise示例代码：**

```javascript
import { describe, it, expect } from '@ohos/hypium';

async function method_1() {
  return new Promise<string>((res: Function, rej: Function) => {
    //做一些异步操作
    setTimeout(() => {
      console.log('执行');
      res('method_1_call');
    }, 5000);
  });
}

export default function abilityTest() {
  describe('ActsAbilityTest', () => {
    it('assertContain', 0, async () => {
      let result = await method_1();
      expect(result).assertEqual('method_1_call');
    })
  })
}
```
##### done 函数
>  - done函数是测试函数的一个可选回调参数,在测试用例中手动调用,测试框架将等待done回调被调用,然后才完成测试。
>  - 当测试函数中定义done函数参数时,测试用例中必须手动调用done函数,否则用例失败会出现超时错误。


**Promise回调示例代码：**
```javascript
import { describe, it, expect } from '@ohos/hypium';
async function method_1() {
  return new Promise<string>((res: Function, rej: Function) => {
    //做一些异步操作
    setTimeout(() => {
      console.log('执行');
      res('method_1_call');
    }, 5000);
  });
}

export default function abilityTest() {
  describe('Promise_Done', () => {
    it('Promise_Done', 0,(done: Function) => {
      method_1().then((result: string) => {
        try {
          expect(result).assertEqual('method_1_call');
        } catch (e) {
        } finally {
          // 调用done函数,用例执行完成,必须手动调用done函数,否则出现超时错误。
          done()
        }
      })

    })
  })
}
```

**回调函数示例代码：**
```javascript
import { describe, it, expect } from '@ohos/hypium';

async function callBack(fn: Function) {
   setTimeout(fn, 5000, 'done')
}

export default function callBackTestTest() {
  describe('CallBackTest', () => {
    it('CallBackTest_001', 0, (done: Function) => {
      callBack( (result: string) => {
        try {
          expect(result).assertEqual('done');
        } catch (e) {
        } finally {
          // 调用done函数,用例执行完成,必须手动调用done函数,否则出现超时错误。
          done()
        }
      })
    })
  })
}
```

#### SysTestKit的公共能力

| No. | API                | 功能说明              |
|:----| :------------------|-------------------|
| 1   | getDescribeName        | 获取当前测试用例所属的测试套名称。 |
| 2   | getItName        | 获取当初测试用例名称。       |
| 3   | getItAttribute        | 获取当初测试用例等级。       |
| 4   | actionStart        | 添加用例执行过程打印自定义日志。  |
| 5   | actionEnd        | 添加用例执行过程打印自定义日志。  |
| 6   | existKeyword        | 检测hilog日志中是否打印，仅支持检测单行日志。 |
| 7 | clearLog | 清理被测样机的hilog缓存。 |

##### 获取当前测试用例所属的测试套名称

示例代码：
```javascript
import { describe, it, expect, SysTestKit } from '@ohos/hypium';
import hilog from '@ohos.hilog';
const domain = 0;
const tag = 'SysTestKitTest'
export default function abilityTest() {
  describe('SysTestKitTest', () => {

    it("testGetDescribeName", 0, () => {
      hilog.debug(domain, tag, `testGetDescribeName start`);
      const describeName = SysTestKit.getDescribeName();
      hilog.debug(domain, tag, `testGetDescribeName describeName, ${describeName}`);
      expect(describeName).assertEqual('SysTestKitTest');
      hilog.debug(domain, tag, `testGetDescribeName end`);
    })
  })
}
```

##### 获取当初测试用例名称

示例代码：
```javascript
import { describe, it, expect, SysTestKit } from '@ohos/hypium';
import hilog from '@ohos.hilog';
const domain = 0;
const tag = 'SysTestKitTest'
export default function abilityTest() {
  describe('SysTestKitTest', () => {

    it("testGetItName", 0, () => {
      hilog.debug(domain, tag, `testGetDescribeName start`);
      const itName = SysTestKit.getItName();
      hilog.debug(domain, tag, `testGetDescribeName itName, ${itName}`);
      expect(itName).assertEqual('testGetItName');
      hilog.debug(domain, tag, `testGetDescribeName end`);
    })
  })
}
```

##### 获取当初测试用例级别

示例代码：
```javascript
import { describe, it, expect, SysTestKit, TestType, Level, Size } from '@ohos/hypium';
import hilog from '@ohos.hilog';
const domain = 0;
const tag = 'SysTestKitTest'
export default function abilityTest() {
  describe('SysTestKitTest', () => {

    it("testGetItAttribute", TestType.FUNCTION | Size.SMALLTEST | Level.LEVEL0, () => {
      hilog.debug(domain, tag, `testGetItAttribute start`);
      const testType: TestType | Size | Level = SysTestKit.getItAttribute();
      hilog.debug(domain, tag, `testGetDescribeName testType, ${ testType }`);
      expect(testType).assertEqual(TestType.FUNCTION | Size.SMALLTEST | Level.LEVEL0);
      hilog.debug(domain, tag, `testGetItAttribute end`);
    })
  })
}
```

##### 添加自定义打印日志。

示例代码：
```javascript
import { describe, it, expect, SysTestKit, TestType, Level, Size } from '@ohos/hypium';
import hilog from '@ohos.hilog';
const domain = 0;
const tag = 'SysTestKitTest'
export default function abilityTest() {
  describe('SysTestKitTest', () => {

    it("testActionStart", TestType.FUNCTION | Size.SMALLTEST | Level.LEVEL0, () => {
      hilog.debug(domain, tag, `testActionStart start `);
      SysTestKit.actionStart('testActionStart 自定义日志 ');
      SysTestKit.actionEnd('testActionStart end 自定义日志 ');
      hilog.debug(domain, tag, `testActionStart end`);
    })
  })
}
```

##### 检测hilog日志中是否打印。

示例代码：
```javascript
import { describe, it, expect, SysTestKit, TestType, Level, Size } from '@ohos/hypium';
import hilog from '@ohos.hilog';
const domain = 0;
const tag = 'SysTestKitTest'

function logTest() {
  hilog.info(domain, 'test', `logTest called selfTest`);
}

export default function abilityTest() {
  describe('SysTestKitTest', () => {

    it("testExistKeyword", TestType.FUNCTION | Size.SMALLTEST | Level.LEVEL0, async () => {
      await SysTestKit.clearLog();
      hilog.debug(domain, tag, `testExistKeyword start `);
      logTest();
      const isCalled = await SysTestKit.existKeyword('logTest');
      hilog.debug(domain, tag, `testExistKeyword isCalled, ${isCalled} `);
      expect(isCalled).assertTrue();
      hilog.debug(domain, tag, `testExistKeyword end`);
    })
  })
}
```

#### Mock能力

##### 约束限制

单元测试框架Mock能力从npm包[1.0.1版本](https://ohpm.openharmony.cn/#/cn/detail/@ohos%2Fhypium)开始支持，需修改源码工程中package.info中配置依赖npm包版本号后使用。
> -  仅支持Mock自定义对象,不支持Mock系统API对象。
> -  不支持Mock对象的私有函数。 
-  **接口列表：**
  
**Mockit相关接口**
  
Mockit是Mock的基础类，用于指定需要Mock的实例和方法。
| No. | API | 功能说明                                                                                                                                            |
| --- | --- |-------------------------------------------------------------------------------------------------------------------------------------------------|
| 1 | mockFunc| Mock某个类的实例上的公共方法（如需Mock私有方法，使用下面的mockPrivateFunc），支持使用异步函数（说明：对Mock而言原函数实现是同步或异步没太多区别，因为Mock并不关注原函数的实现）。                                                        |
| 2 | mockPrivateFunc | Mock某个类的实例上的私有方法，支持使用异步函数。 |
| 3 | mockProperty | Mock某个类的实例上的属性，将其值设置为预期值，支持私有属性。 |
| 4 | verify | 验证函数在对应参数下的执行行为是否符合预期，返回一个VerificationMode类。 |
| 5 | ignoreMock | 使用ignoreMock可以还原实例中被Mock后的函数/属性，对被Mock后的函数/属性有效。                                                                                                   |
| 6 | clear | 用例执行完毕后，进行被Mock的实例进行还原处理（还原之后对象恢复被Mock之前的功能）。                                                                                                  |
| 7 | clearAll | 用例执行完毕后，进行数据和内存清理,不会还原实例中被Mock后的函数。                                                                                                                  |  
  
**VerificationMode相关接口**
  
VerificationMode用于验证被Mock的函数的被调用次数。
| No. | API | 功能说明                                                                                                                                            |
| --- | --- |-------------------------------------------------------------------------------------------------------------------------------------------------|
| 1 | times | 验证函数被调用过的次数符合预期。                                                                                                                                  |
| 2 | once | 验证函数被调用过一次。                                                                                                                                      |
| 3 | atLeast | 验证函数至少被调用的次数符合预期。                                                                                                                                |
| 4 | atMost | 验证函数至多被调用的次数符合预期。                                                                                                                               |
| 5 | never | 验证函数从未被被调用过。                                                                                                                                |
  
**when相关接口**
  
when是一个函数，用于设置函数期望被Mock的值，使用when方法后需要使用afterXXX方法设置函数被Mock后的返回值或操作。
| No. | API | 功能说明                                                                                                                                            |
| --- | --- |-------------------------------------------------------------------------------------------------------------------------------------------------|
| 1 | when | 对传入后方法做检查，检查是否被Mock并标记过，返回一个函内置函数，函数执行后返回一个类用于设置Mock值。                                                                                                             |
  
设置Mock值的类相关接口如下：
| No. | API | 功能说明                                                                                                                                            |
| --- | --- |-------------------------------------------------------------------------------------------------------------------------------------------------|
| 1 | afterReturn | 设定预期返回一个自定义的值，比如某个字符串或者一个promise。                                                                                                          |
| 2 | afterReturnNothing| 设定预期没有返回值，即 undefined。                                                                                                                          |
| 3 | afterAction | 设定预期返回一个函数执行的操作。                                                                                                                                |
| 4 | afterThrow | 设定预期抛出异常，并指定异常的message。                                                                                                                              |

**ArgumentMatchers相关接口**
  
ArgumentMatchers用于用户自定义函数参数，它的接口以枚举值或是函数的形式使用。
| No. | API | 功能说明                                                                                                                                            |
| --- | --- |-------------------------------------------------------------------------------------------------------------------------------------------------|
| 1 | any | 设定用户传任何类型参数（undefined和null除外），执行的结果都是预期的值，使用ArgumentMatchers.any方式调用。                                                                           |
| 2 | anyString | 设定用户传任何字符串参数，执行的结果都是预期的值，使用ArgumentMatchers.anyString方式调用。                                                                                      |
| 3 | anyBoolean | 设定用户传任何boolean类型参数，执行的结果都是预期的值，使用ArgumentMatchers.anyBoolean方式调用。                                                                               |
| 4 | anyFunction | 设定用户传任何function类型参数，执行的结果都是预期的值，使用ArgumentMatchers.anyFunction方式调用。                                                                             |
| 5 | anyNumber | 设定用户传任何数字类型参数，执行的结果都是预期的值，使用ArgumentMatchers.anyNumber方式调用。                                                                                     |
| 6 | anyObj | 设定用户传任何对象类型参数，执行的结果都是预期的值，使用ArgumentMatchers.anyObj方式调用。                                                                                        |
| 7 | matchRegexs | 设定用户传任何符合正则表达式验证的参数，执行的结果都是预期的值，使用ArgumentMatchers.matchRegexs(Regex)方式调用。                                                                    |


-  **使用示例：**

用户可以通过以下方式引入Mock模块进行测试用例编写：

- **须知：**
使用时候必须引入的Mock能力模块： MockKit，when，根据自己用例需要引入断言能力api。
例如：`import { describe, expect, it, MockKit, when} from '@ohos/hypium'`

**示例1: afterReturn 的使用**

```javascript
import { describe, expect, it, MockKit, when } from '@ohos/hypium';

class ClassName {
  constructor() {
  }

  method_1(arg: string) {
    return '888888';
  }

  method_2(arg: string) {
    return '999999';
  }
}
export default function afterReturnTest() {
  describe('afterReturnTest', () => {
    it('afterReturnTest', 0, () => {
      console.info("it1 begin");
      // 1.创建一个Mock能力的对象MockKit
      let mocker: MockKit = new MockKit();
      // 2.定类ClassName，里面两个函数，然后创建一个对象claser
      let claser: ClassName = new ClassName();
      // 3.进行Mock操作,比如需要对ClassName类的method_1函数进行Mock
      let mockfunc: Function = mocker.mockFunc(claser, claser.method_1);
      // 4.期望claser.method_1函数被Mock后, 以'test'为入参时调用函数返回结果'1'
      when(mockfunc)('test').afterReturn('1');
      // 5.对Mock后的函数进行断言，看是否符合预期
      // 执行成功案例，参数为'test'
      expect(claser.method_1('test')).assertEqual('1'); // 执行通过
    })
  })
}
```
- **须知：**
`when(mockfunc)('test').afterReturn('1');`  
这句代码中的`('test')`是Mock后的函数需要传递的匹配参数，目前支持传递多个参数。  
`afterReturn('1')`是用户需要预期返回的结果。  
有且只有在参数是`('test')`的时候，执行的结果才是用户自定义的预期结果。

**示例2: afterReturnNothing 的使用**

```javascript
import { describe, expect, it, MockKit, when } from '@ohos/hypium';

class ClassName {
  constructor() {
  }

  method_1(arg: string) {
    return '888888';
  }

  method_2(arg: string) {
    return '999999';
  }
}
export default function  afterReturnNothingTest() {
  describe('afterReturnNothingTest', () => {
    it('testMockfunc', 0, () => {
      console.info("it1 begin");
      // 1.创建一个Mock能力的对象MockKit
      let mocker: MockKit = new MockKit();
      // 2.定类ClassName，里面两个函数，然后创建一个对象claser
      let claser: ClassName = new ClassName();
      // 3.进行Mock操作,比如需要对ClassName类的method_1函数进行Mock
      let mockfunc: Function = mocker.mockFunc(claser, claser.method_1);
      // 4.期望claser.method_1函数被Mock后, 以'test'为入参时调用函数返回结果undefined
      when(mockfunc)('test').afterReturnNothing();
      // 5.对Mock后的函数进行断言，看是否符合预期，注意选择跟第4步中对应的断言方法
      // 执行成功案例，参数为'test'，这时候执行原对象claser.method_1的方法，会发生变化
      // 这时候执行的claser.method_1不会再返回'888888'，而是设定的afterReturnNothing()生效// 不返回任何值;
      expect(claser.method_1('test')).assertUndefined(); // 执行通过
    })
  })
}
```

**示例3: 设定参数类型为any ，即接受任何参数（undefined和null除外）的使用**


- **须知：**
需要引入ArgumentMatchers类，即参数匹配器，例如：ArgumentMatchers.any

```javascript
import { describe, expect, it, MockKit, when, ArgumentMatchers } from '@ohos/hypium';

class ClassName {
  constructor() {
  }

  method_1(arg: string) {
    return '888888';
  }

  method_2(arg: string) {
    return '999999';
  }
}
export default function argumentMatchersAnyTest() {
  describe('argumentMatchersAnyTest', () => {
    it('testMockfunc', 0, () => {
      console.info("it1 begin");
      // 1.创建一个Mock能力的对象MockKit
      let mocker: MockKit = new MockKit();
      // 2.定类ClassName，里面两个函数，然后创建一个对象claser
      let claser: ClassName = new ClassName();
      // 3.进行Mock操作,比如需要对ClassName类的method_1函数进行Mock
      let mockfunc: Function = mocker.mockFunc(claser, claser.method_1);
      // 4.期望claser.method_1函数被Mock后, 以任何参数调用函数时返回结果'1'
      when(mockfunc)(ArgumentMatchers.any).afterReturn('1');
      // 5.对Mock后的函数进行断言，看是否符合预期，注意选择跟第4步中对应的断言方法
      // 执行成功的案例1，传参为字符串类型
      expect(claser.method_1('test')).assertEqual('1'); // 用例执行通过。
      // 执行成功的案例2，传参为数字类型123
      expect(claser.method_1("123")).assertEqual('1');// 用例执行通过。
      // 执行成功的案例3，传参为boolean类型true
      expect(claser.method_1("true")).assertEqual('1');// 用例执行通过。
    })
  })
}
```

**示例4: 设定参数类型ArgumentMatchers的使用**

```javascript
import { describe, expect, it, MockKit, when, ArgumentMatchers } from '@ohos/hypium';

class ClassName {
  constructor() {
  }

  method_1(arg: string) {
    return '888888';
  }

  method_2(arg: string) {
    return '999999';
  }
}
export default function argumentMatchersTest() {
  describe('argumentMatchersTest', () => {
    it('testMockfunc', 0, () => {
      console.info("it1 begin");
      // 1.创建一个Mock能力的对象MockKit
      let mocker: MockKit = new MockKit();
      // 2.定类ClassName，里面两个函数，然后创建一个对象claser
      let claser: ClassName = new ClassName();
      // 3.进行Mock操作,比如需要对ClassName类的method_1函数进行Mock
      let mockfunc: Function = mocker.mockFunc(claser, claser.method_1);
      // 4.期望claser.method_1函数被Mock后, 以任何string类型为参数调用函数时返回结果'1'
      when(mockfunc)(ArgumentMatchers.anyString).afterReturn('1');
      // 4.对Mock后的函数进行断言，看是否符合预期，注意选择跟第4步中对应的断言方法
      // 执行成功的案例，传参为字符串类型
      expect(claser.method_1('test')).assertEqual('1'); // 用例执行通过。
      expect(claser.method_1('abc')).assertEqual('1'); // 用例执行通过。
    })
  })
}
```

**示例5: 设定参数类型为matchRegexs（Regex）等 的使用**
```javascript
import { describe, expect, it, MockKit, when, ArgumentMatchers } from '@ohos/hypium';

class ClassName {
  constructor() {
  }

  method_1(arg: string) {
    return '888888';
  }

  method_2(arg: string) {
    return '999999';
  }
}
export default function matchRegexsTest() {
  describe('matchRegexsTest', () => {
    it('testMockfunc', 0, () => {
      console.info("it1 begin");
      // 1.创建一个Mock能力的对象MockKit
      let mocker: MockKit = new MockKit();
      let claser: ClassName = new ClassName();
      // 2.进行Mock操作,比如需要对ClassName类的method_1函数进行Mock
      let mockfunc: Function = mocker.mockFunc(claser, claser.method_1);
      // 3.期望claser.method_1函数被Mock后, 以"test"为入参调用函数时返回结果'1'
      when(mockfunc)(ArgumentMatchers.matchRegexs(new RegExp("test"))).afterReturn('1');
      // 4.对Mock后的函数进行断言，看是否符合预期，注意选择跟第4步中对应的断言方法
      // 执行成功的案例，传参为字符串类型
      expect(claser.method_1('test')).assertEqual('1'); // 用例执行通过。
    })
  })
}
```

**示例6: 验证功能 Verify函数的使用**
```javascript
import { describe, it, MockKit } from '@ohos/hypium';

class ClassName {
  constructor() {
  }

  method_1(...arg: string[]) {
    return '888888';
  }

  method_2(...arg: string[]) {
    return '999999';
  }
}
export default function verifyTest() {
  describe('verifyTest', () => {
    it('testMockfunc', 0, () => {
      console.info("it1 begin");
      // 1.创建一个Mock能力的对象MockKit
      let mocker: MockKit = new MockKit();
      // 2.然后创建一个对象claser
      let claser: ClassName = new ClassName();
      // 3.进行Mock操作,比如需要对ClassName类的method_1和method_2两个函数进行Mock
      mocker.mockFunc(claser, claser.method_1);
      mocker.mockFunc(claser, claser.method_2);
      // 4.方法调用如下
      claser.method_1('abc', 'ppp');
      claser.method_1('abc');
      claser.method_1('xyz');
      claser.method_1();
      claser.method_1('abc', 'xxx', 'yyy');
      claser.method_1();
      claser.method_2('111');
      claser.method_2('111', '222');
      // 5.现在对Mock后的两个函数进行验证，验证method_2,参数为'111'执行过一次
      mocker.verify('method_2',['111']).once(); // 执行success
    })
  })
}
```

**示例7: ignoreMock(obj, method) 忽略函数的使用**
```javascript
import { describe, expect, it, MockKit, when, ArgumentMatchers } from '@ohos/hypium';

class ClassName {
  constructor() {
  }

  method_1(...arg: number[]) {
    return '888888';
  }

  method_2(...arg: number[]) {
    return '999999';
  }
}
export default function ignoreMockTest() {
  describe('ignoreMockTest', () => {
    it('testMockfunc', 0, () => {
      console.info("it1 begin");
      // 1.创建一个Mock能力的对象MockKit
      let mocker: MockKit = new MockKit();
      // 2.创建一个对象claser
      let claser: ClassName = new ClassName();
      // 3.进行Mock操作,比如需要对ClassName类的method_1和method_2两个函数进行Mock
      let func_1: Function = mocker.mockFunc(claser, claser.method_1);
      let func_2: Function = mocker.mockFunc(claser, claser.method_2);
      // 4.期望claser.method_1函数被Mock后, 以number类型为入参时调用函数返回结果'4'
      when(func_1)(ArgumentMatchers.anyNumber).afterReturn('4');
      // 4.期望claser.method_2函数被Mock后, 以number类型为入参时调用函数返回结果'5'
      when(func_2)(ArgumentMatchers.anyNumber).afterReturn('5');
      // 5.方法调用如下
      expect(claser.method_1(123)).assertEqual("4");
      expect(claser.method_2(456)).assertEqual("5");
      // 6.现在对Mock后的两个函数的其中一个函数method_1进行忽略处理（原理是就是还原）
      mocker.ignoreMock(claser, claser.method_1);
      // 7.然后再去调用 claser.method_1函数，用断言测试結果
      expect(claser.method_1(123)).assertEqual('888888');
    })
  })
}
```

**示例8: clear(obj)函数的使用**

```javascript
import { describe, expect, it, MockKit, when, ArgumentMatchers } from '@ohos/hypium';

class ClassName {
  constructor() {
  }

  method_1(...arg: number[]) {
    return '888888';
  }

  method_2(...arg: number[]) {
    return '999999';
  }
}
export default function clearTest() {
  describe('clearTest', () => {
    it('testMockfunc', 0, () => {
      console.info("it1 begin");
      // 1.创建一个Mock能力的对象MockKit
      let mocker: MockKit = new MockKit();
      // 2.创建一个对象claser
      let claser: ClassName = new ClassName();
      // 3.进行Mock操作,比如需要对ClassName类的method_1和method_2两个函数进行Mock
      let func_1: Function = mocker.mockFunc(claser, claser.method_1);
      let func_2: Function = mocker.mockFunc(claser, claser.method_2);
      // 4.期望claser.method_1函数被Mock后, 以任何number类型为参数调用函数时返回结果'4'
      when(func_1)(ArgumentMatchers.anyNumber).afterReturn('4');
      // 4.期望claser.method_2函数被Mock后, 以任何number类型为参数调用函数时返回结果'5'
      when(func_2)(ArgumentMatchers.anyNumber).afterReturn('5');
      // 5.方法调用如下
      expect(claser.method_1(123)).assertEqual('4');
      expect(claser.method_2(123)).assertEqual('5');
      // 6.清除obj上所有的Mock能力（原理是就是还原）
      mocker.clear(claser);
      // 7.然后再去调用 claser.method_1,claser.method_2 函数，测试结果
      expect(claser.method_1(123)).assertEqual('888888');
      expect(claser.method_2(123)).assertEqual('999999');
    })
  })
}
```

**示例9: afterThrow(msg)函数的使用**

```javascript
import { describe, expect, it, MockKit, when } from '@ohos/hypium';

class ClassName {
  constructor() {
  }

  method_1(arg: string) {
    return '888888';
  }
}
export default function afterThrowTest() {
  describe('afterThrowTest', () => {
    it('testMockfunc', 0, () => {
      console.info("it1 begin");
      // 1.创建一个Mock能力的对象MockKit
      let mocker: MockKit = new MockKit();
      // 2.创建一个对象claser
      let claser: ClassName = new ClassName();
      // 3.进行Mock操作,比如需要对ClassName类的method_1函数进行Mock
      let mockfunc: Function = mocker.mockFunc(claser, claser.method_1);
      // 4.期望claser.method_1函数被Mock后, 以'test'为参数调用函数时抛出error xxx异常
      when(mockfunc)('test').afterThrow('error xxx');
      // 5.执行Mock后的函数，捕捉异常并使用assertEqual对比msg否符合预期
      try {
        claser.method_1('test');
      } catch (e) {
        expect(e).assertEqual('error xxx'); // 执行通过
      }
    })
  })
}
```

**示例10： Mock异步返回promise对象的使用**

```javascript
import { describe, expect, it, MockKit, when } from '@ohos/hypium';

class ClassName {
  constructor() {
  }

  async method_1(arg: string) {
    return new Promise<string>((resolve: Function, reject: Function) => {
      setTimeout(() => {
        console.log('执行');
        resolve('数据传递');
      }, 2000);
    });
  }
}
export default function mockPromiseTest() {
  describe('mockPromiseTest', () => {
    it('testMockfunc', 0, async (done: Function) => {
      console.info("it1 begin");
      // 1.创建一个Mock能力的对象MockKit
      let mocker: MockKit = new MockKit();
      // 2.创建一个对象claser
      let claser: ClassName = new ClassName();
      // 3.进行Mock操作,比如需要对ClassName类的method_1函数进行Mock
      let mockfunc: Function = mocker.mockFunc(claser, claser.method_1);
      // 4.期望claser.method_1函数被Mock后, 以'test'为参数调用函数时返回一个promise对象
      when(mockfunc)('test').afterReturn(new Promise<string>((resolve: Function, reject: Function) => {
        console.log("do something");
        resolve('success something');
      }));
      // 5.执行Mock后的函数，即对定义的promise进行后续执行
      let result = await claser.method_1('test');
      expect(result).assertEqual("success something");
      done();
    })
  })
}
```

**示例11：verify times函数的使用（验证函数调用次数）**

```javascript
import { describe, it, MockKit, when } from '@ohos/hypium'

class ClassName {
  constructor() {
  }

  method_1(...arg: string[]) {
    return '888888';
  }
}
export default function verifyTimesTest() {
  describe('verifyTimesTest', () => {
    it('test_verify_times', 0, () => {
      // 1.创建MockKit对象
      let mocker: MockKit = new MockKit();
      // 2.创建类对象
      let claser: ClassName = new ClassName();
      // 3.Mock 类ClassName对象的某个方法，比如method_1
      let func_1: Function = mocker.mockFunc(claser, claser.method_1);
      // 4.期望被Mock后的函数返回结果'4'
      when(func_1)('123').afterReturn('4');
      // 5.随机执行几次函数，参数如下
      claser.method_1('123', 'ppp');
      claser.method_1('abc');
      claser.method_1('xyz');
      claser.method_1();
      claser.method_1('abc', 'xxx', 'yyy');
      claser.method_1('abc');
      claser.method_1();
      // 6.验证函数method_1且参数为'abc'时，执行过的次数是否为2
      mocker.verify('method_1', ['abc']).times(2);
    })
  })
}
```

**示例12：verify atLeast函数的使用(验证函数调用次数)**

```javascript
import { describe, it, MockKit, when } from '@ohos/hypium'

class ClassName {
  constructor() {
  }

  method_1(...arg: string[]) {
    return '888888';
  }
}
export default function verifyAtLeastTest() {
  describe('verifyAtLeastTest', () => {
    it('test_verify_atLeast', 0, () => {
      // 1.创建MockKit对象
      let mocker: MockKit = new MockKit();
      // 2.创建类对象
      let claser: ClassName = new ClassName();
      // 3.Mock  类ClassName对象的某个方法，比如method_1
      let func_1: Function = mocker.mockFunc(claser, claser.method_1);
      // 4.期望被Mock后的函数返回结果'4'
      when(func_1)('123').afterReturn('4');
      // 5.随机执行几次函数，参数如下
      claser.method_1('123', 'ppp');
      claser.method_1('abc');
      claser.method_1('xyz');
      claser.method_1();
      claser.method_1('abc', 'xxx', 'yyy');
      claser.method_1();
      // 6.验证函数method_1且参数为空时，是否至少执行过2次
      mocker.verify('method_1', []).atLeast(2);
    })
  })
}
```

**示例13：Mock静态函数**
> @since1.0.16 支持

```javascript
import { describe, it, expect, MockKit, when, ArgumentMatchers } from '@ohos/hypium';

class ClassName {
  constructor() {
  }

  static method_1() {
    return 'ClassName_method_1_call';
  }
}

export default function staticTest() {
  describe('staticTest', () => {
    it('staticTest_001', 0, () => {
      let really_result = ClassName.method_1();
      expect(really_result).assertEqual('ClassName_method_1_call');
      // 1.创建MockKit对象
      let mocker: MockKit = new MockKit();
      // 2.Mock  类ClassName对象的某个方法，比如method_1
      let func_1: Function = mocker.mockFunc(ClassName, ClassName.method_1);
      // 3.期望被Mock后的函数返回结果'mock_data'
      when(func_1)(ArgumentMatchers.any).afterReturn('mock_data');
      let mock_result = ClassName.method_1();
      expect(mock_result).assertEqual('mock_data');
      // 清除Mock能力
      mocker.clear(ClassName);
      let really_result1 = ClassName.method_1();
      expect(really_result1).assertEqual('ClassName_method_1_call');
    })
  })
}
```

**示例14：Mock私有函数**

> @since1.0.25 支持

```javascript
import { describe, it, expect, MockKit, when, ArgumentMatchers } from '@ohos/hypium';

class ClassName {
  constructor() {
  }
  method(arg: number):number {
    return this.method_1(arg);
  }
  private method_1(arg: number) {
    return arg;
  }
}

export default function staticTest() {
  describe('privateTest', () => {
    it('private_001', 0, () => {
      let claser: ClassName = new ClassName(); 
      let really_result = claser.method(123);
      expect(really_result).assertEqual(123);
      // 1.创建MockKit对象
      let mocker: MockKit = new MockKit();
      // 2.Mock  类ClassName对象的私有方法，比如method_1
      let func_1: Function = mocker.mockPrivateFunc(claser, "method_1");
      // 3.期望被Mock后的函数返回结果456
      when(func_1)(ArgumentMatchers.any).afterReturn(456);
      let mock_result = claser.method(123);
      expect(mock_result).assertEqual(456);
      // 清除Mock能力
      mocker.clear(claser);
      let really_result1 = claser.method(123);
      expect(really_result1).assertEqual(123);
    })
  })
}
```

**示例14：Mock成员变量**

> @since1.0.25 支持

```javascript
import { describe, it, expect, MockKit, when, ArgumentMatchers } from '@ohos/hypium';

class ClassName {
  constructor() {
  }
  data = 1;
  private priData = 2;
  method() {
    return this.priData;
  }
}

export default function staticTest() {
  describe('propertyTest', () => {
    it('property_001', 0, () => {
      let claser: ClassName = new ClassName(); 
      let data = claser.data;
      expect(data).assertEqual(1);
      let priData = claser.method();
      expect(priData).assertEqual(2);
      // 1.创建MockKit对象
      let mocker: MockKit = new MockKit();
      // 2.Mock  类ClassName对象的成员变量data
      mocker.mockProperty(claser, "data", 3);
      mocker.mockProperty(claser, "priData", 4);
      // 3.期望被Mock后的成员和私有成员的值分别为3，4
      let mock_result = claser.data;
      let mock_private_result = claser.method();
      expect(mock_result).assertEqual(3);
      expect(mock_private_result).assertEqual(4);
      // 清除Mock能力
      mocker.ignoreMock(claser, "data");
      mocker.ignoreMock(claser, "priData");
      let really_result = claser.data;
      expect(really_result).assertEqual(1);
      let really_private_result = claser.method();
      expect(really_private_result).assertEqual(2);
    })
  })
}
```

#### 数据驱动

##### 约束限制

单元测试框架数据驱动能力从[框架 1.0.2版本](https://ohpm.openharmony.cn/#/cn/detail/@ohos%2Fhypium)开始支持。

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

DevEco Studio从V3.0 Release（2022-09-06）版本开始支持

stage模型：

在TestAbility目录下TestAbility.ets文件中导入data.json，并在Hypium.hypiumTest() 方法执行前，设置参数数据

FA模型：

在TestAbility目录下app.js或app.ets文件中导入data.json，并在Hypium.hypiumTest() 方法执行前，设置参数数据

```javascript
import AbilityDelegatorRegistry from '@ohos.application.abilityDelegatorRegistry'
import { Hypium } from '@ohos/hypium'
import testsuite from '../test/List.test'
import data from '../test/data.json';

...
Hypium.setData(data);
Hypium.hypiumTest(abilityDelegator, abilityDelegatorArguments, testsuite);
...
```

```javascript
 import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';

 export default function abilityTest() {
  describe('actsAbilityTest', () => {
    it('testDataDriverAsync', 0, async (done: Function, data: ParmObj) => {
      console.info('name: ' + data.name);
      console.info('value: ' + data.value);
      done();
    });

    it('testDataDriver', 0, () => {
      console.info('stress test');
    })
  })
}
 interface ParmObj {
   name: string,
   value: string
 }
```
>说明 : 若要使用数据驱动传入参数功能，测试用例it的func必须传入两个参数：done定义在前面，data定义在后面；若不使用数据驱动传入参数功能，func可以不传参或传入done

正确示例：
```javascript
    it('testcase01', 0, async (done: Function, data: ParmObj) => {
      ...
      done();
    });
    it('testcase02', 0, async (done: Function) => {
      ...
      done();
    });
    it('testcase03', 0, () => {
      ...
    });
```
错误示例：
```javascript
    it('testcase01', 0, async (data: ParmObj, done: Function) => {
      ...
      done();
    });
    it('testcase02', 0, async (data: ParmObj) => {
      ...
    });
```

#### 专项能力

该项能力需通过在cmd窗口中输入aa test命令执行触发，并通过设置执行参数触发不同功能。另外，测试应用模型与编译方式不同，对应的aa test命令也不同，具体可参考[自动化测试框架使用指导](https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/application-test/arkxtest-guidelines.md#cmd%E6%89%A7%E8%A1%8C)

- **筛选能力**

  1、按测试用例属性筛选

  可以利用框架提供的Level、Size、TestType 对象，对测试用例进行标记，以区分测试用例的级别、粒度、测试类型，各字段含义及代码如下：

  | Key      | 含义说明     | Value取值范围                                                                                                                                          |
  | -------- | ------------ |----------------------------------------------------------------------------------------------------------------------------------------------------|
  | level    | 用例级别     | "0","1","2","3","4", 例如：-s level 1                                                                                                                 |
  | size     | 用例粒度     | "small","medium","large", 例如：-s size small                                                                                                         |
  | testType | 用例测试类型 | "function","performance","power","reliability","security","global","compatibility","user","standard","safety","resilience", 例如：-s testType function |

  示例代码：

 ```javascript
 import { describe, it, expect, TestType, Size, Level } from '@ohos/hypium';

 export default function attributeTest() {
  describe('attributeTest', () => {
    it("testAttributeIt", TestType.FUNCTION | Size.SMALLTEST | Level.LEVEL0, () => {
      console.info('Hello Test');
    })
  })
}
 ```

  示例命令: 

  ```shell
  hdc shell aa test -b xxx -m xxx -s unittest OpenHarmonyTestRunner -s testType function -s size small -s level 0
  ```

  该命令作用是筛选测试应用中同时满足，用例测试类型是“function”、用例粒度是“small”、用例级别是“0”的三个条件用例执行。

  2、按测试套/测试用例名称筛选

  注意：测试套和测试用例的命名要符合框架规则，即以字母开头，后跟一个或多个字母、数字，不能包含特殊符号。

  框架可以通过指定测试套与测试用例名称，来指定特定用例的执行，测试套与用例名称用“#”号连接，多个用“,”英文逗号分隔

  | Key      | 含义说明                | Value取值范围                                                |
  | -------- | ----------------------- | ------------------------------------------------------------ |
  | class    | 指定要执行的测试套&用例 | ${describeName}#${itName}，${describeName} , 例如：-s class attributeTest#testAttributeIt |
  | notClass | 指定不执行的测试套&用例 | ${describeName}#${itName}，${describeName} , 例如：-s notClass attributeTest#testAttribut |

  示例代码：

  ```javascript
  import { describe, it, expect, TestType, Size, Level } from '@ohos/hypium';

  export default function attributeTest() {
  describe('describeTest_000',  () => {
    it("testIt_00", TestType.FUNCTION | Size.SMALLTEST | Level.LEVEL0,  () => {
      console.info('Hello Test');
    })

    it("testIt_01", TestType.FUNCTION | Size.SMALLTEST | Level.LEVEL0, () => {
      console.info('Hello Test');
    })
  })

  describe('describeTest_001',  () => {
    it("testIt_02", TestType.FUNCTION | Size.SMALLTEST | Level.LEVEL0, () => {
      console.info('Hello Test');
    })
  })
}
  ```

  示例命令1: 

  ```shell
  hdc shell aa test -b xxx -m xxx -s unittest OpenHarmonyTestRunner -s class describeTest_000#testIt_00,describeTest_001
  ```

  该命令作用是执行“describeTest_001”测试套中所有用例，以及“describeTest_000”测试套中的“testIt_00”用例。

  示例命令2：

  ```shell
  hdc shell aa test -b xxx -m xxx -s unittest OpenHarmonyTestRunner -s notClass describeTest_000#testIt_01
  ```

  该命令作用是不执行“describeTest_000”测试套中的“testIt_01”用例。

- **随机执行**

  使测试套与测试用例随机执行，用于稳定性测试。

  | Key    | 含义说明                             | Value取值范围                                  |
  | ------ | ------------------------------------ | ---------------------------------------------- |
  | random | @since1.0.3 测试套、测试用例随机执行 | true, 不传参默认为false， 例如：-s random true |

  示例命令：

  ```shell
  hdc shell aa test -b xxx -m xxx -s unittest OpenHarmonyTestRunner -s random true
  ```

- **压力测试**

  指定要执行用例的执行次数，用于压力测试。

  | Key    | 含义说明                             | Value取值范围                  |
  | ------ | ------------------------------------ | ------------------------------ |
  | stress | @since1.0.5 指定要执行用例的执行次数 | 正整数， 例如： -s stress 1000 |

  示例命令：

  ```shell
  hdc shell aa test -b xxx -m xxx -s unittest OpenHarmonyTestRunner -s stress 1000
  ```

- **用例超时时间设置**

  指定测试用例执行的超时时间，用例实际耗时如果大于超时时间，用例会抛出"timeout"异常，用例结果会显示“excute timeout XXX”

  | Key     | 含义说明                   | Value取值范围                                        |
  | ------- | -------------------------- | ---------------------------------------------------- |
  | timeout | 指定测试用例执行的超时时间 | 正整数(单位ms)，默认为 5000，例如： -s timeout 15000 |

  示例命令：

  ```shell
  hdc shell aa test -b xxx -m xxx -s unittest OpenHarmonyTestRunner -s timeout 15000
  ```

- **遇错即停模式**

  | Key          | 含义说明                                                     | Value取值范围                                        |
  | ------------ | ------------------------------------------------------------ | ---------------------------------------------------- |
  | breakOnError | @since1.0.6 遇错即停模式，当执行用例断言失败或者发生错误时，退出测试执行流程 | true, 不传参默认为false， 例如：-s breakOnError true |
  
  示例命令：

  ```shell
  hdc shell aa test -b xxx -m xxx -s unittest OpenHarmonyTestRunner -s breakOnError true
  ```
  
- **测试套中用例信息输出**

  输出测试应用中待执行的测试用例信息

  | Key    | 含义说明                     | Value取值范围                                  |
  | ------ | ---------------------------- | ---------------------------------------------- |
  | dryRun | 显示待执行的测试用例信息全集 | true, 不传参默认为false， 例如：-s dryRun true |

  示例命令：

  ```shell
  hdc shell aa test -b xxx -m xxx -s unittest OpenHarmonyTestRunner -s dryRun true
  ```

- **嵌套能力**

  1.示例代码
  ```javascript
  // Test1.test.ets
  import { describe, expect, it } from '@ohos/hypium';
  import test2 from './Test2.test';

  export default function test1() {
    describe('test1', () => {
      it('assertContain1', 0, () => {
        let a = true;
        let b = true;
        expect(a).assertEqual(b);
      })
      // 引入测试套test2
      test2();
    })
  }
  ```

  ```javascript
  //Test2.test.ets
  import { describe, expect, it } from '@ohos/hypium';

  export default function test2() {
    describe('test2', () => {
      it('assertContain1', 0, () => {
        let a = true;
        let b = true;
        expect(a).assertEqual(b);
      })
      it('assertContain2', 0, () => {
        let a = true;
        let b = true;
        expect(a).assertEqual(b);
      })
    })
  }
  ```

  ```javascript
  //List.test.ets
  import test1 from './nest/Test1.test';

  export default function testsuite() {
    test1();
  }
  ```

  2.示例筛选参数
    ```shell
    #执行test1的全部测试用例
    -s class test1 
    ```
    ```shell
    #执行test1的子测试用例
    -s class test1#assertContain1
    ```
    ```shell
    #执行test1的子测试套test2的测试用例
    -s class test1.test2#assertContain1
    ```

- **跳过能力**

    | Key          | 含义说明                                                     | Value取值范围                                        |
  | ------------ | ------------------------------------------------------------ | ---------------------------------------------------- |
  | skipMessage | @since1.0.17 显示待执行的测试用例信息全集中是否包含跳过测试套和跳过用例的信息 | true/false, 不传参默认为false， 例如：-s skipMessage true |
  | runSkipped | @since1.0.17 指定要执行的跳过测试套&跳过用例 | all，skipped，${describeName}#${itName}，${describeName}，不传参默认为空，例如：-s runSkipped all |

  1.示例代码
  
  ```javascript
  //Skip1.test.ets
  import { expect, xdescribe, xit } from '@ohos/hypium';
  
  export default function skip1() {
    xdescribe('skip1', () => {
      //注意：在xdescribe中不支持编写it用例
      xit('assertContain1', 0, () => {
        let a = true;
        let b = true;
        expect(a).assertEqual(b);
      })
    })
  }
  ```
  
    ```javascript
  //Skip2.test.ets
  import { describe, expect, xit, it } from '@ohos/hypium';
  
  export default function skip2() {
    describe('skip2', () => {
      //默认会跳过assertContain1
      xit('assertContain1', 0, () => {
        let a = true;
        let b = true;
        expect(a).assertEqual(b);
      })
      it('assertContain2', 0, () => {
        let a = true;
        let b = true;
        expect(a).assertEqual(b);
      })
    })
  }
    ```



### 使用方式

单元测试框架以ohpm包形式发布至[服务组件官网](https://ohpm.openharmony.cn/#/cn/detail/@ohos%2Fhypium)，开发者可以下载Deveco Studio后，在应用工程中配置依赖后使用框架能力，测试工程创建及测试脚本执行使用指南请参见[IDE指导文档](https://developer.harmonyos.com/cn/docs/documentation/doc-guides/ohos-openharmony-test-framework-0000001263160453)。
>**说明**
>
>1.0.8版本开始单元测试框架以HAR(Harmony Archive)格式发布
>
> 


