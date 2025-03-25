export type PrimitiveType = int | byte | short | long | number | string | undefined | null | boolean | BigInt;
export type AnyType = PrimitiveType | Date | RegExp | object | never | (() => void); // | symbol @todo 不支持symbol
export type ItfnType =
  | (() => void)
  | (() => Promise<void>)
  | ((done: () => void) => void)
  | ((done: () => void) => Promise<void>)
  | ((done: () => void, param2: AnyType) => void)
  | ((done: () => void, param2: AnyType) => Promise<void>);
