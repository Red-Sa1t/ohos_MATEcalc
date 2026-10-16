/*
 * UI state of the calculator window and the actions behind buttons, menus and
 * keyboard shortcuts. The calculation state itself lives in the native engine
 * (libmatecalc.so, a port of MathEquation); after every call the returned
 * EngineState is copied here so ArkUI re-renders.
 */
import calc from 'libmatecalc.so';
import { BitsInfo, ConversionInfo, EngineSettings, EngineState, HistoryInfo, UnitCategoryInfo,
  UnitInfo } from 'libmatecalc.so';
import { intl } from '@kit.LocalizationKit';
import { ButtonMode, ButtonSpec } from './ButtonSpecs';
import { CalcSettings, SettingsStore } from './SettingsStore';
import { CurrencyService, RateFiles } from './CurrencyService';
import { I18n } from '../common/I18n';

export enum NumberMode {
  NORMAL = 0,
  SUPERSCRIPT = 1,
  SUBSCRIPT = 2
}

/* One rendered character (or the caret) of the display */
export class DisplayChar {
  key: string = '';
  text: string = '';
  /* UTF-16 offset of this character in the display text */
  offset: number = 0;
  ans: boolean = false;
  selected: boolean = false;
  caret: boolean = false;
}

export class HistoryRow {
  index: number = 0;
  equation: string = '';
  answer: string = '';
}

export class UnitChoice {
  name: string = '';
  label: string = '';
}

const BIT_COUNT: number = 64;

@Observed
export class CalcModel {
  /* Display */
  display: string = '';
  selectionStart: number = 0;
  selectionEnd: number = 0;
  ansStart: number = -1;
  ansEnd: number = -1;
  status: string = '';
  numberMode: number = NumberMode.NORMAL;
  isResult: boolean = false;
  inSolve: boolean = false;
  showSpinner: boolean = false;
  chars: DisplayChar[] = [];

  /* Mode and preferences */
  mode: number = ButtonMode.BASIC;
  programmingBase: number = 10;
  base: number = 10;
  accuracy: number = 9;
  numberFormat: number = 0;
  angleUnits: number = 1;
  wordSize: number = 64;
  showThousands: boolean = false;
  showZeroes: boolean = false;
  showHistory: boolean = false;

  /* Programming mode bit panel */
  bitsEnabled: boolean = false;
  bits: string = '0'.repeat(BIT_COUNT);
  bitsLabel: string = '';

  /* History panel */
  history: HistoryRow[] = [];
  private historyCursor: number = -1;

  /* Unit / currency converter */
  unitCategories: UnitCategoryInfo[] = [];
  converterCategory: string = '';
  sourceUnit: string = '';
  targetUnit: string = '';
  conversionText: string = '';
  conversionEnabled: boolean = false;
  private sourceUnits: string = 'degree';
  private targetUnits: string = 'radian';
  private sourceCurrency: string = '';
  private targetCurrency: string = '';

  /* Button id of the financial dialog being shown */
  financialDialog: string = '';

  private currency: CurrencyService | undefined = undefined;
  private spinnerTimer: number = -1;

  /* Startup: settings, locale characters, rates, initial state */
  init(variablesFile: string, cacheDir: string, settings: CalcSettings): void {
    this.apply(calc.init(variablesFile));

    this.accuracy = settings.accuracy;
    this.wordSize = settings.wordSize;
    this.programmingBase = settings.base;
    this.showThousands = settings.showThousands;
    this.showZeroes = settings.showZeroes;
    this.showHistory = settings.showHistory;
    this.numberFormat = settings.numberFormat;
    this.angleUnits = settings.angleUnits;
    this.sourceUnits = settings.sourceUnits;
    this.targetUnits = settings.targetUnits;
    this.sourceCurrency = settings.sourceCurrency;
    this.targetCurrency = settings.targetCurrency;

    const engineSettings: EngineSettings = {
      accuracy: this.accuracy,
      showThousandsSeparators: this.showThousands,
      showTrailingZeroes: this.showZeroes,
      numberFormat: this.numberFormat,
      wordSize: this.wordSize,
      angleUnits: this.angleUnits
    };
    const separators: string[] = CalcModel.localeSeparators();
    engineSettings.radix = separators[0];
    engineSettings.thousandsSeparator = separators[1];
    this.apply(calc.setSettings(engineSettings));

    this.unitCategories = calc.getUnitCategories();
    this.currency = new CurrencyService(cacheDir);
    const cached: RateFiles = this.currency.readCached();
    if (cached.imf.length > 0 || cached.ecb.length > 0) {
      this.apply(calc.loadCurrencyRates(cached.imf, cached.ecb));
    }

    this.setMode(settings.buttonMode);
  }

  /* Radix and thousands separator of the system locale (nl_langinfo upstream) */
  private static localeSeparators(): string[] {
    let radix: string = '.';
    let separator: string = ',';
    try {
      const formatted: string = new intl.NumberFormat(I18n.systemLocale()).format(1234.5);
      // e.g. "1,234.5", "1.234,5", "1 234,5"
      if (formatted.length >= 7) {
        separator = formatted.charAt(1);
        radix = formatted.charAt(formatted.length - 2);
      }
    } catch (err) {
      // Keep the defaults
    }
    if (radix === separator) {
      separator = radix === '.' ? ',' : '.';
    }
    return [radix, separator];
  }

  /* Copy engine state into observable fields */
  apply(state: EngineState): void {
    this.display = state.display;
    this.selectionStart = state.selectionStart;
    this.selectionEnd = state.selectionEnd;
    this.ansStart = state.ansStart;
    this.ansEnd = state.ansEnd;
    this.status = I18n.status(state.status);
    this.numberMode = state.numberMode;
    this.isResult = state.isResult;
    this.inSolve = state.inSolve;
    this.chars = this.buildChars();
    this.updateBits();
    this.updateConversion();
  }

  private buildChars(): DisplayChar[] {
    const result: DisplayChar[] = [];
    const caretAt: number = this.selectionStart === this.selectionEnd ? this.selectionEnd : -1;
    let offset: number = 0;
    let index: number = 0;
    const text: string = this.display;
    while (offset <= text.length) {
      if (offset === caretAt) {
        const caret: DisplayChar = new DisplayChar();
        caret.key = `caret_${offset}`;
        caret.text = '​';
        caret.offset = offset;
        caret.caret = true;
        result.push(caret);
      }
      if (offset === text.length) {
        break;
      }
      const code: number = text.codePointAt(offset) ?? 0;
      const width: number = code > 0xFFFF ? 2 : 1;
      const item: DisplayChar = new DisplayChar();
      item.text = text.substring(offset, offset + width);
      item.offset = offset;
      item.ans = this.ansStart >= 0 && offset >= this.ansStart && offset < this.ansEnd;
      item.selected = offset >= Math.min(this.selectionStart, this.selectionEnd) &&
        offset < Math.max(this.selectionStart, this.selectionEnd);
      item.key = `${index}_${item.text}_${item.ans ? 1 : 0}${item.selected ? 1 : 0}`;
      result.push(item);
      offset += width;
      index++;
    }
    return result;
  }

  private updateBits(): void {
    if (this.mode !== ButtonMode.PROGRAMMING) {
      return;
    }
    const info: BitsInfo = calc.getBits();
    this.bitsEnabled = info.enabled;
    this.bits = info.bits;
    this.bitsLabel = info.label;
  }

  /* Button actions (math-buttons.c callbacks) */

  press(spec: ButtonSpec): void {
    const id: string = spec.id;
    if (id.startsWith('digit_')) {
      this.apply(calc.insertDigit(Number.parseInt(id.substring(6))));
    } else if (id === 'numeric_point') {
      this.apply(calc.insertNumericPoint());
    } else if (id === 'subtract') {
      this.apply(calc.insertSubtract());
    } else if (id === 'exponential') {
      this.apply(calc.insertExponent());
    } else if (id === 'result') {
      this.solve();
    } else if (id === 'factor') {
      this.factorize();
    } else if (id === 'clear') {
      this.apply(calc.clear());
    } else if (id === 'undo') {
      this.apply(calc.undo());
    } else if (id === 'superscript') {
      this.toggleNumberMode(NumberMode.SUPERSCRIPT);
    } else if (id === 'subscript') {
      this.toggleNumberMode(NumberMode.SUBSCRIPT);
    } else if (spec.insert.length > 0) {
      this.insert(spec.insert);
    }
  }

  insert(text: string): void {
    this.apply(calc.insert(text));
  }

  insertDigit(digit: number): void {
    this.apply(calc.insertDigit(digit));
  }

  insertNumericPoint(): void {
    this.apply(calc.insertNumericPoint());
  }

  insertSubtract(): void {
    this.apply(calc.insertSubtract());
  }

  insertExponent(): void {
    this.apply(calc.insertExponent());
  }

  insertCharacterCode(text: string): void {
    this.apply(calc.insertCharacterCode(text));
  }

  private toggleNumberMode(mode: number): void {
    this.apply(calc.setNumberMode(this.numberMode === mode ? NumberMode.NORMAL : mode));
  }

  setNumberMode(mode: number): void {
    this.apply(calc.setNumberMode(mode));
  }

  clear(): void {
    this.apply(calc.clear());
  }

  undo(): void {
    this.apply(calc.undo());
  }

  redo(): void {
    this.apply(calc.redo());
  }

  backspace(): void {
    this.apply(calc.backspace());
  }

  deleteForward(): void {
    this.apply(calc.deleteChar());
  }

  async solve(): Promise<void> {
    this.startSpinner();
    const state: EngineState = await calc.solve();
    this.stopSpinner();
    this.apply(state);
    this.refreshHistory();
  }

  async factorize(): Promise<void> {
    this.startSpinner();
    const state: EngineState = await calc.factorize();
    this.stopSpinner();
    this.apply(state);
    this.refreshHistory();
  }

  /* "Calculating" indicator after 100 ms like math_equation_show_in_progress */
  private startSpinner(): void {
    this.stopSpinner();
    this.spinnerTimer = setTimeout(() => {
      this.showSpinner = true;
      this.status = I18n.tr('Calculating');
    }, 100);
  }

  private stopSpinner(): void {
    if (this.spinnerTimer >= 0) {
      clearTimeout(this.spinnerTimer);
      this.spinnerTimer = -1;
    }
    this.showSpinner = false;
  }

  /* Cursor and selection (keyboard / mouse in the display) */

  setCursor(offset: number): void {
    this.apply(calc.setSelection(offset, offset));
  }

  moveCursor(delta: number, extend: boolean): void {
    let target: number = this.selectionEnd + delta;
    if (!extend && this.selectionStart !== this.selectionEnd) {
      // Collapse the selection towards the movement direction
      target = delta < 0 ? Math.min(this.selectionStart, this.selectionEnd) :
        Math.max(this.selectionStart, this.selectionEnd);
    }
    target = Math.max(0, Math.min(this.display.length, target));
    this.apply(calc.setSelection(extend ? this.selectionStart : target, target));
  }

  moveCursorTo(offset: number, extend: boolean): void {
    this.apply(calc.setSelection(extend ? this.selectionStart : offset, offset));
  }

  selectAll(): void {
    this.apply(calc.setSelection(0, this.display.length));
  }

  copyText(): string {
    return calc.copy();
  }

  cutText(): string {
    const text: string = calc.copy();
    if (this.selectionStart !== this.selectionEnd) {
      this.apply(calc.backspace());
    }
    return text;
  }

  paste(text: string): void {
    if (text.length > 0) {
      this.apply(calc.insert(text));
    }
  }

  /* Mode, base and preferences */

  setMode(mode: number): void {
    this.mode = mode;
    SettingsStore.put('button-mode', mode);
    this.base = mode === ButtonMode.PROGRAMMING ? this.programmingBase : 10;
    this.apply(calc.setSettings({ base: this.base }));
    this.setupConverter();
    if (mode === ButtonMode.FINANCIAL) {
      this.updateRates();
    }
    this.refreshHistory();
  }

  setProgrammingBase(base: number): void {
    this.programmingBase = base;
    SettingsStore.put('base', base);
    if (this.mode === ButtonMode.PROGRAMMING) {
      this.base = base;
      this.apply(calc.setSettings({ base: base }));
      this.refreshHistory();
    }
  }

  setAccuracy(accuracy: number): void {
    this.accuracy = Math.max(0, Math.min(100, Math.floor(accuracy)));
    SettingsStore.put('accuracy', this.accuracy);
    this.apply(calc.setSettings({ accuracy: this.accuracy }));
    this.refreshHistory();
  }

  setNumberFormat(format: number): void {
    this.numberFormat = format;
    SettingsStore.put('number-format', format);
    this.apply(calc.setSettings({ numberFormat: format }));
    this.refreshHistory();
  }

  setAngleUnits(units: number): void {
    this.angleUnits = units;
    SettingsStore.put('angle-units', units);
    this.apply(calc.setSettings({ angleUnits: units }));
  }

  setWordSize(size: number): void {
    this.wordSize = size;
    SettingsStore.put('word-size', size);
    this.apply(calc.setSettings({ wordSize: size }));
  }

  setShowThousands(visible: boolean): void {
    this.showThousands = visible;
    SettingsStore.put('show-thousands', visible);
    this.apply(calc.setSettings({ showThousandsSeparators: visible }));
    this.refreshHistory();
  }

  setShowZeroes(visible: boolean): void {
    this.showZeroes = visible;
    SettingsStore.put('show-zeroes', visible);
    this.apply(calc.setSettings({ showTrailingZeroes: visible }));
    this.refreshHistory();
  }

  setShowHistory(visible: boolean): void {
    this.showHistory = visible;
    SettingsStore.put('show-history', visible);
    this.refreshHistory();
  }

  /* Programming mode */

  toggleBit(bit: number): void {
    this.apply(calc.toggleBit(bit));
    this.refreshHistory();
  }

  shift(count: number): void {
    this.apply(calc.shift(count));
    this.refreshHistory();
  }

  /* Variables */

  storeVariable(name: string): void {
    this.apply(calc.store(name));
  }

  recallVariable(name: string): void {
    this.apply(calc.recall(name));
  }

  /* Financial dialogs */

  financial(func: number, args: string[]): void {
    this.apply(calc.financial(func, args));
    this.refreshHistory();
  }

  /* History (math-history.c) */

  refreshHistory(): void {
    if (!this.showHistory) {
      return;
    }
    const entries: HistoryInfo[] = calc.getHistory();
    const rows: HistoryRow[] = [];
    for (let i = 0; i < entries.length; i++) {
      const row: HistoryRow = new HistoryRow();
      row.index = i;
      row.equation = entries[i].equation;
      row.answer = entries[i].answer;
      rows.push(row);
    }
    this.history = rows;
    this.historyCursor = rows.length;
  }

  historySetEquation(index: number): void {
    this.apply(calc.historySetEquation(index));
  }

  historyInsertAnswer(index: number): void {
    this.apply(calc.historyInsertAnswer(index));
  }

  /* Alt+Up / Alt+Down history navigation (math-window.c key_press_cb) */
  historyNavigate(delta: number): void {
    const count: number = calc.getHistory().length;
    if (count === 0) {
      return;
    }
    if (this.historyCursor < 0 || this.historyCursor > count) {
      this.historyCursor = count;
    }
    this.historyCursor = Math.max(0, Math.min(count - 1, this.historyCursor + delta));
    this.apply(calc.historySetEquation(this.historyCursor));
  }

  clearHistory(): void {
    calc.clearHistory();
    this.history = [];
    this.historyCursor = -1;
  }

  /* Converter (math-converter.c) */

  private setupConverter(): void {
    if (this.mode === ButtonMode.FINANCIAL) {
      this.converterCategory = 'currency';
      let source: string = this.sourceCurrency;
      let target: string = this.targetCurrency;
      if (!this.categoryHasUnit('currency', source) || !this.categoryHasUnit('currency', target)) {
        const chinese: boolean = I18n.getLanguage().startsWith('zh');
        source = chinese ? 'USD' : 'EUR';
        target = chinese ? 'CNY' : 'USD';
      }
      this.sourceUnit = source;
      this.targetUnit = target;
    } else if (this.mode === ButtonMode.ADVANCED) {
      let category: string = this.findCategory(this.sourceUnits);
      let source: string = this.sourceUnits;
      let target: string = this.targetUnits;
      if (category.length === 0 || !this.categoryHasUnit(category, target)) {
        category = 'angle';
        source = 'degree';
        target = 'radian';
      }
      this.converterCategory = category;
      this.sourceUnit = source;
      this.targetUnit = target;
    } else {
      this.converterCategory = '';
    }
    this.updateConversion();
  }

  private findCategory(unit: string): string {
    for (let i = 0; i < this.unitCategories.length; i++) {
      if (this.categoryHasUnit(this.unitCategories[i].name, unit)) {
        return this.unitCategories[i].name;
      }
    }
    return '';
  }

  private categoryHasUnit(category: string, unit: string): boolean {
    const units: UnitInfo[] = this.unitsOf(category);
    for (let i = 0; i < units.length; i++) {
      if (units[i].name === unit) {
        return true;
      }
    }
    return false;
  }

  private unitsOf(category: string): UnitInfo[] {
    for (let i = 0; i < this.unitCategories.length; i++) {
      if (this.unitCategories[i].name === category) {
        return this.unitCategories[i].units;
      }
    }
    return [];
  }

  /* Categories offered by the converter: all in advanced mode, currency in financial mode */
  converterCategories(): UnitChoice[] {
    const result: UnitChoice[] = [];
    for (let i = 0; i < this.unitCategories.length; i++) {
      const category: UnitCategoryInfo = this.unitCategories[i];
      if (this.mode === ButtonMode.FINANCIAL && category.name !== 'currency') {
        continue;
      }
      const choice: UnitChoice = new UnitChoice();
      choice.name = category.name;
      choice.label = I18n.tr(category.displayName);
      result.push(choice);
    }
    return result;
  }

  converterUnits(): UnitChoice[] {
    const units: UnitInfo[] = this.unitsOf(this.converterCategory);
    const result: UnitChoice[] = [];
    for (let i = 0; i < units.length; i++) {
      const choice: UnitChoice = new UnitChoice();
      choice.name = units[i].name;
      choice.label = this.converterCategory === 'currency' ?
        `${units[i].name} — ${I18n.tr(units[i].displayName)}` : I18n.tr(units[i].displayName);
      result.push(choice);
    }
    return result;
  }

  setConverterCategory(category: string): void {
    if (category === this.converterCategory) {
      return;
    }
    const units: UnitInfo[] = this.unitsOf(category);
    if (units.length === 0) {
      return;
    }
    this.converterCategory = category;
    this.sourceUnit = units[0].name;
    this.targetUnit = units.length > 1 ? units[1].name : units[0].name;
    this.saveConversion();
    this.updateConversion();
  }

  setConversion(source: string, target: string): void {
    this.sourceUnit = source;
    this.targetUnit = target;
    this.saveConversion();
    this.updateConversion();
  }

  /* ⇆ button: convert the display and swap the units */
  swapConversion(): void {
    if (this.conversionEnabled) {
      this.apply(calc.applyConversion(this.converterCategory, this.sourceUnit, this.targetUnit));
      this.refreshHistory();
    }
    this.setConversion(this.targetUnit, this.sourceUnit);
  }

  private saveConversion(): void {
    if (this.converterCategory === 'currency') {
      this.sourceCurrency = this.sourceUnit;
      this.targetCurrency = this.targetUnit;
      SettingsStore.put('source-currency', this.sourceUnit);
      SettingsStore.put('target-currency', this.targetUnit);
    } else {
      this.sourceUnits = this.sourceUnit;
      this.targetUnits = this.targetUnit;
      SettingsStore.put('source-units', this.sourceUnit);
      SettingsStore.put('target-units', this.targetUnit);
    }
  }

  private updateConversion(): void {
    if (this.converterCategory.length === 0) {
      this.conversionEnabled = false;
      this.conversionText = '';
      return;
    }
    const info: ConversionInfo = calc.convert(this.converterCategory, this.sourceUnit, this.targetUnit);
    this.conversionEnabled = info.ok;
    this.conversionText = info.ok ? `${info.source} = ${info.target}` : '';
  }

  /* Exchange rates: download in the background when stale */
  updateRates(): void {
    const service: CurrencyService | undefined = this.currency;
    if (service === undefined) {
      return;
    }
    service.update().then((files: RateFiles | undefined) => {
      if (files !== undefined) {
        this.apply(calc.loadCurrencyRates(files.imf, files.ecb));
      }
    });
  }

  ratesAvailable(): boolean {
    return calc.hasCurrencyRates();
  }

  /* Show an already translated message in the status line */
  showMessage(message: string): void {
    this.apply(calc.setStatus(''));
    this.status = message;
  }

  answerText(): string {
    return calc.getAnswer();
  }
}
