/*
 * Financial function dialogs (buttons-financial.ui). Enter moves to the next
 * field and calculates on the last one, like finc_activate_cb.
 */
import { FINANCIAL_DIALOGS, FinancialDialogSpec } from '../model/ButtonSpecs';
import { CalcModel } from '../model/CalcModel';
import { I18n } from '../common/I18n';

@CustomDialog
export struct FinancialDialog {
  controller?: CustomDialogController;
  model: CalcModel = new CalcModel();
  spec: FinancialDialogSpec = new FinancialDialogSpec();
  @State values: string[] = [];

  aboutToAppear(): void {
    const spec: FinancialDialogSpec | undefined = FINANCIAL_DIALOGS[this.model.financialDialog];
    if (spec !== undefined) {
      this.spec = spec;
    }
    const initial: string[] = [];
    for (let i = 0; i < this.spec.fields.length; i++) {
      initial.push('');
    }
    this.values = initial;
  }

  private calculate(): void {
    const args: string[] = this.values.map((value: string) => {
      const trimmed: string = value.trim().replace(new RegExp(',', 'g'), '.');
      return trimmed.length > 0 ? trimmed : '0';
    });
    this.model.financial(this.spec.func, args);
    this.controller?.close();
  }

  build() {
    Column({ space: 12 }) {
      Text(I18n.tr(this.spec.title))
        .fontSize(18)
        .fontWeight(FontWeight.Medium)
        .fontColor($r('app.color.display_text'))
        .width('100%')
      Text(I18n.tr(this.spec.description).trim())
        .fontSize(13)
        .fontColor($r('app.color.display_status'))
        .width('100%')

      ForEach(this.spec.fields, (field: string, index: number) => {
        Row({ space: 8 }) {
          Text(I18n.tr(field))
            .fontSize(14)
            .fontColor($r('app.color.display_text'))
            .width(150)
          TextInput({ text: this.values[index], placeholder: '0' })
            .layoutWeight(1)
            .height(36)
            .fontSize(14)
            .defaultFocus(index === 0)
            .id(`finc_field_${index}`)
            .onChange((value: string) => {
              const next: string[] = this.values.slice();
              next[index] = value;
              this.values = next;
            })
            .onSubmit(() => {
              if (index + 1 < this.spec.fields.length) {
                this.getUIContext().getFocusController().requestFocus(`finc_field_${index + 1}`);
              } else {
                this.calculate();
              }
            })
        }
        .width('100%')
      }, (field: string, index: number) => `${this.spec.func}_${index}`)

      Row({ space: 12 }) {
        Blank()
        Button(I18n.tr('_Cancel'))
          .fontSize(14)
          .backgroundColor($r('app.color.key_function_bg'))
          .fontColor($r('app.color.key_text'))
          .onClick(() => {
            this.controller?.close();
          })
        Button(I18n.tr('C_alculate'))
          .fontSize(14)
          .onClick(() => {
            this.calculate();
          })
      }
      .width('100%')
    }
    .width('100%')
    .padding(20)
  }
}
