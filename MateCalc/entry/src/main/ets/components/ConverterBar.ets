/*
 * Unit / currency converter (math-converter.c): "[unit] in [unit] ⇆" and the
 * converted value of the display. Shown in advanced (all categories) and
 * financial (currencies) modes.
 */
import { CalcModel, UnitChoice } from '../model/CalcModel';
import { ButtonMode } from '../model/ButtonSpecs';
import { I18n } from '../common/I18n';

@Component
export struct ConverterBar {
  @ObjectLink model: CalcModel;

  private options(choices: UnitChoice[]): SelectOption[] {
    return choices.map((choice: UnitChoice) => {
      const option: SelectOption = { value: choice.label };
      return option;
    });
  }

  private indexOf(choices: UnitChoice[], name: string): number {
    return Math.max(0, choices.findIndex((choice: UnitChoice) => choice.name === name));
  }

  private labelOf(choices: UnitChoice[], name: string): string {
    const index: number = choices.findIndex((choice: UnitChoice) => choice.name === name);
    return index >= 0 ? choices[index].label : '';
  }

  build() {
    Row({ space: 6 }) {
      if (this.model.mode === ButtonMode.ADVANCED) {
        Select(this.options(this.model.converterCategories()))
          .selected(this.indexOf(this.model.converterCategories(), this.model.converterCategory))
          .value(this.labelOf(this.model.converterCategories(), this.model.converterCategory))
          .font({ size: 13 })
          .onSelect((index: number) => {
            this.model.setConverterCategory(this.model.converterCategories()[index].name);
          })
      }
      Select(this.options(this.model.converterUnits()))
        .selected(this.indexOf(this.model.converterUnits(), this.model.sourceUnit))
        .value(this.labelOf(this.model.converterUnits(), this.model.sourceUnit))
        .font({ size: 13 })
        .onSelect((index: number) => {
          this.model.setConversion(this.model.converterUnits()[index].name, this.model.targetUnit);
        })
      // Label displayed between the two conversion combo boxes, e.g. "[degrees] in [radians]"
      Text(I18n.tr(' in ').trim())
        .fontSize(13)
        .fontColor($r('app.color.display_status'))
      Select(this.options(this.model.converterUnits()))
        .selected(this.indexOf(this.model.converterUnits(), this.model.targetUnit))
        .value(this.labelOf(this.model.converterUnits(), this.model.targetUnit))
        .font({ size: 13 })
        .onSelect((index: number) => {
          this.model.setConversion(this.model.sourceUnit, this.model.converterUnits()[index].name);
        })
      Button({ type: ButtonType.Normal, stateEffect: true }) {
        Text('⇆').fontSize(16).fontColor($r('app.color.key_text'))
      }
      .width(36)
      .height(30)
      .borderRadius(6)
      .backgroundColor(Color.Transparent)
      .focusable(false)
      .bindTips(I18n.tr('Switch conversion units'))
      .onClick(() => {
        this.model.swapConversion();
      })
      Text(this.model.conversionText)
        .fontSize(14)
        .fontColor(this.model.conversionEnabled ? $r('app.color.display_text') : $r('app.color.display_status'))
        .layoutWeight(1)
        .textAlign(TextAlign.End)
        .maxLines(1)
        .textOverflow({ overflow: TextOverflow.Ellipsis })
    }
    .width('100%')
    .height(36)
  }
}
