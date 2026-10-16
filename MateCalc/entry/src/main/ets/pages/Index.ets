/*
 * Main calculator window (math-window.c): menu bar, history, display,
 * converter and button panel, plus the keyboard handling of math-display.c.
 */
import calc from 'libmatecalc.so';
import { common } from '@kit.AbilityKit';
import { pasteboard } from '@kit.BasicServicesKit';
import { BusinessError } from '@kit.BasicServicesKit';
import { hilog } from '@kit.PerformanceAnalysisKit';
import { ButtonMode, FINANCIAL_DIALOGS } from '../model/ButtonSpecs';
import { CalcModel, NumberMode } from '../model/CalcModel';
import { CalcSettings, SettingsStore } from '../model/SettingsStore';
import { CalcDisplay } from '../components/CalcDisplay';
import { ButtonPanel } from '../components/ButtonPanel';
import { HistoryPanel } from '../components/HistoryPanel';
import { ConverterBar } from '../components/ConverterBar';
import { PreferencesDialog } from '../dialogs/PreferencesDialog';
import { FinancialDialog } from '../dialogs/FinancialDialog';
import { AboutDialog, CharacterCodeDialog, HelpDialog } from '../dialogs/InfoDialogs';
import { KeyCodes } from '../common/KeyCodes';
import { I18n } from '../common/I18n';
import { WindowHolder } from '../common/WindowHolder';

const DOMAIN: number = 0x0C1C;
const SUPERSCRIPT_DIGITS: string[] = ['⁰', '¹', '²', '³', '⁴', '⁵', '⁶', '⁷', '⁸', '⁹'];
const SUBSCRIPT_DIGITS: string[] = ['₀', '₁', '₂', '₃', '₄', '₅', '₆', '₇', '₈', '₉'];
/* US layout symbols for key codes without a unicode value: [plain, shifted] */
const SYMBOL_KEYS: Record<string, string[]> = {
  '2043': [',', '<'], '2044': ['.', '>'], '2050': [' ', ' '], '2056': ['`', '~'], '2057': ['-', '_'],
  '2058': ['=', '+'], '2059': ['[', '{'], '2060': [']', '}'], '2061': ['\\', '|'], '2062': [';', ':'],
  '2063': ['\'', '"'], '2064': ['/', '?'], '2066': ['+', '+'], '2010': ['*', '*'],
  '2113': ['/', '/'], '2114': ['*', '*'], '2115': ['-', '-'], '2116': ['+', '+'], '2118': [',', ','],
  '2120': ['=', '='], '2121': ['(', '('], '2122': [')', ')']
};
const SHIFTED_DIGITS: string = ')!@#$%^&*(';

@Entry
@Component
struct Index {
  @State model: CalcModel = new CalcModel();
  @State openMenu: string = '';

  private preferencesDialog: CustomDialogController = new CustomDialogController({
    builder: PreferencesDialog({ model: this.model }),
    autoCancel: true,
    width: 460
  });
  private characterDialog: CustomDialogController = new CustomDialogController({
    builder: CharacterCodeDialog({ model: this.model }),
    autoCancel: true,
    width: 400
  });
  private aboutDialog: CustomDialogController = new CustomDialogController({
    builder: AboutDialog(),
    autoCancel: true,
    width: 480
  });
  private helpDialog: CustomDialogController = new CustomDialogController({
    builder: HelpDialog(),
    autoCancel: true,
    width: 560
  });
  private financialDialog: CustomDialogController = new CustomDialogController({
    builder: FinancialDialog({ model: this.model }),
    autoCancel: true,
    width: 520
  });

  aboutToAppear(): void {
    const context: common.UIAbilityContext = this.getUIContext().getHostContext() as common.UIAbilityContext;
    I18n.load(context.resourceManager);
    const keys: string[] = I18n.catalogKeys();
    calc.setTranslations(keys, I18n.catalogValues(keys));
    const settings: CalcSettings = SettingsStore.load(context);
    this.model.init(`${context.filesDir}/registers`, context.cacheDir, settings);
    WindowHolder.fitToMode(this.model.mode, this.model.showHistory);
  }

  /* Menu actions */

  private setMode(mode: number): void {
    this.model.setMode(mode);
    WindowHolder.fitToMode(mode, this.model.showHistory);
  }

  private toggleHistory(): void {
    this.model.setShowHistory(!this.model.showHistory);
    WindowHolder.fitToMode(this.model.mode, this.model.showHistory);
  }

  private copy(): void {
    const text: string = this.model.copyText();
    this.writeClipboard(text);
  }

  private cut(): void {
    this.writeClipboard(this.model.cutText());
  }

  private writeClipboard(text: string): void {
    try {
      const data: pasteboard.PasteData = pasteboard.createData(pasteboard.MIMETYPE_TEXT_PLAIN, text);
      pasteboard.getSystemPasteboard().setData(data).catch((err: BusinessError) => {
        hilog.warn(DOMAIN, 'Index', 'Copy failed: %{public}s', err.message);
      });
    } catch (err) {
      hilog.warn(DOMAIN, 'Index', 'Copy failed: %{public}s', JSON.stringify(err));
    }
  }

  /* Reading the pasteboard is allowed after a PasteButton click; from the
   * keyboard it may be refused, then point the user at the menu item. */
  private paste(fromPasteButton: boolean): void {
    try {
      pasteboard.getSystemPasteboard().getData().then((data: pasteboard.PasteData) => {
        this.model.paste(data.getPrimaryText());
      }).catch((err: BusinessError) => {
        hilog.warn(DOMAIN, 'Index', 'Paste failed: %{public}s', err.message);
        if (!fromPasteButton) {
          this.model.showMessage(`${I18n.tr('_Calculator')} › ${I18n.tr('_Paste')}`);
        }
      });
    } catch (err) {
      hilog.warn(DOMAIN, 'Index', 'Paste failed: %{public}s', JSON.stringify(err));
    }
  }

  private quit(): void {
    const context: common.UIAbilityContext = this.getUIContext().getHostContext() as common.UIAbilityContext;
    try {
      context.terminateSelf().catch((err: BusinessError) => {
        hilog.error(DOMAIN, 'Index', 'Quit failed: %{public}s', err.message);
      });
    } catch (err) {
      hilog.error(DOMAIN, 'Index', 'Quit failed: %{public}s', JSON.stringify(err));
    }
  }

  private openFinancial(id: string): void {
    if (FINANCIAL_DIALOGS[id] === undefined) {
      return;
    }
    this.model.financialDialog = id;
    this.financialDialog.open();
  }

  /* Keyboard (math-display.c display_key_press_cb, math-window.c key_press_cb, menu accelerators) */

  private handleKey(event: KeyEvent): boolean {
    const code: number = event.keyCode;
    const ctrl: boolean = event.getModifierKeyState !== undefined && event.getModifierKeyState(['Ctrl']);
    const shift: boolean = event.getModifierKeyState !== undefined && event.getModifierKeyState(['Shift']);
    const alt: boolean = event.getModifierKeyState !== undefined && event.getModifierKeyState(['Alt']);
    const model: CalcModel = this.model;

    /* Menu accelerators */
    if (ctrl && shift) {
      if (code === KeyCodes.Z) {
        model.redo();
        return true;
      }
      if (code === KeyCodes.H) {
        this.toggleHistory();
        return true;
      }
      if (code === KeyCodes.FORWARD_DEL) {
        model.clearHistory();
        return true;
      }
    }
    if (ctrl && !shift && !alt) {
      if (code === KeyCodes.C) {
        this.copy();
        return true;
      }
      if (code === KeyCodes.X) {
        this.cut();
        return true;
      }
      if (code === KeyCodes.V) {
        this.paste(false);
        return true;
      }
      if (code === KeyCodes.A) {
        model.selectAll();
        return true;
      }
      if (code === KeyCodes.Z) {
        model.undo();
        return true;
      }
      if (code === KeyCodes.Q || code === KeyCodes.W) {
        this.quit();
        return true;
      }
    }
    if (code === KeyCodes.F1) {
      this.helpDialog.open();
      return true;
    }

    /* Keyboard navigation for history */
    if (alt && (code === KeyCodes.DPAD_UP || code === KeyCodes.DPAD_DOWN)) {
      model.historyNavigate(code === KeyCodes.DPAD_UP ? -1 : 1);
      return true;
    }

    /* Base shortcuts in programming mode */
    if (model.mode === ButtonMode.PROGRAMMING && ctrl) {
      const bases: Record<string, number> = { '2018': 2, '2031': 8, '2020': 10, '2024': 16 };
      const base: number | undefined = bases[`${code}`];
      if (base !== undefined) {
        model.setProgrammingBase(base);
        return true;
      }
    }

    /* Solve on enter */
    if (code === KeyCodes.ENTER || code === KeyCodes.NUMPAD_ENTER) {
      model.solve();
      return true;
    }

    /* Clear on escape */
    if ((code === KeyCodes.ESCAPE && !ctrl && !alt) || (code === KeyCodes.BACKSPACE && ctrl) ||
      (code === KeyCodes.FORWARD_DEL && shift)) {
      model.clear();
      return true;
    }

    /* Numeric keypad will often insert '.' regardless of locale */
    if (code === KeyCodes.NUMPAD_DOT) {
      model.insertNumericPoint();
      return true;
    }

    /* Editing keys of the text view */
    if (code === KeyCodes.BACKSPACE) {
      model.backspace();
      return true;
    }
    if (code === KeyCodes.FORWARD_DEL) {
      model.deleteForward();
      return true;
    }
    if (code === KeyCodes.DPAD_LEFT || code === KeyCodes.DPAD_RIGHT) {
      model.moveCursor(code === KeyCodes.DPAD_LEFT ? -1 : 1, shift);
      return true;
    }
    if (code === KeyCodes.MOVE_HOME || code === KeyCodes.MOVE_END) {
      model.moveCursorTo(code === KeyCodes.MOVE_HOME ? 0 : model.display.length, shift);
      return true;
    }

    /* Shortcuts */
    if (ctrl && !alt) {
      const inserts: Record<string, string> = {
        '2059': '⌈', '2060': '⌉', '2025': '⁻¹', '2032': 'π', '2034': '√', '2037': 'µ', '2057': '⁻', '2063': '°'
      };
      const text: string | undefined = inserts[`${code}`];
      if (text !== undefined) {
        model.insert(text);
        return true;
      }
      if (code === KeyCodes.E) {
        model.insertExponent();
        return true;
      }
      if (code === KeyCodes.F) {
        model.factorize();
        return true;
      }
    }
    if (alt && !ctrl) {
      if (code === KeyCodes.LEFT_BRACKET) {
        model.insert('⌊');
        return true;
      }
      if (code === KeyCodes.RIGHT_BRACKET) {
        model.insert('⌋');
        return true;
      }
    }

    const digit: number = Index.digitOf(code);
    if (digit >= 0 && !shift) {
      if (ctrl || model.numberMode === NumberMode.SUPERSCRIPT) {
        model.insert(SUPERSCRIPT_DIGITS[digit]);
        return true;
      }
      if (alt || model.numberMode === NumberMode.SUBSCRIPT) {
        model.insert(SUBSCRIPT_DIGITS[digit]);
        return true;
      }
    }
    if (ctrl || alt) {
      return false;
    }

    /* Typed characters */
    const c: string = Index.characterOf(event, shift);
    if (c.length === 0) {
      return false;
    }
    if (c === '*') {
      model.insert('×');
    } else if (c === '/') {
      model.insert('÷');
    } else if (c === '-') {
      model.insertSubtract();
    } else if (c >= '0' && c <= '9') {
      model.insertDigit(c.charCodeAt(0) - 48);
    } else {
      model.insert(c);
    }
    return true;
  }

  private static digitOf(code: number): number {
    if (code >= KeyCodes.DIGIT_0 && code <= KeyCodes.DIGIT_9) {
      return code - KeyCodes.DIGIT_0;
    }
    if (code >= KeyCodes.NUMPAD_0 && code <= KeyCodes.NUMPAD_9) {
      return code - KeyCodes.NUMPAD_0;
    }
    return -1;
  }

  private static characterOf(event: KeyEvent, shift: boolean): string {
    const unicode: number = event.unicode ?? 0;
    if (unicode >= 0x20 && unicode !== 0x7F) {
      return String.fromCodePoint(unicode);
    }
    const code: number = event.keyCode;
    if (code >= KeyCodes.DIGIT_0 && code <= KeyCodes.DIGIT_9) {
      return shift ? SHIFTED_DIGITS.charAt(code - KeyCodes.DIGIT_0) : `${code - KeyCodes.DIGIT_0}`;
    }
    if (code >= KeyCodes.NUMPAD_0 && code <= KeyCodes.NUMPAD_9) {
      return `${code - KeyCodes.NUMPAD_0}`;
    }
    if (code >= KeyCodes.A && code <= KeyCodes.Z) {
      const letter: string = String.fromCharCode(97 + code - KeyCodes.A);
      const upper: boolean = shift !== (event.isCapsLockOn ?? false);
      return upper ? letter.toUpperCase() : letter;
    }
    const symbols: string[] | undefined = SYMBOL_KEYS[`${code}`];
    if (symbols !== undefined) {
      return shift ? symbols[1] : symbols[0];
    }
    return '';
  }

  /* Menu bar */

  /* Paste needs the PasteButton security component to read the pasteboard */
  @Builder
  pasteMenuItem() {
    // Security components need an opaque background and a bounded size
    PasteButton({ text: PasteDescription.PASTE, buttonType: ButtonType.Normal })
      .width(240)
      .height(40)
      .borderRadius(8)
      .backgroundColor($r('app.color.menu_item_bg'))
      .fontColor($r('app.color.display_text'))
      .fontSize(16)
      .padding({ left: 12 })
      .align(Alignment.Start)
      .onClick((event: ClickEvent, result: PasteButtonOnClickResult) => {
        if (result === PasteButtonOnClickResult.SUCCESS) {
          this.paste(true);
        }
        this.closeMenu('calculator');
      })
  }

  @Builder
  calculatorMenu() {
    Menu() {
      MenuItem({ content: I18n.tr('_Copy'), labelInfo: 'Ctrl+C' })
        .onClick(() => {
          this.copy();
        })
      MenuItem(this.pasteMenuItem)
      MenuItem({ content: I18n.tr('_Undo'), labelInfo: 'Ctrl+Z' })
        .onClick(() => {
          this.model.undo();
        })
      MenuItem({ content: I18n.tr('_Redo'), labelInfo: 'Shift+Ctrl+Z' })
        .onClick(() => {
          this.model.redo();
        })
      MenuItem({ content: I18n.tr('_Clear History'), labelInfo: 'Shift+Ctrl+Delete' })
        .onClick(() => {
          this.model.clearHistory();
        })
      MenuItemGroup() {
        MenuItem({ content: I18n.tr('_Preferences') })
          .onClick(() => {
            this.preferencesDialog.open();
          })
      }
      MenuItemGroup() {
        MenuItem({ content: I18n.tr('_Quit'), labelInfo: 'Ctrl+Q' })
          .onClick(() => {
            this.quit();
          })
      }
    }
  }

  @Builder
  modeItem(label: string, mode: number) {
    MenuItem({ content: I18n.tr(label) })
      .selected(this.model.mode === mode)
      .selectIcon(true)
      .onClick(() => {
        this.setMode(mode);
      })
  }

  @Builder
  modeMenu() {
    Menu() {
      this.modeItem('_Basic', ButtonMode.BASIC)
      this.modeItem('_Advanced', ButtonMode.ADVANCED)
      this.modeItem('_Financial', ButtonMode.FINANCIAL)
      this.modeItem('_Programming', ButtonMode.PROGRAMMING)
    }
  }

  @Builder
  viewMenu() {
    Menu() {
      MenuItem({ content: I18n.tr('_History'), labelInfo: 'Shift+Ctrl+H' })
        .selected(this.model.showHistory)
        .selectIcon(true)
        .onClick(() => {
          this.toggleHistory();
        })
    }
  }

  @Builder
  helpMenu() {
    Menu() {
      MenuItem({ content: I18n.tr('_Contents'), labelInfo: 'F1' })
        .onClick(() => {
          this.helpDialog.open();
        })
      MenuItem({ content: I18n.tr('_About') })
        .onClick(() => {
          this.aboutDialog.open();
        })
    }
  }

  @Builder
  menuButton(label: string, menu: string) {
    Text(I18n.tr(label))
      .fontSize(14)
      .fontColor($r('app.color.display_text'))
      .padding({ left: 10, right: 10, top: 4, bottom: 4 })
      .borderRadius(4)
      .backgroundColor(this.openMenu === menu ? $r('app.color.key_function_bg') : Color.Transparent)
      .onClick(() => {
        this.openMenu = menu;
      })
  }

  @Builder
  menuBar() {
    Row({ space: 2 }) {
      Column() {
        this.menuButton('_Calculator', 'calculator')
      }
      .bindMenu(this.openMenu === 'calculator', this.calculatorMenu, {
        onDisappear: () => {
          this.closeMenu('calculator');
        }
      })
      Column() {
        this.menuButton('_Mode', 'mode')
      }
      .bindMenu(this.openMenu === 'mode', this.modeMenu, {
        onDisappear: () => {
          this.closeMenu('mode');
        }
      })
      Column() {
        this.menuButton('_View', 'view')
      }
      .bindMenu(this.openMenu === 'view', this.viewMenu, {
        onDisappear: () => {
          this.closeMenu('view');
        }
      })
      Column() {
        this.menuButton('_Help', 'help')
      }
      .bindMenu(this.openMenu === 'help', this.helpMenu, {
        onDisappear: () => {
          this.closeMenu('help');
        }
      })
    }
    .width('100%')
    .height(32)
    .padding({ left: 6 })
    .backgroundColor($r('app.color.menu_bar_bg'))
  }

  private closeMenu(menu: string): void {
    if (this.openMenu === menu) {
      this.openMenu = '';
    }
  }

  build() {
    Column() {
      this.menuBar()
      Column({ space: 8 }) {
        if (this.model.showHistory) {
          HistoryPanel({ model: this.model })
            .height(190)
        }
        CalcDisplay({ model: this.model })
          .height(this.model.mode === ButtonMode.BASIC ? 104 : 112)
        if (this.model.mode === ButtonMode.ADVANCED || this.model.mode === ButtonMode.FINANCIAL) {
          ConverterBar({ model: this.model })
        }
        ButtonPanel({
          model: this.model,
          onFinancial: (id: string) => {
            this.openFinancial(id);
          },
          onCharacterCode: () => {
            this.characterDialog.open();
          }
        })
          .layoutWeight(1)
      }
      .layoutWeight(1)
      .width('100%')
      .padding(8)
    }
    .width('100%')
    .height('100%')
    .backgroundColor($r('app.color.window_bg'))
    .focusable(true)
    .defaultFocus(true)
    .onKeyEvent((event: KeyEvent) => {
      if (event.type === KeyType.Down && this.handleKey(event)) {
        event.stopPropagation();
      }
    })
  }
}
