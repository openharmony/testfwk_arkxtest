import { AbilityConstant, UIAbility, Want } from '@kit.AbilityKit';
import abilityDelegatorRegistry from '@ohos.app.ability.abilityDelegatorRegistry';

import { Hypium } from '../index';

import testsuite from '../test/List.test';
const ON_DESTROY_ERROR = -2;

export default class TestAbility extends UIAbility {
  private abilityDelegator: abilityDelegatorRegistry.AbilityDelegator;

  constructor() {
    super();
    this.abilityDelegator = abilityDelegatorRegistry.getAbilityDelegator();
  }

  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam) {
    const abilityDelegatorArguments = abilityDelegatorRegistry.getArguments();
    Hypium.hypiumTest(this.abilityDelegator, abilityDelegatorArguments, testsuite);
  }

  onDestroy() {
    this.abilityDelegator.finishTest('TestAbility onDestory unexpectedly!', ON_DESTROY_ERROR);
  }
  onWindowStageDestroy() {}

  onForeground() {}

  onBackground() {}
}
