# OpenHarmony自动化测试框架使用介绍

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

| No.  | 特性     | 功能说明                           |
| ---- | -------- | ---------------------------------- |
| 1    | 基础流程 | 支持编写及执行基础用例             |
| 2    | 断言库   | 判断用例实际期望值与预期值是否相符 |

### 使用说明

####  基础流程

测试用例采用业内通用语法，describe代表一个测试套， it代表一条用例。

| No.  | API        | 功能说明                                                     |
| ---- | ---------- | ------------------------------------------------------------ |
| 1    | describe   | 定义一个测试套，支持两个参数：测试套名称和测试套函数         |
| 2    | beforeAll  | 在测试套内定义一个预置条件，在所有测试用例开始前执行且仅执行一次，支持一个参数：预置动作函数 |
| 3    | beforeEach | 在测试套内定义一个单元预置条件，在每条测试用例开始前执行，执行次数与it定义的测试用例数一致，支持一个参数：预置动作函数 |
| 4    | afterEach  | 在测试套内定义一个单元清理条件，在每条测试用例结束后执行，执行次数与it定义的测试用例数一致，支持一个参数：清理动作函数 |
| 5    | afterAll   | 在测试套内定义一个清理条件，在所有测试用例结束后执行且仅执行一次，支持一个参数：清理动作函数 |
| 6    | it         | 定义一条测试用例，支持三个参数：用例名称，过滤参数和用例函数 |
| 7    | expect     | 支持bool类型判断等多种断言方法                               |

示例代码：

```javascript
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from 'hypium/index'
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


| No.  | API              | 功能说明                                                     |
| :--- | :--------------- | ------------------------------------------------------------ |
| 1    | assertClose      | 检验actualvalue和expectvalue(0)的接近程度是否是expectValue(1) |
| 2    | assertContain    | 检验actualvalue中是否包含expectvalue                         |
| 3    | assertEqual      | 检验actualvalue是否等于expectvalue[0]                        |
| 4    | assertFail       | 抛出一个错误                                                 |
| 5    | assertFalse      | 检验actualvalue是否是false                                   |
| 6    | assertTrue       | 检验actualvalue是否是true                                    |
| 7    | assertInstanceOf | 检验actualvalue是否是expectvalue类型                         |
| 8    | assertLarger     | 检验actualvalue是否大于expectvalue                           |
| 9    | assertLess       | 检验actualvalue是否小于expectvalue                           |
| 10   | assertNull       | 检验actualvalue是否是null                                    |
| 11   | assertThrowError | 检验actualvalue抛出Error内容是否是expectValue                |
| 12   | assertUndefined  | 检验actualvalue是否是undefined                               |

示例代码：

```javascript
import { describe, it, expect } from 'hypium/index'
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
### 使用方式

  单元测试框架以npm包（hypium）形式发布至官网([https://www.npmjs.com/](https://www.npmjs.com/))，集成至sdk，开发者可以下载Deveco Studio使用，测试工程创建及测试脚本执行使用指南请参见IDE指导文档。

## Ui测试框架功能特性

| No.  | 特性        | 功能说明                                                     |
| ---- | ----------- | ------------------------------------------------------------ |
| 1    | UiDriver    | Ui测试的入口，提供查找控件，检查控件存在性以及注入按键能力   |
| 2    | By          | 用于描述目标控件特征(文本、id、类型等)，`UiDriver`根据`By`描述的控件特征信息来查找控件 |
| 3    | UiComponent | UiDriver查找返回的控件对象，提供查询控件属性，滑动查找等触控和检视能力 |

**使用者在测试脚本通过如下方式引入使用：**

```typescript
import {UiDriver,BY,UiComponent,MatchPattern} from '@ohos.uitest'
```

> 注意事项
> 1. `By`类提供的接口全部是同步接口，使用者可以使用`builder`模式链式调用其接口构造控件筛选条件。
> 2. `UiDrivier`和`UiComponent`类提供的接口全部是异步接口(`Promise`形式)，**需使用`await`语法**。
> 3. Ui测试用例均需使用**异步**语法编写用例，需遵循单元测试框架异步用例编写规范。

   

在测试用例文件中import `By/UiDriver/UiComponent`类，然后调用API接口编写测试用例。

```javascript
import {describe, beforeAll, beforeEach, afterEach, afterAll, it, expect} from 'hypium/index'
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

| No.  | API                                                          | 功能描述               |
| ---- | ------------------------------------------------------------ | ---------------------- |
| 1    | create():Promise<UiDriver>                                   | 静态方法，构造UiDriver |
| 2    | findComponent(b:By):Promise<UiComponent>                     | 查找匹配控件           |
| 3    | pressBack():Promise<void>                                    | 单击BACK键             |
| 4    | click(x:number, y:number):Promise<void>                      | 基于坐标点的单击       |
| 5    | swipe(x1:number, y1:number, x2:number, y2:number):Promise<void> | 基于坐标点的滑动       |
| 6    | assertComponentExist(b:By):Promise<void>                     | 断言匹配的控件存在     |
| 7    | delayMs(t:number):Promise<void>                              | 延时                   |
| 8    | screenCap(s:path):Promise<void>                              | 截屏                   |

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

### By使用说明

Ui测试框架通过`By`类提供了丰富的控件特征描述API，用来匹配查找要操作或检视的目标控件。`By`提供的API能力具有以下特点：

- 支持匹配单属性和匹配多属性组合，例如同时指定目标控件text和id。
- 控件属性支持多种匹配模式(等于，包含，`STARTS_WITH`，`ENDS_WITH`)。
- 支持相对定位控件，可通过`isBefore`和`isAfter`等API限定邻近控件特征进行辅助定位。

| No.  | API                                | 功能描述                                       |
| ---- | ---------------------------------- | ---------------------------------------------- |
| 1    | id(i:number):By                    | 指定控件id                                     |
| 2    | text(t:string, p?:MatchPattern):By | 指定控件文本，可指定匹配模式                   |
| 3    | type(t:string)):By                 | 指定控件类型                                   |
| 4    | enabled(e:bool):By                 | 指定控件使能状态                               |
| 5    | clickable(c:bool):By               | 指定控件可单击状态                             |
| 6    | focused(f:bool):By                 | 指定控件获焦状态                               |
| 7    | scrollable(s:bool):By              | 指定控件可滑动状态                             |
| 8    | selected(s:bool):By                | 指定控件选中状态                               |
| 9    | isBefore(b:By):By                  | **相对定位**，限定目标控件位于指定特征控件之前 |
| 10   | isAfter(b:By):By                   | **相对定位**，限定目标控件位于指定特征控件之后 |

其中，`text`属性支持{`MatchPattern.EQUALS`，`MatchPattern.CONTAINS`，`MatchPattern.STARTS_WITH`，`MatchPattern.ENDS_WITH`}四种匹配模式，缺省使用`MatchPattern.EQUALS`模式。

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

| No.  | API                               | 功能描述                                     |
| ---- | --------------------------------- | -------------------------------------------- |
| 1    | click():Promise<void>             | 单击该控件                                   |
| 2    | inputText(t:string):Promise<void> | 向控件中输入文本(适用于文本框控件)           |
| 3    | scrollSearch(s:By):Promise<bool>  | 在该控件上滑动查找目标控件(适用于List等控件) |
| 4    | getText():Promise<string>         | 获取控件text                                 |
| 5    | getId():Promise<number>           | 获取控件id                                   |
| 6    | getType():Promise<string>         | 获取控件类型                                 |
| 7    | isEnabled():Promise<bool>         | 获取控件使能状态                             |

`UiComponent`完整的API列表请参考其API文档。

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
### 使用方式

  开发者可以下载Deveco Studio创建测试工程后，在其中调用框架提供接口进行相关测试操作，测试工程创建及测试脚本执行使用指南请参见IDE指导文档。

### 推送Ui测试框架至设备

> Ui测试框架暂时不随版本编译，使用时需自行编译后推送至OpenHarmony设备。后续随版本编译后，直接使用版本即可。

#### 构建方式

```shell
./build.sh --product-name rk3568 --build-target uitestkit
```
#### 推送方式

```shell
hdc_std target mount
hdc_std shell mount -o rw,remount /
hdc_std file send uitest /system/bin/uitest
hdc_std file send libuitest.z.so /system/lib/module/libuitest.z.so
hdc_std shell chmod +x /system/bin/uitest
```
