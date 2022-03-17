

# Hypium使用介绍

## 简介
​    Hypium测试框架支持App的单元测试和UI测试，应用开发者可以使用Hypium测试应用内接口逻辑以及相应的界面UI。

​    JsUnit单元测试框架(JS/TS)提供单元测试用例执行能力，提供用例编写基础接口，生成对应报告，用于测试系统或app接口。
​    UiTest OpenHarmony应用UI测试框架，提供稳定的UI触控/检视能力和简洁易用的用例编写API，用于FA界面/控件的自动化测试。

## 目录

```
hypium 
  |-----jsunit  js/ts单元测试框架
  |-----uitest  ui界面测试框架
```
## 约束限制
> **说明:**
>
> 本模块首批接口从API version 8开始支持。后续版本的新增接口，采用上角标单独标记接口的起始版本。

## 单元测试功能特性

| No.  | 特性     | 功能说明                           |
| ---- | -------- | ---------------------------------- |
| 1    | 基础流程 | 支持基础用例编写及执行             |
| 2    | 断言库   | 判断用例实际期望值与预期值是否相符 |

### 使用说明

####  基础流程

测试用例采用业内通用语法，describe 代表一个测试套， it 代表一条用例。

| No.  | API        | 功能说明                                                     |
| ---- | ---------- | ------------------------------------------------------------ |
| 1    | describe   | 定义一个测试套，支持两个参数： 测试套名称和测试套函数        |
| 2    | beforeAll  | 在测试套内定义一个预置条件，在所有测试用例开始前执行且仅执行一次，支持一个参数：预置动作函数 |
| 3    | beforeEach | 在测试套内定义一个单元预置条件，在每条测试用例开始前执行，执行次数与 it 定义的测试用例数一致，支持一个参数：预置动作函数 |
| 4    | afterEach  | 在测试套内定义一个单元清理条件，在每条测试用例结束后执行，执行次数与 it 定义的测试用例数一致，支持一个参数：清理动作函数 |
| 5    | afterAll   | 在测试套内定义一个清理条件，在所有测试用例结束后执行且仅执行一次，支持一个参数：清理动作函数 |
| 6    | it         | 定义一条测试用例，支持三个参数：用例名称，过滤参数和用例函数 |
| 7    | expect     | 支持 bool 类型判断等多种断言方法                             |

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
          console.info(err.code);
        })
    })
  })
}
```



####  断言库

断言功能列表


| No.  | API              | 功能说明                                                     |
| :--- | :--------------- | ------------------------------------------------------------ |
| 1    | assertClose      | 检验 actualvalue 和 expectvalue(0) 的接近程度是否为 expectValue(1) |
| 2    | assertContain    | 检验 actualvalue中是否包含 expectvalue                       |
| 3    | assertEqual      | 检验 actualvalue是否和 expectvalue[0]相等                    |
| 4    | assertFail       | 抛出一个错误                                                 |
| 5    | assertFalse      | 检验 actualvalue 是否为 false                                |
| 6    | assertTrue       | 检验 actualvalue 是否为 true                                 |
| 7    | assertInstanceOf | 检验 actualvalue 是否是 expectvalue类型                      |
| 8    | assertLarger     | 检验 actualvalue 是否大于 expectvalue                        |
| 9    | assertLess       | 检验 actualvalue 是否小于 expectvalue                        |
| 10   | assertNull       | 检验 actualvalue 是否为 null                                 |
| 11   | assertThrowError | 检验 actualvalue 抛出 Error 内容是否为 expectValue           |
| 12   | assertUndefined  | 检验 actualvalue 是否为 undefined                            |

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

  JsUnit测试框架以npm包形式发布至官网，集成至sdk，开发者可以下载Deveco Studio使用，使用指南参见IDE 文档。

## UITest功能特性

| No.  | 特性        | 功能说明                                                     |
| ---- | ----------- | ------------------------------------------------------------ |
| 1    | UiDriver    | UI测试的入口，提供控件查找，控件存在性检查以及按键注入能力   |
| 2    | By          | 类用于描述目标控件特征(文本、id、类型等)，`UiDriver`根据`By`描述的控件特征信息来完成控件查找 |
| 3    | UiComponent | UiDriver`查找返回的控件对象，提供控件属性查询，控件，滑动查找等触控/检视能力 |

**使用者在测试脚本通过如下方式引入使用:**

```typescript
import {UiDriver,BY,UiCOmponent,MatchPattern} from '@ohos.uitest'
```

> 注意事项
> 1. `By`类提供的接口全部为同步接口, 使用者可以以`builder`模式链式调用其接口构造控件筛选条件
> 2. `UiDrivier`和`UiComponent`类提供的接口全部为异步接口(`Promise`形式)，**需以`await`方式调用**
> 3. UI测试用例均需以**异步**用例方式编写，需遵循JsUnit测试框架异步用例编写规范

   

在测试用例文件中import `By/UiDriver/UiComponent`类，然后调用API接口编写测试用例。

```javascript
import {BY,UiDriver,UiComponent} from '@ohos.uitest'

export default async function abilityTest() {
  describe('uiTestDemo', function() {
    it('uitest_demo0', 0, async function(done) {
      try{
        // create UiDriver
        let driver = await UiDriver.create()
        // find component by text
        let button = await UiDriver.findComponent(BY.text('hello').enabled(true))
        // click component
        await button.click()
        // get and assert component text
        let content = await button.getText()
        expect(content).assertEquals('clicked!')
      } finally {
        done()
      }
    })
  })
}
```

### UIDrvier使用说明

`UiDriver`类作为uitest测试框架的总入口，提供控件查找，按键注入，坐标点击/滑动/手势，截图等能力

| No.  | API                                                          | 功能描述               |
| ---- | ------------------------------------------------------------ | ---------------------- |
| 1    | create():Promise<UiDriver>                                   | 静态方法，构造UiDriver |
| 2    | findComponent(b:By):Promise<UiComponent>                     | 查找匹配控件           |
| 3    | pressBack():Promise<void>                                    | 点击BACK键             |
| 4    | click(x:number, y:number):Promise<void>                      | 基于坐标点的点击       |
| 5    | swipe(x1:number, y1:number, x2:number, y2:number):Promise<void> | 基于坐标点的滑动       |
| 6    | assertComponentExist(b:By):Promise<void>                     | 断言匹配的控件存在     |
| 7    | delayMs(t:number):Promise<void>                              | 延时                   |
| 8    | screenCap(s:path):Promise<void>                              | 截屏                   |

其中assertComponentExist接口为断言API，用于断言当前界面存在目标控件; 如果控件不存在，该API将抛出JS异常，使当前测试用例失败。

```javascript
import {BY,UiDriver,UiComponent} from '@ohos.uitest'

export default async function abilityTest() {
  describe('uiTestDemo', function() {
    it('uitest_demo0', 0, async function(done) {
      try{
        // create UiDriver
        let driver = await UiDriver.create()
        // assert text 'hello' exists on current UI
        await assertComponentExist(BY.text('hello'))
      } finally {
        done()
      }
    })
  })
}
```

### By使用说明

UiTest框架通过`By`类提供了丰富的控件特征描述API，用来匹配查找要操作/检视的目标控件。`By`提供的API能力具有以下特点:

- 支持单属性匹配和多属性组合匹配，例如同时指定目标控件text和id
- 控件属性支持多种匹配模式(等于，包含，`STARTS_WITH`，`ENDS_WITH`)
- 支持控件相对定位，可通过`isBefore`和`isAfter`等API限定邻近控件特征进行辅助定位

| No.  | API                                | 功能描述                                       |
| ---- | ---------------------------------- | ---------------------------------------------- |
| 1    | id(i:number):By                    | 指定控件id                                     |
| 2    | text(t:string, p?:MatchPattern):By | 指定控件文本,可指定匹配模式                    |
| 3    | type(t:string)):By                 | 指定控件类型                                   |
| 4    | enabled(e:bool):By                 | 指定控件使能状态                               |
| 5    | clickable(c:bool):By               | 指定控件可点击状态                             |
| 6    | focused(f:bool):By                 | 指定控件获焦状态                               |
| 7    | scrollable(s:bool):By              | 指定控件可滑动状态                             |
| 8    | selected(s:bool):By                | 指定控件选中状态                               |
| 9    | isBefore(b:By):By                  | **相对定位**，限定目标控件位于指定特征控件之前 |
| 10   | isAfter(b:By):By                   | **相对定位**，限定目标控件位于指定特征控件之后 |

其中，`text`属性支持{`MatchPattern.EQUALS`，`MatchPattern.CONTAINS`，`MatchPattern.STARTS_WITH`，`MatchPattern.ENDS_WITH`}四种匹配模式, 缺省使用`MatchPattern.EQUALS`模式。

#### 控件绝对定位

**示例代码1**: 查找id为`Id_button`的控件:

```javascript
let button = await driver.findComponent(BY.id(Id_button))
```

 **示例代码2**：查找id为`Id_button`并且状态为`enabled`的控件, 适用于无法通过单一属性定位的场景:

```javascript
let button = await driver.findComponent(BY.id(Id_button).enabled(true))
```

通过`By.id(x).enabled(y)`来对目标控件的多个属性进行指定

**示例代码3**: 查找文本中包含`hello`的控件, 适用于控件属性取值不能完全确定的场景:

```javascript
let txt = await driver.findComponent(BY.text("hello", MatchPattern.CONTAINS))
```

通过向`By.text()`方法传入第二个参数`MatchPattern.CONTAINS`来指定文本匹配规则；默认规则为`MatchPattern.EQUALS`，即目标控件text属性必须严格等于给定值。

####  控件相对定位

**示例代码1**: 查找位于文本控件`Item3_3`后面的，id为`ResourceTable.Id_switch`的Switch控件:

```javascript
let switch = await driver.findComponent(BY.id(Id_switch).isAfter(BY.text("Item3_3")))
```

通过`By.isAfter`方法，指定位于目标控件前面的特征控件属性，通过该特征控件进行相对定位。一般地，特征控件为某个具有全局唯一特征的控件(例如具有唯一的id或者唯一的text)。


类似的，可以使用`By.isBefore`控件指定位于目标控件后面的特征控件属性，实现相对定位。

### UiComponent使用说明

`UiComponent`类代表了UI界面上的一个控件，一般是通过`UiDriver.findComponent(by)`方法查找到的。通过该类的实例，用户可以进行控件属性获取，控件点击，滑动查找，文本注入等操作。

`UiComponent`包含的常用API:

| No.  | API                               | 功能描述                                     |
| ---- | --------------------------------- | -------------------------------------------- |
| 1    | click():Promise<void>             | 点击该控件                                   |
| 2    | inputText(t:string):Promise<void> | 向控件中输入文本(适用于文本框控件)           |
| 3    | scrollSearch(s:By):Promise<bool>  | 在该控件上滑动查找目标控件(适用于List等控件) |
| 4    | getText():Promise<string>         | 获取控件text                                 |
| 5    | getId():Promise<number>           | 获取控件id                                   |
| 6    | getType():Promise<string>         | 获取控件类型                                 |
| 7    | isEnabled():Promise<bool>         | 获取控件使能状态                             |

`UiComponent`完整的API列表请参考其API文档。

**示例代码1**: 控件点击

```javascript
let button = await driver.findComponent(BY.id(Id_button))
await button.click()
```

**示例代码2**: 通过get接口获取控件属性后，可以使用单元测试框架提供的assert*接口对其进行断言检查:

```javascript
let component = await driver.findComponent(BY.id(Id_title))
expect(component != null).assertTrue()
```

**示例代码3**: 在ListContainer控件中滑动查找text为`Item3_3`的子控件:

```javascript
let listContainer = await driver.findComponent(BY.id(Id_list))
let found = await listContainer.scrollSearch(BY.text("Item3_3"))
expect(found).assertTrue()
```

**示例代码4**: 向输入框控件中输入文本

```javascript
let editText = await driver.findComponent(BY.type('Input'))
await editText.inputText("user_name")
```
### 推送UiTest至设备

> UiTest框架暂时不随版本编译，使用时需自行编译后推送至OpenHarmony设备。后续随版本编译后，直接使用版本即可

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

