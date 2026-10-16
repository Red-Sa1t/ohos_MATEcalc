/*
 * The display (math-display.c): right aligned equation with the answer in
 * bold, a blinking caret, mouse caret placement, and the status line.
 * Keyboard input is handled by the window and forwarded to the engine.
 */
import { CalcModel, DisplayChar } from '../model/CalcModel';

@Component
export struct CalcDisplay {
  @ObjectLink model: CalcModel;
  @Prop focused: boolean = true;
  @State caretVisible: boolean = true;
  private blinkTimer: number = -1;
  private scroller: Scroller = new Scroller();

  aboutToAppear(): void {
    this.blinkTimer = setInterval(() => {
      this.caretVisible = !this.caretVisible;
    }, 530);
  }

  aboutToDisappear(): void {
    if (this.blinkTimer >= 0) {
      clearInterval(this.blinkTimer);
    }
  }

  build() {
    Column() {
      Scroll(this.scroller) {
        Text() {
          ForEach(this.model.chars, (item: DisplayChar) => {
            if (item.caret) {
              Span('│')
                .fontWeight(FontWeight.Lighter)
                .fontColor(this.focused && this.caretVisible ? $r('app.color.accent') : Color.Transparent)
                .letterSpacing(-6)
            } else {
              Span(item.text)
                .fontWeight(item.ans ? FontWeight.Bold : FontWeight.Normal)
                .textBackgroundStyle({
                  color: item.selected ? $r('app.color.selection_bg') : Color.Transparent,
                  radius: 2
                })
                .onClick(() => {
                  // Place the caret after the clicked character
                  this.model.setCursor(item.offset + item.text.length);
                })
            }
          }, (item: DisplayChar) => item.key)
        }
        .width('100%')
        .fontSize(28)
        .fontColor($r('app.color.display_text'))
        .textAlign(TextAlign.End)
        .wordBreak(WordBreak.BREAK_ALL)
        .padding({ left: 12, right: 12, top: 10, bottom: 4 })
        .accessibilityText(this.model.display)
        .id('calc_display')
      }
      .layoutWeight(1)
      .width('100%')
      .scrollBar(BarState.Auto)
      .align(Alignment.Bottom)

      Row({ space: 6 }) {
        Text(this.model.status)
          .id('calc_status')
          .fontSize(13)
          .fontColor($r('app.color.display_status'))
          .textAlign(TextAlign.End)
          .maxLines(2)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
          .layoutWeight(1)
        if (this.model.showSpinner) {
          LoadingProgress()
            .width(18)
            .height(18)
            .color($r('app.color.accent'))
        }
      }
      .width('100%')
      .height(26)
      .padding({ left: 12, right: 12, bottom: 4 })
    }
    .width('100%')
    .height('100%')
    .backgroundColor($r('app.color.display_bg'))
    .borderRadius(8)
    .border({ width: 1, color: $r('app.color.key_border') })
  }
}
