/*
 * Variable popup of the memory button (math-variable-popup.c): lists stored
 * variables, inserts one on click, deletes it, or stores the current value
 * under a new name. "ans" and "rand" are always listed.
 */
import calc from 'libmatecalc.so';
import { VariableInfo } from 'libmatecalc.so';
import { CalcModel } from '../model/CalcModel';

@Component
export struct VariablePopup {
  @ObjectLink model: CalcModel;
  onClose: () => void = () => {};
  @State variables: VariableInfo[] = [];
  @State newName: string = '';
  @State answer: string = '';

  aboutToAppear(): void {
    this.reload();
  }

  private reload(): void {
    const list: VariableInfo[] = calc.getVariables();
    list.sort((a: VariableInfo, b: VariableInfo) => a.name.localeCompare(b.name));
    this.variables = list;
    this.answer = this.model.answerText();
  }

  private insert(name: string): void {
    this.model.recallVariable(name);
    this.onClose();
  }

  private store(): void {
    const name: string = this.newName.trim();
    if (name.length === 0) {
      return;
    }
    this.model.storeVariable(name);
    this.newName = '';
    this.onClose();
  }

  @Builder
  entry(name: string, value: string, deletable: boolean) {
    Row({ space: 4 }) {
      Button({ type: ButtonType.Normal, stateEffect: true }) {
        Text(value.length > 0 ? `${name} = ${value}` : name)
          .fontSize(14)
          .maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
          .fontColor($r('app.color.key_text'))
      }
      .layoutWeight(1)
      .height(32)
      .padding({ left: 8, right: 8 })
      .backgroundColor(Color.Transparent)
      .focusable(false)
      .onClick(() => {
        this.insert(name);
      })

      if (deletable) {
        Button({ type: ButtonType.Normal, stateEffect: true }) {
          Text('✕').fontSize(13).fontColor($r('app.color.key_text'))
        }
        .width(32)
        .height(32)
        .backgroundColor(Color.Transparent)
        .focusable(false)
        .onClick(() => {
          calc.deleteVariable(name);
          this.reload();
        })
      }
    }
    .width('100%')
  }

  build() {
    Column({ space: 2 }) {
      ForEach(this.variables, (item: VariableInfo) => {
        this.entry(item.name, item.value, true)
      }, (item: VariableInfo) => `${item.name}=${item.value}`)

      Row({ space: 4 }) {
        TextInput({ text: this.newName })
          .layoutWeight(1)
          .height(32)
          .fontSize(14)
          .onChange((value: string) => {
            // Can't have whitespace in names, so replace with underscores
            this.newName = value.replace(new RegExp('\\s', 'g'), '_');
          })
          .onSubmit(() => {
            this.store();
          })
        Button({ type: ButtonType.Normal, stateEffect: true }) {
          Text('+').fontSize(18).fontColor($r('app.color.key_text'))
        }
        .width(32)
        .height(32)
        .enabled(this.newName.trim().length > 0)
        .backgroundColor($r('app.color.key_function_bg'))
        .onClick(() => {
          this.store();
        })
      }
      .width('100%')
      .margin({ top: 4, bottom: 4 })

      this.entry('ans', this.answer, false)
      this.entry('rand', '', false)
    }
    .width(280)
    .padding(8)
  }
}
