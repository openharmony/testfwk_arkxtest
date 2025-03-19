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

enum iDirection {
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
  left: number;
  top: number;
  right: number;
  bottom: number;
}

class RectInner implements Rect {
  left: number;
  top: number;
  right: number;
  bottom: number;
}

interface WindowFilter {
  bundleName?: string;
  title?: string;
  focused?: boolean;
  actived?: boolean;
  active?: boolean;
}

class WindowFilterInner implements WindowFilter {
  bundleName?: string;
  title?: string;
  focused?: boolean;
  actived?: boolean;
  active?: boolean;
}

interface UIElementInfo {
  bundleName: string;
  type: string;
  text: string;
}

class UIElementInfoInner implements UIElementInfo {
  bundleName: string;
  type: string;
  text: string;
}

interface TouchPadSwipeOptions {
  stay?: boolean;
  speed?: number;
}

class TouchPadSwipeOptionsInner implements TouchPadSwipeOptions {
  stay?: boolean;
  speed?: number;
}

export class On {
  static{
    loadLibrary("uitest.z");
  }
  private nativeOn:string = '';
  public constructor(on:string) {
    this.nativeOn = on;
  }
  public constructor() {
  }
  static $_instantiate(factory: ()=> On): On {
    let x = factory()
    return x
  }
  native within(on:On): On;
  native text(txt: string, pattern?: MatchPattern): On;
  native id(id: string): On;
  native type(tp: string): On;
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
  native description(val: string, pattern?: MatchPattern): On;
  native id(id: string, pattern: MatchPattern): On;
  native type(tp: string, pattern: MatchPattern): On;
  native hint(val: string, pattern?: MatchPattern): On;
}

export class Component {
  static{
    loadLibrary("uitest.z");
  }
  private nativeComponent:string = '';
  public constructor(Component:string) {
    if (this.nativeComponent == '') {
      this.nativeComponent = Component;
    }
  }
  native comClickSync(): boolean;
  native comLongClickSync(): boolean;
  native comDoubleClickSync(): boolean;
  native comDragToSync(target: Component): boolean;
  native getTextSync(): string;
  native getTypeSync(): string;
  native getIdSync(): string;
  native getHintSync(): string;
  native getDescriptionSync(): string;
  native comInputTextSync(text: string): boolean;
  native clearTextSync(): boolean;
  native scrollToTopSync(speed?: number): boolean;
  native scrollToBottomSync(speed?: number): boolean;
  native scrollSearchSync(on: On, vertical?: boolean, offset?: number): boolean;
  native pinchInSync(scale: number): boolean;
  native pinchOutSync(scale: number): boolean;
  native isScrollableSync(): boolean;
  native isSelectedSync(): boolean;
  native isLongClickableSync(): boolean;
  native isClickableSync(): boolean;
  native isFocusedSync(): boolean;
  native isEnabledSync(): boolean;
  native isCheckedSync(): boolean;
  native isCheckableSync(): boolean;

  click():Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.comClickSync());
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  longClick():Promise<void> {
    let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
    let p1 = taskpool.execute(():boolean => this.comLongClickSync());
        p1.then((e:NullishType)=>{
            resolve(Promise.resolve());
        });
    });
    return p;
  }

  doubleClick():Promise<void> {
    let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
    let p1 = taskpool.execute(():boolean => this.comDoubleClickSync());
        p1.then((e:NullishType)=>{
            resolve(Promise.resolve());
        });
    });
    return p;
  }
  
  scrollSearch(on: On, vertical?: boolean, offset?: number):Promise<Component> {
    let p = new Promise<Component>((resolve: (value: Component) => void, reject: (error: Object) => void) => {
    let p1 = taskpool.execute(():boolean => this.scrollSearchSync(on, vertical, offset));
        p1.then((e:NullishType)=>{
            let value : Component = e as Component;
            resolve(value);
        });
    });
    return p;
  }

  pinchIn(scale: number):Promise<void> {
    let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
    let p1 = taskpool.execute(():boolean => this.pinchInSync(scale));
        p1.then((e:NullishType)=>{
            resolve(Promise.resolve());
        });
    });
    return p;
  }  
  pinchOut(scale: number):Promise<void> {
    let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
    let p1 = taskpool.execute(():boolean => this.pinchOutSync(scale));
        p1.then((e:NullishType)=>{
            resolve(Promise.resolve());
        });
    });
    return p;
  }
  
  scrollToTop(speed?: number):Promise<void> {
    let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
    let p1 = taskpool.execute(():boolean => this.scrollToTopSync(speed));
        p1.then((e:NullishType)=>{
            resolve(Promise.resolve());
        });
    });
    return p;
  }
  
  scrollToBottom(speed?: number):Promise<void> {
    let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
    let p1 = taskpool.execute(():boolean => this.scrollToBottomSync(speed));
        p1.then((e:NullishType)=>{
            resolve(Promise.resolve());
        });
    });
    return p;
  }
  
  inputText(text: string):Promise<void> {
    let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
    let p1 = taskpool.execute(():boolean => this.comInputTextSync(text));
        p1.then((e:NullishType)=>{
            resolve(Promise.resolve());
        });
    });
    return p;
  }
  
  dragTo(target: Component):Promise<void> {
    let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
    let p1 = taskpool.execute(():boolean => this.comDragToSync(target));
        p1.then((e:NullishType)=>{
            resolve(Promise.resolve());
        });
    });
    return p;
  }

  clearText():Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.clearTextSync());
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  getHint():Promise<string> {
      let p = new Promise<string>((resolve: (value: string) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():string => this.getHintSync());
          p1.then((e:NullishType)=>{
              let value : string = e as string;
              resolve(value);
          });
      });
      return p;
  }

  getDescription():Promise<string> {
      let p = new Promise<string>((resolve: (value: string) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():string => this.getDescriptionSync());
          p1.then((e:NullishType)=>{
              let value : string = e as string;
              resolve(value);
          });
      });
      return p;
  }

  getHint():Promise<string> {
      let p = new Promise<string>((resolve: (value: string) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():string => this.getHintSync());
          p1.then((e:NullishType)=>{
              let value : string = e as string;
              resolve(value);
          });
      });
      return p;
  }

  getType():Promise<string> {
      let p = new Promise<string>((resolve: (value: string) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():string => this.getTypeSync());
          p1.then((e:NullishType)=>{
              let value : string = e as string;
              resolve(value);
          });
      });
      return p;
  }

  getText():Promise<string> {
      let p = new Promise<string>((resolve: (value: string) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():string => this.getTextSync());
          p1.then((e:NullishType)=>{
              let value : string = e as string;
              resolve(value);
          });
      });
      return p;
  }

  isChecked():Promise<boolean> {
    let p = new Promise<boolean>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
    let p1 = taskpool.execute(():boolean => this.isCheckedSync());
        p1.then((e:NullishType)=>{
            let value : boolean = e as boolean;
            resolve(value);
        });
    });
    return p;
  }
  isCheckable():Promise<boolean> {
    let p = new Promise<boolean>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
    let p1 = taskpool.execute(():boolean => this.isCheckableSync());
        p1.then((e:NullishType)=>{
            let value : boolean = e as boolean;
            resolve(value);
        });
    });
    return p;
  }
  isScrollable():Promise<boolean> {
    let p = new Promise<boolean>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
    let p1 = taskpool.execute(():boolean => this.isScrollableSync());
        p1.then((e:NullishType)=>{
            let value : boolean = e as boolean;
            resolve(value);
        });
    });
    return p;
  }
  isFocused():Promise<boolean> {
    let p = new Promise<boolean>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
    let p1 = taskpool.execute(():boolean => this.isFocusedSync());
        p1.then((e:NullishType)=>{
            let value : boolean = e as boolean;
            resolve(value);
        });
    });
    return p;
  }
  isEnabled():Promise<boolean> {
    let p = new Promise<boolean>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
    let p1 = taskpool.execute(():boolean => this.isEnabledSync());
        p1.then((e:NullishType)=>{
            let value : boolean = e as boolean;
            resolve(value);
        });
    });
    return p;
  }
  isSelected():Promise<boolean> {
    let p = new Promise<boolean>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
    let p1 = taskpool.execute(():boolean => this.isSelectedSync());
        p1.then((e:NullishType)=>{
            let value : boolean = e as boolean;
            resolve(value);
        });
    });
    return p;
  }
  isLongClickable():Promise<boolean> {
    let p = new Promise<boolean>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
    let p1 = taskpool.execute(():boolean => this.isLongClickableSync());
        p1.then((e:NullishType)=>{
            let value : boolean = e as boolean;
            resolve(value);
        });
    });
    return p;
  }
  isClickable():Promise<boolean> {
    let p = new Promise<boolean>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
    let p1 = taskpool.execute(():boolean => this.isClickableSync());
        p1.then((e:NullishType)=>{
            let value : boolean = e as boolean;
            resolve(value);
        });
    });
    return p;
  }
  getBounds():Promise<Rect> {
    let p = new Promise<Rect>((resolve: (value: Rect) => void, reject: (error: Object) => void) => {
    let p1 = taskpool.execute(():Rect => this.getBoundsSync());
        p1.then((e:NullishType)=>{
            let value : Rect = e as Rect;
            resolve(value);
        });
    });
    return p;
  }
  getBoundsCenter():Promise<Point> {
    let p = new Promise<Point>((resolve: (value: Point) => void, reject: (error: Object) => void) => {
    let p1 = taskpool.execute(():Point => this.getBoundsCenterSync());
        p1.then((e:NullishType)=>{
            let value : Point = e as Point;
            resolve(value);
        });
    });
    return p;
  }
}

export class UiWindow {
  nativeWindow:string = '';
  public constructor(Window:string) {
    this.nativeWindow = Window;
  }
  native splitSync(): boolean;
  native resumeSync(): boolean;
  native closeSync(): boolean;
  native minimizeSync(): boolean;
  native maximizeSync(): boolean;
  native focusSync(): boolean;
  native isFocusedSync(): boolean;
  native isActiveSync(): boolean;
  native resizeSync(w:int, h:int, d:ResizeDirection): boolean;
  native moveToSync(x:int, y:int): boolean;
  native getWindowModeSync(): int;
  native GetBundleNameSync(): string;
  native getTitileSync(): string;
  native getBoundsSync(): Rect;

  split():Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.splitSync());
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  resume():Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.resumeSync());
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  close():Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.closeSync());
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  minimize():Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.minimizeSync());
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  maximize():Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.maximizeSync());
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  focus():Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.focusSync());
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  isFocused():Promise<boolean> {
      let p = new Promise<void>((resolve: (value: PromiseLike<boolean>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.isFocusedSync());
          p1.then((e:NullishType)=>{
              let value : boolean = e as boolean;
              resolve(value);
          });
      });
      return p;
  }

  isActive():Promise<boolean> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.isActiveSync());
          p1.then((e:NullishType)=>{
              let value : boolean = e as boolean;
              resolve(value);
          });
      });
      return p;
  }

  resize(w:int, h:int, d:ResizeDirection):Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.resizeSync(w, h, d));
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  moveTo(x:int, y:int):Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.moveToSync(x, y));
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  getWindowMode():Promise<int> {
      let p = new Promise<int>((resolve: (value: PromiseLike<int>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():int => this.getWindowModeSync());
          p1.then((e:NullishType)=>{
              let value : int = e as int;
              resolve(value);
          });
      });
      return p;
  }

  GetBundleName():Promise<string> {
      let p = new Promise<string>((resolve: (value: string) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():string => this.GetBundleNameSync());
          p1.then((e:NullishType)=>{
              let value : string = e as string;
              resolve(value);
          });
      });
      return p;
  }

  getTitile():Promise<string> {
      let p = new Promise<string>((resolve: (value: string) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():string => this.getTitileSync());
          p1.then((e:NullishType)=>{
              let value : string = e as string;
              resolve(value);
          });
      });
      return p;
  }

  getBounds():Promise<Rect> {
      let p = new Promise<Rect>((resolve: (value: Rect) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():Rect => this.getBoundsSync());
          p1.then((e:NullishType)=>{
              let value : Rect = e as Rect;
              resolve(value);
          });
      });
      return p;
  }
}

export class Driver {
  native static ScheduleEstablishConnection(tokken: String): void;
  static{
    loadLibrary("uitest.z");
  }  
  private nativeDriver:string = '';
  public constructor(driver:driver) {
    if (this.nativeDriver == '') {
      this.nativeDriver = driver;
    }
  }
  private native comClickSync(x:int, y:int):boolean;
  private native delayMsSync(t:int):boolean;
  private native findComponentSync(on: On):boolean;

  native static create():Driver;
  native clickSync(x:int, y:int):boolean;
  native longClickSync(x:int, y:int):boolean;
  native doubleClickSync(x:int, y:int):boolean;
  native flingSync(from:Point, to: Point, stepLen: int, speed: int):boolean;
  native swipeSync(startx: int, starty: int, endx: int, endy: int, speed?: int):boolean;
  native dragSync(startx: int, starty: int, endx: int, endy: int, speed?: int):boolean;
  native pressBackSync():boolean;
  native assertComponentExistSync(on: On):boolean;
  native triggerKeySync(keyCode: number):boolean;
  native inputTextSync(p: Point, text: string):boolean;
  native findWindowSync(filter: WindowFilter):UiWindow;
  native wakeUpDisplaySync():boolean;
  native pressHomeSync():boolean;
  native getDisplaySizeSync():Point;
  native getDisplaySizeDensitySync():Point;
  native getDisplayRotationSync():int;
  // native createUIEventObserverSync():UIEventObserver;
  native findComponentsSync(on: On):Array<Component>;

  native waitForIdleSync(idleTime: number, timeout: number):boolean;
  native waitForComponentSync(on: On, time: number):Component;
  native triggerCombineKeysSync(key0: number, key1: number, key2?: number):boolean;

  native setDisplayRotationEnabledSync(enable: boolean):boolean;
  native setDisplayRotationSync(rotation: DisplayRotation):boolean;
  native screenCaptureSync(savepath: string, rect?: Rect):boolean;
  native screenCapSync(savepath: string):boolean;

  native penSwipeSync(f: Point, t: Point, speed?: number, pressure: number): boolean;
  native penClickSync(p: Point):boolean;
  native penDoubleClickSync(p: Point):boolean;
  native penLongClickSync(p: Point, pressure?: number):boolean;

  native mouseScrollSync(p: Point, down: boolean, d:number, key1?: number, key2?: number, speed?: number): boolean;
  native mouseMoveWithTrackSync(f: Point, t: Point, speed?: number): boolean;
  native mouseMoveToSync(p: Point): boolean;
  native mouseDragkSync(f: Point, t: Point, speed?: number): boolean;
  native mouseClickSync(p: Point, btnId: MouseButton, key1?: number, key2: number):boolean;
  native mouseDoubleClickSync(p: Point, btnId: MouseButton, key1?: number, key2: number):boolean;
  native mouseLongClickSync(p: Point, btnId: MouseButton, key1?: number, key2: number):boolean;

  native injectMultiPointerActionSync(pointers: PointerMatrix, speed?: number): boolean;
  native injectPenPointerActionSync(pointers: PointerMatrix, speed?: number, pressure?: number): void;
  native touchPadMultiFingerSwipeSync(fingers: number, direction: number, options?: TouchPadSwipeOptions): boolean;

  delayMs(t: int): Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.delayMsSync(t));
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  click(x: int, y: int): Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.clickSync(x, y));
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  doubleClick(x: int, y: int): Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.doubleClickSync(x, y));
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  findComponent(on: On): Promise<Component> {
      let p = new Promise<Component>((resolve: (c: Component) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():Component => this.findComponentSync(on));
          p1.then((e:NullishType)=>{
              let value : Component = e as Component;
              resolve(value);
          });
      });
      return p;
  }

  waitForIdle(idleTime: number, timeout: number): Promise<boolean> {
      let p = new Promise<boolean>((resolve: (c: boolean) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.waitForIdleSync(idleTime, timeout));
          p1.then((e:NullishType)=>{
              let value : boolean = e as boolean;
              resolve(value);
          });
      });
      return p;
  }

  waitForComponent(on: On): Promise<Component> {
      let p = new Promise<Component>((resolve: (c: Component) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():Component => this.waitForComponentSync(on));
          p1.then((e:NullishType)=>{
              let value : Component = e as Component;
              resolve(value);
          });
      });
      return p;
  }

  triggerCombineKeys(key0: number, key1: number, key2?: number): Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.triggerCombineKeysSync(key0, key1, key2));
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  setDisplayRotationEnabled(enable: boolean): Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.setDisplayRotationEnabledSync(enable));
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  setDisplayRotation(rotation: DisplayRotation): Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.setDisplayRotationSync(rotation));
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  screenCapture(path: string, rect: Rect): Promise<boolean> {
      let p = new Promise<boolean>((resolve: (c: boolean) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.screenCaptureSync(path, rect));
          p1.then((e:NullishType)=>{
              let value : boolean = e as boolean;
              resolve(value);
          });
      });
      return p;
  }

  screenCap(path: string): Promise<boolean> {
      let p = new Promise<boolean>((resolve: (c: boolean) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.screenCapSync(path));
          p1.then((e:NullishType)=>{
              let value : boolean = e as boolean;
              resolve(value);
          });
      });
      return p;
  }

  penSwipe(f: Point, t: Point, speed?: number, pressure: number): Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.penSwipeSync(f, t, speed, pressure));
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  penClick(p: Point): Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.penClickSync(p));
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  penDoubleClick(p: Point): Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.penDoubleClickSync(p));
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  penLongClick(p: Point, pressure: number): Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.penLongClickSync(p, pressure));
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  mouseScroll(p: Point, down: boolean, d:number, key1?: number, key2?: number, speed?: number): Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.mouseScrollSync(p, down, d, key1, key2, speed));
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  mouseMoveWithTrack(f: Point, t: Point, speed?: number): Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.mouseMoveWithTrackSync(f, t, speed));
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  mouseMoveTo(p: Point): Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.mouseMoveToSync(p));
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  mouseDragk(f: Point, t: Point, speed?: number): Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.mouseDragkSync(f, t, speed));
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  mouseClick(p: Point): Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.mouseClickSync(p));
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  mouseDoubleClick(p: Point): Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():boolean => this.mouseDoubleClickSync(p));
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  mouseLongClick(p: Point): Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():void => this.mouseLongClick(p));
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  injectMultiPointerAction(pointers: PointerMatrix, speed?: number): Promise<boolean> {
      let p = new Promise<boolean>((resolve: (c: boolean) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():void => this.injectMultiPointerActionSync(pointers, speed));
          p1.then((e:NullishType)=>{
              let value : boolean = e as boolean;
              resolve(value);
          });
      });
      return p;
  }

  injectPenPointerAction(pointers: PointerMatrix, speed?: number, pressure?: number): Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void, reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():void => this.injectPenPointerActionSync(pointers, speed, pressure));
          p1.then((e:NullishType)=>{
              resolve(Promise.resolve());
          });
      });
      return p;
  }

  touchPadMultiFingerSwipe(fingers: number, direction: UiDirection, options?: TouchPadSwipeOptions): Promise<void> {
      let p = new Promise<void>((resolve: (value: PromiseLike<void>) => void reject: (error: Object) => void) => {
      let p1 = taskpool.execute(():void => this.touchPadMultiFingerSwipeSync(fingers, direction, options));
          p1.then((e:NullishType)=>{
            resolve(Promise.resolve());
          });
      });
      return p;
  }
}

export class PointerMatrix {
  private nativePointerMatrix:string = '';
  public constructor(PointerMatrix:string) {
    if (this.nativePointerMatrix == '') {
      this.nativePointerMatrix = PointerMatrix;
    }
  }

  native static create(fingers: number, steps: number): PointerMatrix;
  native setPoint(finger: number, step: number, point: Point): void;
}

const ON: On = new On();
