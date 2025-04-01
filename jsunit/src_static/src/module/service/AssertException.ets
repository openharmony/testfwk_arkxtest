class AssertException extends Error {
  constructor(message: string) {
    super(message);
    super.name = 'AssertException';
  }
}

export { AssertException };
