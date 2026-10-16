/*
 * Button panel of the current mode (math-buttons.c): the key grid, the
 * drop-down menus (variables, f(x), const, shifts), and in programming mode the
 * base selector and the bit editor.
 */
import { ADVANCED_TRIG_PANEL, ButtonMode, ButtonSpec, CONSTANT_MENU, FUNCTION_MENU, MenuEntry, PanelSpec,
  panelForMode } from '../model/ButtonSpecs';
import { CalcModel, NumberMode } from '../model/CalcModel';
import { CalcKey } from './CalcKey';
import { BitPanel } from './BitPanel';
import { VariablePopup } from './VariablePopup';
import { I18n } from '../common/I18n';

class BaseChoice {
  base: number = 10;
  label: string = '';

  static of(base: number, label: string): BaseChoice {
    const choice: BaseChoice = new BaseChoice();
    choice.base = base;
    choice.label = label;
    return choice;
  }
}

const BASES: BaseChoice[] = [
  BaseChoice.of(2, 'Binary'),
  BaseChoice.of(8, 'Octal'),
  BaseChoice.of(10, 'Decimal'),
  BaseChoice.of(16, 'Hexadecimal')
];

const SHIFT_COUNTS: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15];

function repeatTemplate(count: number): string {
  return new Array<string>(count).fill('1fr').join(' ');
}

@Component
export struct ButtonPanel {
  @ObjectLink model: CalcModel;
  onFinancial: (id: string) => void = () => {};
  onCharacterCode: () => void = () => {};
  @State openMenu: string = '';

  private panel(): PanelSpec {
    return panelForMode(this.model.mode);
  }

  private isActive(spec: ButtonSpec): boolean {
    if (spec.id === 'superscript') {
      return this.model.numberMode === NumberMode.SUPERSCRIPT;
    }
    if (spec.id === 'subscript') {
      return this.model.numberMode === NumberMode.SUBSCRIPT;
    }
    return false;
  }

  private handlePress(spec: ButtonSpec): void {
    if (spec.dropDown) {
      this.openMenu = spec.id;
    } else if (spec.id.startsWith('finc_')) {
      this.onFinancial(spec.id);
    } else if (spec.id === 'character') {
      this.onCharacterCode();
    } else {
      this.model.press(spec);
    }
  }

  private menuEntries(id: string): MenuElement[] {
    const result: MenuElement[] = [];
    if (id === 'function' || id === 'const') {
      const entries: MenuEntry[] = id === 'function' ? FUNCTION_MENU : CONSTANT_MENU;
      for (let i = 0; i < entries.length; i++) {
        const entry: MenuEntry = entries[i];
        const label: string = entry.detail.length > 0 ?
          `${I18n.tr(entry.name)}  (${entry.insert} = ${I18n.tr(entry.detail)})` : I18n.tr(entry.name);
        const element: MenuElement = {
          value: label,
          action: () => {
            this.model.insert(entry.insert);
          }
        };
        result.push(element);
      }
    } else if (id === 'shift_left' || id === 'shift_right') {
      const direction: number = id === 'shift_left' ? 1 : -1;
      for (let i = 0; i < SHIFT_COUNTS.length; i++) {
        const count: number = SHIFT_COUNTS[i];
        const msgid: string = count < 10 ? (count === 1 ? '_%d place' : '_%d places') :
          (count === 1 ? '%d place' : '%d places');
        const element: MenuElement = {
          value: I18n.format(msgid, [`${count}`]),
          action: () => {
            this.model.shift(direction * count);
          }
        };
        result.push(element);
      }
    }
    return result;
  }

  @Builder
  memoryPopup() {
    VariablePopup({
      model: this.model,
      onClose: () => {
        this.openMenu = '';
      }
    })
  }

  @Builder
  keyCell(spec: ButtonSpec) {
    CalcKey({
      spec: spec,
      active: this.isActive(spec),
      onPress: (pressed: ButtonSpec) => {
        this.handlePress(pressed);
      }
    })
  }

  @Builder
  trigGrid() {
    Grid() {
      ForEach(ADVANCED_TRIG_PANEL.keys, (trig: ButtonSpec) => {
        GridItem() {
          this.keyCell(trig)
        }
        .rowStart(trig.row)
        .rowEnd(trig.row)
        .columnStart(trig.col)
        .columnEnd(trig.col)
      }, (trig: ButtonSpec) => trig.id)
    }
    .rowsTemplate(repeatTemplate(ADVANCED_TRIG_PANEL.rows))
    .columnsTemplate(repeatTemplate(ADVANCED_TRIG_PANEL.cols))
    .rowsGap(4)
    .columnsGap(4)
    .width('100%')
    .height('100%')
  }

  @Builder
  keyGrid() {
    Grid() {
      ForEach(this.panel().keys, (spec: ButtonSpec) => {
        if (spec.id === 'memory') {
          GridItem() {
            this.keyCell(spec)
          }
          .rowStart(spec.row)
          .rowEnd(spec.row + spec.rowSpan - 1)
          .columnStart(spec.col)
          .columnEnd(spec.col + spec.colSpan - 1)
          .bindPopup(this.openMenu === 'memory', {
            builder: this.memoryPopup,
            placement: Placement.Bottom,
            enableArrow: true,
            autoCancel: true,
            popupColor: $r('app.color.display_bg'),
            onStateChange: (event: PopupStateChangeParam) => {
              if (!event.isVisible && this.openMenu === 'memory') {
                this.openMenu = '';
              }
            }
          })
        } else if (spec.dropDown) {
          GridItem() {
            this.keyCell(spec)
          }
          .rowStart(spec.row)
          .rowEnd(spec.row + spec.rowSpan - 1)
          .columnStart(spec.col)
          .columnEnd(spec.col + spec.colSpan - 1)
          .bindMenu(this.openMenu === spec.id, this.menuEntries(spec.id), {
            onDisappear: () => {
              if (this.openMenu === spec.id) {
                this.openMenu = '';
              }
            }
          })
        } else {
          GridItem() {
            this.keyCell(spec)
          }
          .rowStart(spec.row)
          .rowEnd(spec.row + spec.rowSpan - 1)
          .columnStart(spec.col)
          .columnEnd(spec.col + spec.colSpan - 1)
        }
      }, (spec: ButtonSpec) => `${this.model.mode}_${spec.id}`)

      if (this.model.mode === ButtonMode.ADVANCED) {
        GridItem() {
          this.trigGrid()
        }
        .rowStart(0)
        .rowEnd(2)
        .columnStart(6)
        .columnEnd(9)
      }
    }
    .rowsTemplate(repeatTemplate(this.panel().rows))
    .columnsTemplate(repeatTemplate(this.panel().cols))
    .rowsGap(4)
    .columnsGap(4)
    .width('100%')
    .layoutWeight(1)
  }

  @Builder
  programmingHeader() {
    Row({ space: 8 }) {
      Select(BASES.map((choice: BaseChoice) => {
        const option: SelectOption = { value: I18n.tr(choice.label) };
        return option;
      }))
        .selected(Math.max(0, BASES.findIndex((choice: BaseChoice) => choice.base === this.model.programmingBase)))
        .value(I18n.tr(BASES[Math.max(0, BASES.findIndex((choice: BaseChoice) =>
          choice.base === this.model.programmingBase))].label))
        .font({ size: 14 })
        .onSelect((index: number) => {
          this.model.setProgrammingBase(BASES[index].base);
        })
      Text(this.model.bitsEnabled ? this.model.bitsLabel : '')
        .fontSize(14)
        .fontColor($r('app.color.display_status'))
        .layoutWeight(1)
        .textAlign(TextAlign.End)
        .maxLines(1)
        .textOverflow({ overflow: TextOverflow.Ellipsis })
    }
    .width('100%')
  }

  build() {
    Column({ space: 6 }) {
      if (this.model.mode === ButtonMode.PROGRAMMING) {
        this.programmingHeader()
        BitPanel({ model: this.model })
      }
      this.keyGrid()
    }
    .width('100%')
    .height('100%')
  }
}
