/*
 * One calculator key (a GtkButton of the upstream button panels).
 */
import { ButtonSpec, KeyKind } from '../model/ButtonSpecs';
import { I18n } from '../common/I18n';

@Component
export struct CalcKey {
  spec: ButtonSpec = new ButtonSpec();
  /* Pressed state of the ↑n / ↓n toggle buttons */
  @Prop active: boolean = false;
  @Prop keyEnabled: boolean = true;
  onPress: (spec: ButtonSpec) => void = () => {};

  private keyBackground(): Resource {
    if (this.active) {
      return $r('app.color.key_active_bg');
    }
    switch (this.spec.kind) {
      case KeyKind.NUMBER:
        return $r('app.color.key_number_bg');
      case KeyKind.OPERATOR:
        return $r('app.color.key_operator_bg');
      case KeyKind.RESULT:
        return $r('app.color.key_result_bg');
      case KeyKind.GROUP:
      case KeyKind.ACTION:
        return $r('app.color.key_group_bg');
      default:
        return $r('app.color.key_function_bg');
    }
  }

  private keyForeground(): Resource {
    if (this.spec.kind === KeyKind.RESULT || this.active) {
      return $r('app.color.key_result_text');
    }
    return $r('app.color.key_text');
  }

  private labelText(): string {
    // Text labels such as "Clear" and "Undo" are translated, symbols are not
    if (this.spec.id === 'clear' || this.spec.id === 'undo') {
      return I18n.tr(this.spec.label);
    }
    return this.spec.label;
  }

  private tipText(): string {
    return this.spec.tip.length > 0 ? I18n.tr(this.spec.tip) : '';
  }

  build() {
    Button({ type: ButtonType.Normal, stateEffect: true }) {
      Row({ space: 2 }) {
        Text(this.labelText())
          .fontSize(this.spec.kind === KeyKind.NUMBER || this.spec.kind === KeyKind.OPERATOR ||
            this.spec.kind === KeyKind.RESULT ? 18 : 15)
          .fontWeight(this.spec.kind === KeyKind.NUMBER || this.spec.kind === KeyKind.RESULT ?
            FontWeight.Medium : FontWeight.Normal)
          .fontStyle(this.spec.italic ? FontStyle.Italic : FontStyle.Normal)
          .fontColor(this.keyForeground())
          .maxLines(1)
          .minFontSize(10)
          .maxFontSize(18)
          .textOverflow({ overflow: TextOverflow.Clip })
        if (this.spec.dropDown) {
          Polygon({ width: 8, height: 5 })
            .points([[0, 0], [8, 0], [4, 5]])
            .fill(this.keyForeground())
            .margin({ left: 3 })
        }
      }
      .justifyContent(FlexAlign.Center)
    }
    .width('100%')
    .height('100%')
    .borderRadius(6)
    .backgroundColor(this.keyBackground())
    .border({ width: 1, color: $r('app.color.key_border') })
    .enabled(this.keyEnabled)
    .opacity(this.keyEnabled ? 1 : 0.4)
    .focusable(false)
    .bindTips(this.tipText())
    .accessibilityText(this.tipText().length > 0 ? this.tipText() : this.labelText())
    .onClick(() => {
      this.onPress(this.spec);
    })
  }
}
