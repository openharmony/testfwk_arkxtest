class LogExpectError {
    static getErrorMsg(matcherName, actualValue, expect, originMsg) {
        if (matcherName === "assertNull") {
            return "expect not null, actualValue is " + (actualValue)
        }
        if (matcherName === "assertTrue") {
            return "expect not true, actualValue is " + (actualValue)
        }
        if (matcherName === "assertFalse") {
            return "expect not false, actualValue is " + (actualValue)
        }
        if (matcherName === "assertEqual") {
            const aClassName = Object.prototype.toString.call(actualValue);
            const bClassName = Object.prototype.toString.call(expect);
            return "expect not Equal, actualValue is "
                + actualValue + aClassName + ' equals ' + expect + bClassName
        }
        if (matcherName === "assertContain") {
            return "expect not have, " + actualValue + " have " + expect;
        }
        if (matcherName === "assertInstanceOf") {
            return "expect not InstanceOf, "
                + actualValue + ' is '
                + Object.prototype.toString.call(actualValue) +  expected;
        }
        if (matcherName === "assertLarger") {
            return "expect not Larger, "
                + (actualValue) + ' is larger than ' + expected;
        }
        if (matcherName === "assertLargerOrEqual") {
            return "expect not LargerOrEqual, "
                + (actualValue) + ' larger than ' + expected;
        }
        if (matcherName === "assertLess") {
            return "expect not Less, "
                + (actualValue) + ' less than ' + expected;
        }
        if (matcherName === "assertLessOrEqual") {
            return "expect not LessOrEqual, "
                + (actualValue) + ' is less than ' + expected;
        }
        if (matcherName === "assertNaN") {
            return "expect not NaN, actualValue is " + (actualValue)
        }
        if (matcherName === "assertNegUnlimited") {
            return "expect not NegUnlimited, actualValue is " + (actualValue)
        }
        if (matcherName === "assertPosUnlimited") {
            return "expect not PosUnlimited, actualValue is " + (actualValue)
        }
        if (matcherName === "assertUndefined") {
            return "expect not Undefined, actualValue is " + (actualValue)
        }
        return originMsg;
    }
}
export default  LogExpectError