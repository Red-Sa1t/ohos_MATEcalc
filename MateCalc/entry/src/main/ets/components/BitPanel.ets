/*
 * 64-bit editor of the programming mode (bit_table in buttons-programming.ui).
 * Bit index 0 is the most significant bit; clicking a bit toggles it.
 */
import { CalcModel } from '../model/CalcModel';

const NIBBLES_PER_ROW: number[] = [0, 1, 2, 3, 4, 5, 6, 7];
const BITS_PER_NIBBLE: number[] = [0, 1, 2, 3];

@Component
export struct BitPanel {
  @ObjectLink model: CalcModel;

  private bitText(index: number): string {
    return this.model.bits.charAt(index);
  }

  @Builder
  bitRow(firstBit: number) {
    Row() {
      ForEach(NIBBLES_PER_ROW, (nibble: number) => {
        Row() {
          ForEach(BITS_PER_NIBBLE, (bit: number) => {
            Text(this.bitText(firstBit + nibble * 4 + bit))
              .fontSize(14)
              .fontFamily('monospace')
              .fontColor($r('app.color.display_text'))
              .width(14)
              .textAlign(TextAlign.Center)
              .onClick(() => {
                if (this.model.bitsEnabled) {
                  this.model.toggleBit(firstBit + nibble * 4 + bit);
                }
              })
          }, (bit: number) => `${firstBit}_${nibble}_${bit}`)
        }
        .layoutWeight(1)
        .justifyContent(FlexAlign.Center)
      }, (nibble: number) => `${firstBit}_${nibble}`)
    }
    .width('100%')
  }

  @Builder
  markerRow(left: string, middle: string, right: string) {
    Row() {
      Text(left).fontSize(10).fontColor($r('app.color.display_status')).layoutWeight(1).textAlign(TextAlign.Start)
      Text(middle).fontSize(10).fontColor($r('app.color.display_status')).layoutWeight(1).textAlign(TextAlign.Center)
      Text(right).fontSize(10).fontColor($r('app.color.display_status')).layoutWeight(1).textAlign(TextAlign.End)
    }
    .width('100%')
    .padding({ left: 4, right: 4 })
  }

  build() {
    Column({ space: 1 }) {
      this.bitRow(0)
      this.markerRow('63', '47', '32')
      this.bitRow(32)
      this.markerRow('31', '15', '0')
    }
    .width('100%')
    .padding(6)
    .borderRadius(6)
    .backgroundColor($r('app.color.display_bg'))
    .opacity(this.model.bitsEnabled ? 1 : 0.45)
  }
}
