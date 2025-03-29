class AssertException extends Error {
  public name_: string;
  public message_: string;
  constructor(message: string) {
    super();
    this.name_ = 'AssertException';
    this.message_ = message;
  }
}

export { AssertException };
