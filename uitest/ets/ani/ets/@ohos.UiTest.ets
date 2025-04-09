/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License,Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,software
 * distributed under the License is distributed on an "ASIS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
import abilityDelegatorRegistry from '@ohos.app.ability.abilityDelegatorRegistry';
import hilog from '@ohos.hilog'
import { Callback } from '@ohos.base';
import { BusinessError } from '@ohos.base';
let domain: int = 0x0000;
let tag: string = 'uitestTag';

export default namespace UiTest {
  loadLibrary("uitest_ani");
  export native function ScheduleEstablishConnection(tokken: String): boolean;
  export native function GetConnectionStat(): int;
}

export function loadAndSetupUiTest():void {
  hilog.error(domain, tag, 'loadAndSetupUiTest');
  let delegator = abilityDelegatorRegistry.getAbilityDelegator();
  hilog.error(domain, tag, 'after abilityDelegatorRegistry.getAbilityDelegator();');
  try{
    if (delegator!=null) {
      hilog.error(domain, tag, 'UiTestKit_exporter: Got AbilityDelegator');
      let result = UiTest.ScheduleEstablishConnection("default");
      hilog.error(domain, tag, "Uitest.ScheduleEstablishConnection(): "+result )
      hilog.error(domain, tag, "Uitest.GetConnectionStat(): "+UiTest.GetConnectionStat())
      hilog.error(domain, tag, 'UiTestKit_exporter: Begin executing shell command to start server-daemon');
      delegator.executeShellCommand(`uitest start-daemon default`, 3.0).then((value: NullishType) => {
        hilog.error(domain, tag, `UiTestKit_exporter: Start server-daemon finished!!!!`);
      }).catch((error:Error) :void=> {
        hilog.error(domain, tag, "UiTestKit_exporter: Start server-daemon failed:"+error)
      });
      hilog.error(domain, tag, "Uitest.GetConnectionStat(): "+UiTest.GetConnectionStat())
    } else {
      hilog.error(domain, tag, 'UiTestKit_exporter: Cannot get AbilityDelegator, uitest_daemon need to be pre-started');
      UiTest.ScheduleEstablishConnection('default');
    }
  } catch(error: Error){
    hilog.error(domain, tag, "loadAndSetupUiTest failed" + error)
    hilog.error(domain, tag, "loadAndSetupUiTest failed" + error.stack)
    hilog.error(domain, tag, "Uitest.GetConnectionStat(): "+UiTest.GetConnectionStat())
  }
}
enum MatchPattern {
    EQUALS = 0,
    CONTAINS = 1,
    STARTS_WITH = 2,
    ENDS_WITH = 3,
    REG_EXP = 4,
    REG_EXP_ICASE = 5,
  }
  
enum WindowMode {
    FULLSCREEN = 0,
    PRIMARY = 1,
    SECONDARY = 2,
    FLOATING = 3,
}
  
enum ResizeDirection {
    LEFT = 0,
    RIGHT = 1,
    UP = 2,
    DOWN = 3,
    LEFT_UP = 4,
    LEFT_DOWN = 5,
    RIGHT_UP = 6,
    RIGHT_DOWN = 7,
}
  
enum DisplayRotation {
    ROTATION_0 = 0,
    ROTATION_90 = 1,
    ROTATION_180 = 2,
    ROTATION_270 = 3,
}
  
enum UiDirection {
    LEFT = 0,
    RIGHT = 1,
    UP = 2,
    DOWN = 3,
}
  
enum MouseButton {
    MOUSE_BUTTON_LEFT = 0,
    MOUSE_BUTTON_RIGHT = 1,
    MOUSE_BUTTON_MIDDLE = 2,
}
  
interface Point {
 x: int;
 y: int;
}
  
class PointInner implements Point {
 x: int = 0;
 y: int = 0;
}
  
interface Rect {
  left: int;
  top: int;
  right: int;
  bottom: int;
}
  
class RectInner implements Rect {
  left: int = 0;
  top: int = 0;
  right: int = 0;
  bottom: int = 0;
}
  
interface WindowFilter {
  bundleName?: string;
  title?: string;
  focused?: boolean;
  active?: boolean;
}

class WindowFilterInner implements WindowFilter {
  bundleName?: string = "";
  title?: string = "";
  focused?: boolean = false;
  active?: boolean = false;
}

interface UIElementInfo {
  bundleName: string;
  type: string;
  text: string;
}

class UIElementInfoInner implements UIElementInfo {
  bundleName: string = "";
  type: string = "";
  text: string = "";
}

interface TouchPadSwipeOptions {
  stay?: boolean;
  speed?: int;
}

class TouchPadSwipeOptionsInner implements TouchPadSwipeOptions {
  stay?: boolean = false;
  speed?: int = 2000;
}

export class Component {
  private nativeComponent:string = '';
  public constructor(Component:string) {
    if (this.nativeComponent == '') {
      this.nativeComponent = Component;
    }
  }
    private native comClickSync(): boolean;
    private native comLongClickSync(): boolean;
    private native comDoubleClickSync(): boolean;
    private native comDragToSync(target: Component): boolean;
    private native getTextSync(): string;
    private native getTypeSync(): string;
    private native getIdSync(): string;
    private native getHintSync(): string;
    private native getDescriptionSync(): string;
    private native comInputTextSync(text: string): boolean;
    private native clearTextSync(): boolean;
    private native scrollToTopSync(speed?: int): boolean;
    private native scrollToBottomSync(speed?: int): boolean;
    private native scrollSearchSync(on: On, vertical?: boolean, offset?: int): boolean;
    private native pinchInSync(scale: double): boolean;
    private native pinchOutSync(scale: double): boolean;
    private native isScrollableSync(): boolean;
    private native isSelectedSync(): boolean;
    private native isLongClickableSync(): boolean;
    private native isClickableSync(): boolean;
    private native isFocusedSync(): boolean;
    private native isEnabledSync(): boolean;
    private native isCheckedSync(): boolean;
    private native isCheckableSync(): boolean;
    private native comGetBoundsSync(): Rect;
    private native getBoundsCenterSync(): Point;
  
    click():Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.comClickSync());
            promise1.then((e:NullishType) => {
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    longClick():Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.comLongClickSync());
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    doubleClick():Promise<void> {
      let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.comDoubleClickSync());
          promise1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
          });
      });
      return promise;
    }
    
    scrollSearch(on: On, vertical?: boolean, offset?: int):Promise<Component> {
      let promise = new Promise<Component>((resolve: (value: Component) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.scrollSearchSync(on, vertical, offset));
          promise1.then((e:NullishType)=>{
              let value : Component = e as Component;
              resolve(value);
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
          });
      });
      return promise;
    }
  
    pinchIn(scale: double):Promise<void> {
      let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.pinchInSync(scale));
          promise1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
          });
      });
      return promise;
    }  
    pinchOut(scale: double):Promise<void> {
      let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.pinchOutSync(scale));
          promise1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
          });
      });
      return promise;
    }
    
    scrollToTop(speed?: int):Promise<void> {
      let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.scrollToTopSync(speed));
          promise1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
          });
      });
      return promise;
    }
    
    scrollToBottom(speed?: int):Promise<void> {
      let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.scrollToBottomSync(speed));
          promise1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
          });
      });
      return promise;
    }
    
    inputText(text: string):Promise<void> {
      let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.comInputTextSync(text));
          promise1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
          });
      });
      return promise;
    }
    
    dragTo(target: Component):Promise<void> {
      let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.comDragToSync(target));
          promise1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
          });
      });
      return promise;
    }
  
    clearText():Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.clearTextSync());
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    getHint():Promise<string> {
        let promise = new Promise<string>((resolve: (value: string) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():string => this.getHintSync());
            promise1.then((e:NullishType)=>{
                let value : string = e as string;
                resolve(value);
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    getDescription():Promise<string> {
        let promise = new Promise<string>((resolve: (value: string) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():string => this.getDescriptionSync());
            promise1.then((e:NullishType)=>{
                let value : string = e as string;
                resolve(value);
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    getType():Promise<string> {
        let promise = new Promise<string>((resolve: (value: string) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():string => this.getTypeSync());
            promise1.then((e:NullishType)=>{
                let value : string = e as string;
                resolve(value);
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    getText():Promise<string> {
        let promise = new Promise<string>((resolve: (value: string) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():string => this.getTextSync());
            promise1.then((e:NullishType)=>{
                let value : string = e as string;
                resolve(value);
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    getId():Promise<string> {
        let promise = new Promise<string>((resolve: (value: string) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():string => this.getIdSync());
            promise1.then((e:NullishType)=>{
                let value : string = e as string;
                resolve(value);
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }

    isChecked():Promise<boolean> {
      let promise = new Promise<boolean>((resolve: (value: boolean) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.isCheckedSync());
          promise1.then((e:NullishType)=>{
              let value : boolean = e as boolean;
              resolve(value);
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
          });
      });
      return promise;
    }
    isCheckable():Promise<boolean> {
      let promise = new Promise<boolean>((resolve: (value: boolean) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.isCheckableSync());
          promise1.then((e:NullishType)=>{
              let value : boolean = e as boolean;
              resolve(value);
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
          });
      });
      return promise;
    }
    isScrollable():Promise<boolean> {
      let promise = new Promise<boolean>((resolve: (value: boolean) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.isScrollableSync());
          promise1.then((e:NullishType)=>{
              let value : boolean = e as boolean;
              resolve(value);
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
          });
      });
      return promise;
    }
    isFocused():Promise<boolean> {
      let promise = new Promise<boolean>((resolve: (value: boolean) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.isFocusedSync());
          promise1.then((e:NullishType)=>{
              let value : boolean = e as boolean;
              resolve(value);
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
          });
      });
      return promise;
    }
    isEnabled():Promise<boolean> {
      let promise = new Promise<boolean>((resolve: (value: boolean) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.isEnabledSync());
          promise1.then((e:NullishType)=>{
              let value : boolean = e as boolean;
              resolve(value);
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
          });
      });
      return promise;
    }
    isSelected():Promise<boolean> {
      let promise = new Promise<boolean>((resolve: (value: boolean) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.isSelectedSync());
          promise1.then((e:NullishType)=>{
              let value : boolean = e as boolean;
              resolve(value);
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
          });
      });
      return promise;
    }
    isLongClickable():Promise<boolean> {
      let promise = new Promise<boolean>((resolve: (value: boolean) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.isLongClickableSync());
          promise1.then((e:NullishType)=>{
              let value : boolean = e as boolean;
              resolve(value);
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
          });
      });
      return promise;
    }
    isClickable():Promise<boolean> {
      let promise = new Promise<boolean>((resolve: (value: boolean) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.isClickableSync());
          promise1.then((e:NullishType)=>{
              let value : boolean = e as boolean;
              resolve(value);
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
          });
      });
      return promise;
    }
    getBounds():Promise<Rect> {
      let promise = new Promise<Rect>((resolve: (value: Rect) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():Rect => this.comGetBoundsSync());
          promise1.then((e:NullishType)=>{
              let value : Rect = e as Rect;
              resolve(value);
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
          });
      });
      return promise;
    }
    getBoundsCenter():Promise<Point> {
      let promise = new Promise<Point>((resolve: (value: Point) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():Point => this.getBoundsCenterSync());
          promise1.then((e:NullishType)=>{
              let value : Point = e as Point;
              resolve(value);
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
          });
      });
      return promise;
    }
  }
  
  export class UiWindow {
    nativeWindow:string = '';
    public constructor(Window:string) {
      this.nativeWindow = Window;
    }
    private native splitSync(): boolean;
    private native resumeSync(): boolean;
    private native closeSync(): boolean;
    private native minimizeSync(): boolean;
    private native maximizeSync(): boolean;
    private native focusSync(): boolean;
    private native isFocusedSync(): boolean;
    private native isActiveSync(): boolean;
    private native resizeSync(w:int, h:int, d:ResizeDirection): boolean;
    private native moveToSync(x:int, y:int): boolean;
    private native getWindowModeSync(): WindowMode;
    private native GetBundleNameSync(): string;
    private native getTitileSync(): string;
    private native winGetBoundsSync(): Rect;
  
    split():Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.splitSync());
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
            });
        });
        return promise;
    }
  
    resume():Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.resumeSync());
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
            });
        });
        return promise;
    }
  
    close():Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.closeSync());
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
            });
        });
        return promise;
    }
  
    minimize():Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.minimizeSync());
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    maximize():Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.maximizeSync());
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    focus():Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.focusSync());
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    isFocused():Promise<boolean> {
        let promise = new Promise<boolean>((resolve: (value: boolean) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.isFocusedSync());
            promise1.then((e:NullishType)=>{
                let value : boolean = e as boolean;
                resolve(value);
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    isActive():Promise<boolean> {
        let promise = new Promise<boolean>((resolve: (value: boolean) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.isActiveSync());
            promise1.then((e:NullishType)=>{
                let value : boolean = e as boolean;
                resolve(value);
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    resize(w:int, h:int, d:ResizeDirection):Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.resizeSync(w, h, d));
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    moveTo(x:int, y:int):Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.moveToSync(x, y));
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    getWindowMode():Promise<WindowMode> {
        let promise = new Promise<WindowMode>((resolve: (value: WindowMode) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():WindowMode => this.getWindowModeSync());
            promise1.then((e:NullishType)=>{
                let value : WindowMode = e as WindowMode;
                resolve(value);
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    GetBundleName():Promise<string> {
        let promise = new Promise<string>((resolve: (value: string) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():string => this.GetBundleNameSync());
            promise1.then((e:NullishType)=>{
                let value : string = e as string;
                resolve(value);
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    getTitile():Promise<string> {
        let promise = new Promise<string>((resolve: (value: string) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():string => this.getTitileSync());
            promise1.then((e:NullishType)=>{
                let value : string = e as string;
                resolve(value);
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    getBounds():Promise<Rect> {
        let promise = new Promise<Rect>((resolve: (value: Rect) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():Rect => this.winGetBoundsSync());
            promise1.then((e:NullishType)=>{
                let value : Rect = e as Rect;
                resolve(value);
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  }
  
  export class Driver {
    private nativeDriver:string = '';
    public constructor(driver:string) {
        
        hilog.error(domain, tag, 'driver:'+driver);
        this.nativeDriver = driver;
    }
    private native delayMsSync(t:int):boolean;
    private native findComponentSync(on: On):Component;
    native static create():Driver;
    private native clickSync(x:int, y:int):boolean;
    private native longClickSync(x:int, y:int):boolean;
    private native doubleClickSync(x:int, y:int):boolean;
    private native flingSync(from:Point, to: Point, stepLen: int, speed: int):boolean;
    fling(from:Point, to: Point, stepLen: int, speed: int): Promise<void> {
      let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.flingSync(from, to, stepLen, speed));
          promise1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
          });
      });
      return promise;
    }
    private native flingSyncDirection(direction:UiDirection, speed?: int):boolean;
    fling(direction:UiDirection, speed?: int): Promise<void> {
      let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.flingSyncDirection(direction, speed));
          promise1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
          });
      });
      return promise;
    }
    private native swipeSync(startx: int, starty: int, endx: int, endy: int, speed?: int):boolean;
    swipe(startx: int, starty: int, endx: int, endy: int, speed?: int): Promise<void> {
      let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.swipeSync(startx, starty, endx, endy, speed));
          promise1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
          });
      });
      return promise;
    }
    private native dragSync(startx: int, starty: int, endx: int, endy: int, speed?: int):boolean;
    drag(startx: int, starty: int, endx: int, endy: int, speed?: int): Promise<void> {
      let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.dragSync(startx, starty, endx, endy, speed));
          promise1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
          });
      });
      return promise;
    }
    private native pressBackSync():boolean;
    pressBack(): Promise<void> {
      let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.pressBackSync());
          promise1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
            });
      });
      return promise;
    }
    private native assertComponentExistSync(on: On):boolean;
    assertComponentExist(on: On): Promise<void> {
      let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
          let promise1 = taskpool.execute(():boolean => this.assertComponentExistSync(on));
              promise1.then((e:NullishType)=>{
                  resolve(Promise.resolve());
              }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
              });
          });
          return promise;
    }
    private native triggerKeySync(keyCode: int):boolean;
    triggerKey(keyCode: int): Promise<void> {
      let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.triggerKeySync(keyCode));
          promise1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return promise;
    }
    private native inputTextSync(p: Point, text: string):boolean;
    inputText(p: Point, text: string): Promise<void> {
      let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.inputTextSync(p, text));
          promise1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
          });
      });
      return promise;
    }
    private native findWindowSync(filter: WindowFilter):UiWindow;
    findWindow(filter: WindowFilter): Promise<UiWindow> {
      let promise = new Promise<UiWindow>((resolve: (value: UiWindow) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():UiWindow => this.findWindowSync(filter));
      promise1.then((e:NullishType)=>{
          let value : UiWindow = e as UiWindow;
          resolve(value);
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
          });
      });
      return promise;
    }
    native createUIEventObserver():UIEventObserver;

    private native wakeUpDisplaySync():boolean;
     wakeUpDisplay(): Promise<void> {
      let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.wakeUpDisplaySync());
          promise1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
          });
      });
      return promise;
    }
    private native pressHomeSync():boolean;
     pressHome(): Promise<void> {
      let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.pressHomeSync());
          promise1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return promise;
    }
    private native getDisplaySizeSync():Point;
     getDisplaySize(): Promise<Point> {
      let promise = new Promise<Point>((resolve: (value: Point) => void, reject: (error: Object) => void) => {
          let promise1 = taskpool.execute(():Point => this.getDisplaySizeSync());
          promise1.then((e:NullishType)=>{
              let value : Point = e as Point;
              resolve(value);
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
          });
      });
      return promise;
    }
    private native getDisplaySizeDensitySync():Point;
    getDisplaySizeDensity(): Promise<Point> {
      let promise = new Promise<Point>((resolve: (value: Point) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():Point => this.getDisplaySizeDensitySync());
        promise1.then((e:NullishType)=>{
            let value : Point = e as Point;
            resolve(value);
        }, (err: Object): void => {
            let br = err as BusinessError<void>;
            reject(br);
        });
      });
      return promise;
    }
    private native getDisplayRotationSync():DisplayRotation;
     getDisplayRotation(): Promise<DisplayRotation> {
      let promise = new Promise<DisplayRotation>((resolve: (value: DisplayRotation) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():DisplayRotation => this.getDisplayRotationSync());
      promise1.then((e:NullishType)=>{
          let value : DisplayRotation = e as DisplayRotation;
          resolve(value);
        }, (err: Object): void => {
            let br = err as BusinessError<void>;
            reject(br);
        });
      });
      return promise;
    }
    private native findComponentsSync(on: On):Array<Component>;
    findComponents(on: On): Promise<Array<Component>> {
      let promise = new Promise<Array<Component>>((resolve: (value: Array<Component>) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():Array<Component> => this.findComponentsSync(on));
          promise1.then((e:NullishType)=>{
              let value : Array<Component> = e as Array<Component>;
              resolve(value);
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
          });
      });
      return promise;
    }
    private native waitForIdleSync(idleTime: int, timeout: int):boolean;
    private native waitForComponentSync(on: On, time: int):Component;
    private native triggerCombineKeysSync(key0: int, key1: int, key2?: int):boolean;
  
    private native setDisplayRotationEnabledSync(enable: boolean):boolean;
    private native setDisplayRotationSync(rotation: DisplayRotation):boolean;
    private native screenCaptureSync(savepath: string, rect?: Rect):boolean;
    private native screenCapSync(savepath: string):boolean;
  
    private native penSwipeSync(f: Point, t: Point, speed?: int, pressure: double): boolean;
    private native penClickSync(p: Point):boolean;
    private native penDoubleClickSync(p: Point):boolean;
    private native penLongClickSync(p: Point, pressure?: double):boolean;
  
    private native mouseScrollSync(p: Point, down: boolean, d:int, key1?: int, key2?: int, speed?: int): boolean;
    private native mouseMoveWithTrackSync(f: Point, t: Point, speed?: int): boolean;
    private native mouseMoveToSync(p: Point): boolean;
    private native mouseDragkSync(f: Point, t: Point, speed?: int): boolean;
    private native mouseClickSync(p: Point, btnId: MouseButton, key1?: int, key2?: int):boolean;
    private native mouseDoubleClickSync(p: Point, btnId: MouseButton, key1?: int, key2?: int):boolean;
    private native mouseLongClickSync(p: Point, btnId: MouseButton, key1?: int, key2?: int):boolean;
  
    private native injectMultiPointerActionSync(pointers: PointerMatrix, speed?: int): boolean;
    private native injectPenPointerActionSync(pointers: PointerMatrix, speed?: int, pressure?: double): boolean;
    private native touchPadMultiFingerSwipeSync(fingers: int, direction: int, options?: TouchPadSwipeOptions): boolean;
  
    delayMs(t: int): Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.delayMsSync(t));
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    click(x: int, y: int): Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.clickSync(x, y));
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    doubleClick(x: int, y: int): Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.doubleClickSync(x, y));
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    longClick(x: int, y: int): Promise<void> {
      let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let promise1 = taskpool.execute(():boolean => this.longClickSync(x, y));
          promise1.then((e:NullishType)=>{
              resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
      });
      return promise;
    }
  
    findComponent(on: On): Promise<Component> {
        let promise = new Promise<Component>((resolve: (value: Component) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():Component => this.findComponentSync(on));
            promise1.then((e:NullishType)=>{
                let value : Component = e as Component;
                resolve(value);
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    waitForIdle(idleTime: int, timeout: int): Promise<boolean> {
        let promise = new Promise<boolean>((resolve: (value: boolean) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.waitForIdleSync(idleTime, timeout));
            promise1.then((e:NullishType)=>{
                let value : boolean = e as boolean;
                resolve(value);
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    waitForComponent(on: On, time: int): Promise<Component> {
        let promise = new Promise<Component>((resolve: (value: Component) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():Component => this.waitForComponentSync(on, time));
            promise1.then((e:NullishType)=>{
                let value : Component = e as Component;
                resolve(value);
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    triggerCombineKeys(key0: int, key1: int, key2?: int): Promise<void> {
        if (key2 == null) {
            let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
            let promise1 = taskpool.execute(():boolean => this.triggerCombineKeysSync(key0, key1, 0));
                promise1.then((e:NullishType)=>{
                    resolve(Promise.resolve());
                }, (err: Object): void => {
                    let br = err as BusinessError<void>;
                    reject(br);
                });
            });
            return promise;
        } else {
            let promise1 = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
                let promise1 = taskpool.execute(():boolean => this.triggerCombineKeysSync(key0, key1, key2));
                    promise1.then((e:NullishType)=>{
                        resolve(Promise.resolve());
                    }, (err: Object): void => {
                        let br = err as BusinessError<void>;
                        reject(br);
                    });
                });
                return promise1;
        }
    }
  
    setDisplayRotationEnabled(enable: boolean): Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.setDisplayRotationEnabledSync(enable));
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    setDisplayRotation(rotation: DisplayRotation): Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.setDisplayRotationSync(rotation));
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    screenCapture(path: string, rect: Rect): Promise<boolean> {
        let promise = new Promise<boolean>((resolve: (value: boolean) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.screenCaptureSync(path, rect));
            promise1.then((e:NullishType)=>{
                let value : boolean = e as boolean;
                resolve(value);
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    screenCap(path: string): Promise<boolean> {
        let promise = new Promise<boolean>((resolve: (value: boolean) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.screenCapSync(path));
            promise1.then((e:NullishType)=>{
                let value : boolean = e as boolean;
                resolve(value);
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    penSwipe(f: Point, t: Point, speed?: int, pressure: double): Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.penSwipeSync(f, t, speed, pressure));
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    penClick(p: Point): Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.penClickSync(p));
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    penDoubleClick(p: Point): Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.penDoubleClickSync(p));
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    penLongClick(p: Point, pressure: double): Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.penLongClickSync(p, pressure));
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    mouseScroll(p: Point, down: boolean, d:int, key1?: int, key2?: int, speed?: int): Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.mouseScrollSync(p, down, d, key1, key2, speed));
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    mouseMoveWithTrack(f: Point, t: Point, speed?: int): Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.mouseMoveWithTrackSync(f, t, speed));
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    mouseMoveTo(p: Point): Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.mouseMoveToSync(p));
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    mouseDrag(f: Point, t: Point, speed?: int): Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.mouseDragkSync(f, t, speed));
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
    mouseClick(p: Point, btnId: MouseButton, key1?: int, key2?: int): Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.mouseClickSync(p, btnId, key1, key2));
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    mouseDoubleClick(p: Point, btnId: MouseButton, key1?: int, key2?: int): Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.mouseDoubleClickSync(p, btnId, key1, key2));
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    mouseLongClick(p: Point, btnId: MouseButton, key1?: int, key2?: int): Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.mouseLongClickSync(p, btnId, key1, key2));
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    injectMultiPointerAction(pointers: PointerMatrix, speed?: int): Promise<boolean> {
        let promise = new Promise<boolean>((resolve: (value: boolean) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.injectMultiPointerActionSync(pointers, speed));
            promise1.then((e:NullishType)=>{
                let value : boolean = e as boolean;
                resolve(value);
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    injectPenPointerAction(pointers: PointerMatrix, speed?: int, pressure?: double): Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.injectPenPointerActionSync(pointers, speed, pressure));
            promise1.then((e:NullishType)=>{
                resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  
    touchPadMultiFingerSwipe(fingers: int, direction: UiDirection, options?: TouchPadSwipeOptions): Promise<void> {
        let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
        let promise1 = taskpool.execute(():boolean => this.touchPadMultiFingerSwipeSync(fingers, direction, options));
            promise1.then((e:NullishType)=>{
              resolve(Promise.resolve());
            }, (err: Object): void => {
                let br = err as BusinessError<void>;
                reject(br);
            });
        });
        return promise;
    }
  }
  
  export class PointerMatrix {
    private nativePointerMatrix:string = '';
    public constructor(PointerMatrix:string) {
      if (this.nativePointerMatrix == '') {
        this.nativePointerMatrix = PointerMatrix;
      }
    }
  
    native static create(fingers: int, steps: int): PointerMatrix;
    native setPoint(finger: int, step: int, point: Point): void;
  }
  export class On {
    private nativeOn:String = '';
    public constructor(on:string) {
      hilog.error(domain, tag, "newOnBynative");
      hilog.error(domain, tag, on);
      this.nativeOn = on;
    }
    public constructor() {
        hilog.error(domain, tag, "newOn");
    }
    native within(on:On): On;
    native clickable(b?: boolean): On;
    native longClickable(b?: boolean): On;
    native scrollable(b?: boolean): On;
    native enabled(b?: boolean): On;
    native focused(b?: boolean): On;
    native selected(b?: boolean): On;
    native checked(b?: boolean): On;
    native checkable(b?: boolean): On;
    native isBefore(on: On): On;
    native isAfter(on: On): On;
    native inWindow(bundleName: string): On;
    native text(txt: string, pattern?: MatchPattern): On;
    native description(val: string, pattern?: MatchPattern): On;
    native id(id: string, pattern?: MatchPattern): On;
    native type(tp: string, pattern?: MatchPattern): On;
    native hint(val: string, pattern?: MatchPattern): On;
    
    text(txt: string): On {
        return this.text(txt, MatchPattern.EQUALS);
    }
    description(val: string): On {
        return this.description(val, MatchPattern.EQUALS);
    }
    id(id: string): On {
        return this.id(id, MatchPattern.EQUALS);
    }
    type(tp: string): On {
        return this.type(tp, MatchPattern.EQUALS);
    }
    hint(val: string): On {
        return this.hint(val, MatchPattern.EQUALS);
    }
  }

  export class UIEventObserver {
    private nativeUIEventObserver:string = '';
    public constructor(UIEventObserver:string) {
      if (this.nativeUIEventObserver == '') {
        this.nativeUIEventObserver = UIEventObserver;
      }
    }
    private native onceSync(type: String, callback: Callback<UIElementInfo>): void;
    once(type: String, callback: Callback<UIElementInfo>): Promise<void> {
      let promise = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
          let promise1 = taskpool.execute(():void => this.onceSync(type, callback));
          promise1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          }, (err: Object): void => {
              let br = err as BusinessError<void>;
              reject(br);
          });
      });
      return promise;
    }
  }
  
export const ON: On = new On();
