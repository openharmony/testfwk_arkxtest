class AssertException extends Error {
  private name: string;
  private message: string;
  constructor(message: string) {
    super();
    this.name = 'AssertException';
    this.message = message;
  }
}

export { AssertException };
