/*
 * Preferences dialog (preferences.ui / math-preferences.c).
 */
import { CalcModel } from '../model/CalcModel';
import { I18n } from '../common/I18n';

class Choice {
  value: number = 0;
  label: string = '';

  static of(value: number, label: string): Choice {
    const choice: Choice = new Choice();
    choice.value = value;
    choice.label = label;
    return choice;
  }
}

/* MPAngleUnit values: 0 radians, 1 degrees, 2 gradians */
const ANGLE_UNITS: Choice[] = [Choice.of(1, 'Degrees'), Choice.of(0, 'Radians'), Choice.of(2, 'Gradians')];
const NUMBER_FORMATS: Choice[] = [
  Choice.of(0, 'Automatic'), Choice.of(1, 'Fixed'), Choice.of(2, 'Scientific'), Choice.of(3, 'Engineering')
];
const WORD_SIZES: Choice[] = [Choice.of(8, '8-bit'), Choice.of(16, '16-bit'), Choice.of(32, '32-bit'), Choice.of(64, '64-bit')];

function options(choices: Choice[]): SelectOption[] {
  return choices.map((choice: Choice) => {
    const option: SelectOption = { value: I18n.tr(choice.label) };
    return option;
  });
}

function indexOf(choices: Choice[], value: number): number {
  return Math.max(0, choices.findIndex((choice: Choice) => choice.value === value));
}

@CustomDialog
export struct PreferencesDialog {
  controller?: CustomDialogController;
  model: CalcModel = new CalcModel();
  @State angleUnits: number = 1;
  @State numberFormat: number = 0;
  @State accuracy: number = 9;
  @State showZeroes: boolean = false;
  @State showThousands: boolean = false;
  @State wordSize: number = 64;

  aboutToAppear(): void {
    this.angleUnits = this.model.angleUnits;
    this.numberFormat = this.model.numberFormat;
    this.accuracy = this.model.accuracy;
    this.showZeroes = this.model.showZeroes;
    this.showThousands = this.model.showThousands;
    this.wordSize = this.model.wordSize;
  }

  /* "Show %d decimal _places" split around the spin button */
  private accuracyParts(): string[] {
    const text: string = I18n.tr('Show %d decimal _places');
    const index: number = text.indexOf('%d');
    if (index < 0) {
      return [text, ''];
    }
    return [text.substring(0, index).trim(), text.substring(index + 2).trim()];
  }

  @Builder
  labelled(label: string) {
    Text(label)
      .fontSize(14)
      .fontColor($r('app.color.display_text'))
      .width(140)
  }

  build() {
    Column({ space: 14 }) {
      Text(I18n.tr('Preferences'))
        .fontSize(18)
        .fontWeight(FontWeight.Medium)
        .fontColor($r('app.color.display_text'))
        .width('100%')

      Row() {
        this.labelled(I18n.tr('_Angle units:'))
        Select(options(ANGLE_UNITS))
          .selected(indexOf(ANGLE_UNITS, this.angleUnits))
          .value(I18n.tr(ANGLE_UNITS[indexOf(ANGLE_UNITS, this.angleUnits)].label))
          .font({ size: 14 })
          .onSelect((index: number) => {
            this.angleUnits = ANGLE_UNITS[index].value;
            this.model.setAngleUnits(this.angleUnits);
          })
      }
      .width('100%')

      Row() {
        this.labelled(I18n.tr('Number _Format:'))
        Select(options(NUMBER_FORMATS))
          .selected(indexOf(NUMBER_FORMATS, this.numberFormat))
          .value(I18n.tr(NUMBER_FORMATS[indexOf(NUMBER_FORMATS, this.numberFormat)].label))
          .font({ size: 14 })
          .onSelect((index: number) => {
            this.numberFormat = NUMBER_FORMATS[index].value;
            this.model.setNumberFormat(this.numberFormat);
          })
      }
      .width('100%')

      Row({ space: 8 }) {
        Text(this.accuracyParts()[0])
          .fontSize(14)
          .fontColor($r('app.color.display_text'))
        Counter() {
          Text(`${this.accuracy}`)
            .fontSize(14)
            .fontColor($r('app.color.display_text'))
        }
        .onInc(() => {
          if (this.accuracy < 100) {
            this.accuracy++;
            this.model.setAccuracy(this.accuracy);
          }
        })
        .onDec(() => {
          if (this.accuracy > 0) {
            this.accuracy--;
            this.model.setAccuracy(this.accuracy);
          }
        })
        Text(this.accuracyParts()[1])
          .fontSize(14)
          .fontColor($r('app.color.display_text'))
      }
      .width('100%')
      .padding({ left: 20 })

      Row({ space: 8 }) {
        Checkbox()
          .select(this.showZeroes)
          .onChange((value: boolean) => {
            this.showZeroes = value;
            this.model.setShowZeroes(value);
          })
        Text(I18n.tr('Show trailing _zeroes'))
          .fontSize(14)
          .fontColor($r('app.color.display_text'))
      }
      .width('100%')
      .padding({ left: 20 })

      Row({ space: 8 }) {
        Checkbox()
          .select(this.showThousands)
          .onChange((value: boolean) => {
            this.showThousands = value;
            this.model.setShowThousands(value);
          })
        Text(I18n.tr('Show _thousands separators'))
          .fontSize(14)
          .fontColor($r('app.color.display_text'))
      }
      .width('100%')
      .padding({ left: 20 })

      Row() {
        this.labelled(I18n.tr('Word _size:'))
        Select(options(WORD_SIZES))
          .selected(indexOf(WORD_SIZES, this.wordSize))
          .value(I18n.tr(WORD_SIZES[indexOf(WORD_SIZES, this.wordSize)].label))
          .font({ size: 14 })
          .onSelect((index: number) => {
            this.wordSize = WORD_SIZES[index].value;
            this.model.setWordSize(this.wordSize);
          })
      }
      .width('100%')

      Row() {
        Blank()
        Button(I18n.tr('_Close'))
          .fontSize(14)
          .onClick(() => {
            this.controller?.close();
          })
      }
      .width('100%')
    }
    .width('100%')
    .padding(20)
  }
}
