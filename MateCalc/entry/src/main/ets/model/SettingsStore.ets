/*
 * Persistent settings, mirroring the org.mate.calc GSettings schema.
 */
import { preferences } from '@kit.ArkData';
import { common } from '@kit.AbilityKit';
import { hilog } from '@kit.PerformanceAnalysisKit';

const DOMAIN: number = 0x0C1C;

export class CalcSettings {
  accuracy: number = 9;
  wordSize: number = 64;
  base: number = 10;
  showThousands: boolean = false;
  showZeroes: boolean = false;
  showHistory: boolean = false;
  /* 0 automatic, 1 fixed, 2 scientific, 3 engineering */
  numberFormat: number = 0;
  /* 0 radians, 1 degrees, 2 gradians */
  angleUnits: number = 1;
  buttonMode: number = 0;
  sourceCurrency: string = '';
  targetCurrency: string = '';
  sourceUnits: string = 'degree';
  targetUnits: string = 'radian';
}

export class SettingsStore {
  private static store: preferences.Preferences | undefined = undefined;

  static load(context: common.Context): CalcSettings {
    const settings: CalcSettings = new CalcSettings();
    try {
      const store: preferences.Preferences = preferences.getPreferencesSync(context, { name: 'org.mate.calc' });
      SettingsStore.store = store;
      settings.accuracy = SettingsStore.clamp(store.getSync('accuracy', 9) as number, 0, 100);
      settings.wordSize = SettingsStore.clamp(store.getSync('word-size', 64) as number, 8, 64);
      settings.base = SettingsStore.clamp(store.getSync('base', 10) as number, 2, 16);
      settings.showThousands = store.getSync('show-thousands', false) as boolean;
      settings.showZeroes = store.getSync('show-zeroes', false) as boolean;
      settings.showHistory = store.getSync('show-history', false) as boolean;
      settings.numberFormat = SettingsStore.clamp(store.getSync('number-format', 0) as number, 0, 3);
      settings.angleUnits = SettingsStore.clamp(store.getSync('angle-units', 1) as number, 0, 2);
      settings.buttonMode = SettingsStore.clamp(store.getSync('button-mode', 0) as number, 0, 3);
      settings.sourceCurrency = store.getSync('source-currency', '') as string;
      settings.targetCurrency = store.getSync('target-currency', '') as string;
      settings.sourceUnits = store.getSync('source-units', 'degree') as string;
      settings.targetUnits = store.getSync('target-units', 'radian') as string;
    } catch (err) {
      hilog.warn(DOMAIN, 'Settings', 'Failed to load settings: %{public}s', JSON.stringify(err));
    }
    return settings;
  }

  static put(key: string, value: number | boolean | string): void {
    const store: preferences.Preferences | undefined = SettingsStore.store;
    if (store === undefined) {
      return;
    }
    try {
      store.putSync(key, value);
      store.flush();
    } catch (err) {
      hilog.warn(DOMAIN, 'Settings', 'Failed to save %{public}s: %{public}s', key, JSON.stringify(err));
    }
  }

  private static clamp(value: number, min: number, max: number): number {
    if (Number.isNaN(value)) {
      return min;
    }
    return Math.min(Math.max(Math.floor(value), min), max);
  }
}
