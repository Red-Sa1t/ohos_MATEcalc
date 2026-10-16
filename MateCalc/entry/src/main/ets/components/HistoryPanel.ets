/*
 * History view (math-history.c / math-history-entry.c): each row shows the
 * equation and its answer. Clicking the equation puts it back in the display,
 * clicking the answer inserts it at the cursor.
 */
import { CalcModel, HistoryRow } from '../model/CalcModel';

@Component
export struct HistoryPanel {
  @ObjectLink model: CalcModel;
  private scroller: Scroller = new Scroller();

  build() {
    List({ scroller: this.scroller, space: 0 }) {
      ForEach(this.model.history, (row: HistoryRow) => {
        ListItem() {
          Row({ space: 8 }) {
            Text(row.equation)
              .fontSize(14)
              .fontColor($r('app.color.display_text'))
              .layoutWeight(1)
              .maxLines(2)
              .textOverflow({ overflow: TextOverflow.Ellipsis })
              .onClick(() => {
                this.model.historySetEquation(row.index);
              })
            Text(row.answer)
              .fontSize(14)
              .fontWeight(FontWeight.Medium)
              .fontColor($r('app.color.accent'))
              .maxLines(1)
              .textOverflow({ overflow: TextOverflow.Ellipsis })
              .constraintSize({ maxWidth: '45%' })
              .onClick(() => {
                this.model.historyInsertAnswer(row.index);
              })
          }
          .width('100%')
          .padding({ left: 10, right: 10, top: 6, bottom: 6 })
        }
      }, (row: HistoryRow) => `${row.index}_${row.equation}_${row.answer}`)
    }
    .width('100%')
    .height('100%')
    .divider({ strokeWidth: 1, color: $r('app.color.key_border') })
    .backgroundColor($r('app.color.display_bg'))
    .borderRadius(8)
    .border({ width: 1, color: $r('app.color.key_border') })
    .onAppear(() => {
      this.scroller.scrollEdge(Edge.Bottom);
    })
    .onSizeChange(() => {
      this.scroller.scrollEdge(Edge.Bottom);
    })
  }
}
