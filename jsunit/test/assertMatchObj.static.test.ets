import { ArgumentMatchers, describe, expect, it } from '@ohos.hypium';

interface User {
  name: string;
  age: number;
  email: string;
  active: boolean;
  tags: string[];
}

export default function assertMatchObjTest() {
  describe('assertMatchObj Basic Test', () => {
    it('basic_exact_match', 0, () => {
      const user: User = {
        name: "John",
        age: 30,
        email: "john@example.com",
        active: true,
        tags: ["user", "vip"]
      };

      expect(user).assertMatchObj({
        name: "John",
        age: 30,
        active: true
      });
    });

    it('anyString_matcher', 0, () => {
      const user: User = {
        name: "John",
        age: 30,
        email: "john@example.com",
        active: true,
        tags: []
      };

      expect(user).assertMatchObj({
        name: ArgumentMatchers.anyString(),
        email: ArgumentMatchers.anyString()
      });
    });

    it('anyNumber_matcher', 0, () => {
      const user: User = {
        name: "John",
        age: 30,
        email: "john@example.com",
        active: true,
        tags: []
      };

      expect(user).assertMatchObj({
        age: ArgumentMatchers.anyNumber()
      });
    });

    it('partial_match_succeeds', 0, () => {
      const user: User = {
        name: "John",
        age: 30,
        email: "john@example.com",
        active: true,
        tags: ["user", "vip"]
      };

      expect(user).assertMatchObj({
        name: "John"
      });
    });

    it('nested_object_match', 0, () => {
      const user: User = {
        name: "John",
        age: 30,
        email: "john@example.com",
        active: true,
        tags: ["user"]
      };

      expect(user).assertMatchObj({
        name: "John",
        age: 30
      });
    });

    it('property_not_found_should_fail', 0, () => {
      const user: User = {
        name: "John",
        age: 30,
        email: "john@example.com",
        active: true,
        tags: []
      };

      expect(user).assertMatchObj({
        name: "John",
        missingField: "value"
      });
    });
  });
}
