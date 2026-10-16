/*
 * Small dialogs: Insert Character Code (buttons-programming.ui), About
 * (math-window.c about_cb) and the keyboard shortcut help that replaces the
 * yelp help viewer.
 */
import calc from 'libmatecalc.so';
import { LibraryVersions } from 'libmatecalc.so';
import { CalcModel } from '../model/CalcModel';
import { I18n } from '../common/I18n';

export const APP_VERSION: string = '1.28.0';

/* src/mate-calc.about */
const AUTHORS: string[] = [
  'Abel McClendon', 'Adam Erdman', 'Alexander von Gluck IV', 'Alexei Sorokin', 'Allan Nordhøy',
  'Christopher Fujino', 'Clement Lefebvre', 'Friedel Wolff', 'JP Cimalando', 'Klaus Niederkrüger',
  'Laszlo Boros', 'Laurent Napias', 'Marcel Dijkstra', 'Mark Thomas', 'Martin Wimpress', 'Michael Terry',
  'Mike Gabriel', 'Nikolay Martynov', 'Pablo Barciela', 'Perberos', 'Piotr Drąg', 'Rich Burridge',
  'Robert Ancell', 'Robert Buj', 'Sander Sweers', 'Scott Balneaves', 'Stefan Tauner', 'Stefano Karapetsas',
  'Stephen Krauth', 'Steve Zesch', 'Victor Kareh', 'Vlad Orlov', 'Wolfgang Ulbrich', 'Wu Xiaotian'
];

const LICENSE: string[] = [
  'MATE Calculator is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.',
  'MATE Calculator is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.',
  'You should have received a copy of the GNU General Public License along with MATE Calculator; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.'
];

@CustomDialog
export struct CharacterCodeDialog {
  controller?: CustomDialogController;
  model: CalcModel = new CalcModel();
  @State text: string = '';

  private insert(): void {
    this.model.insertCharacterCode(this.text);
    this.controller?.close();
  }

  build() {
    Column({ space: 14 }) {
      Text(I18n.tr('Insert Character Code'))
        .fontSize(18)
        .fontWeight(FontWeight.Medium)
        .fontColor($r('app.color.display_text'))
        .width('100%')
      Row({ space: 8 }) {
        Text(I18n.tr('Ch_aracter:'))
          .fontSize(14)
          .fontColor($r('app.color.display_text'))
        TextInput({ text: this.text })
          .layoutWeight(1)
          .height(36)
          .defaultFocus(true)
          .onChange((value: string) => {
            this.text = value;
          })
          .onSubmit(() => {
            this.insert();
          })
      }
      .width('100%')
      Row({ space: 12 }) {
        Blank()
        Button(I18n.tr('_Cancel'))
          .fontSize(14)
          .backgroundColor($r('app.color.key_function_bg'))
          .fontColor($r('app.color.key_text'))
          .onClick(() => {
            this.controller?.close();
          })
        Button(I18n.tr('_Insert'))
          .fontSize(14)
          .onClick(() => {
            this.insert();
          })
      }
      .width('100%')
    }
    .width('100%')
    .padding(20)
  }
}

@CustomDialog
export struct AboutDialog {
  controller?: CustomDialogController;
  @State showLicense: boolean = false;

  private comments(): string {
    const versions: LibraryVersions = calc.getLibraryVersions();
    return I18n.format('Calculator with financial and scientific modes.\nUsing GNU MPFR %s and GNU MPC %s',
      [versions.mpfr, versions.mpc]);
  }

  build() {
    Column({ space: 6 }) {
      Image($r('app.media.foreground'))
        .width(44)
        .height(44)
      Text(I18n.tr('MATE Calculator'))
        .fontSize(20)
        .fontWeight(FontWeight.Bold)
        .fontColor($r('app.color.display_text'))
      Text(`${APP_VERSION} (HarmonyOS)`)
        .fontSize(14)
        .fontColor($r('app.color.display_status'))
      Text(this.comments())
        .fontSize(14)
        .textAlign(TextAlign.Center)
        .fontColor($r('app.color.display_text'))
      Text('https://mate-desktop.org')
        .fontSize(13)
        .fontColor($r('app.color.accent'))
      Text(I18n.tr('Copyright © 1986–2010 The GCalctool authors\nCopyright © 2011-2021 MATE developers'))
        .fontSize(12)
        .textAlign(TextAlign.Center)
        .fontColor($r('app.color.display_status'))

      Scroll() {
        Text(this.showLicense ? LICENSE.map((part: string) => I18n.tr(part)).join('\n\n') :
          AUTHORS.join(', '))
          .fontSize(12)
          .fontColor($r('app.color.display_text'))
          .width('100%')
      }
      .height(84)
      .width('100%')
      .padding(8)
      .backgroundColor($r('app.color.display_bg'))
      .borderRadius(6)

      Row({ space: 12 }) {
        Button(this.showLicense ? $r('app.string.about_credits') : $r('app.string.about_license'))
          .fontSize(14)
          .backgroundColor($r('app.color.key_function_bg'))
          .fontColor($r('app.color.key_text'))
          .onClick(() => {
            this.showLicense = !this.showLicense;
          })
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
    .padding(16)
  }
}

class Shortcut {
  keys: string = '';
  action: string = '';

  static of(keys: string, action: string): Shortcut {
    const shortcut: Shortcut = new Shortcut();
    shortcut.keys = keys;
    shortcut.action = action;
    return shortcut;
  }
}

/* Keyboard handling of math-display.c / math-window.c */
const SHORTCUTS: Shortcut[] = [
  Shortcut.of('Enter / =', 'Calculate Result'),
  Shortcut.of('Escape, Ctrl+Backspace, Shift+Delete', 'Clear Display [Escape]'),
  Shortcut.of('*  /  -  +  ^  !  %  |  (  )', 'Operators'),
  Shortcut.of('Ctrl+Z', '_Undo'),
  Shortcut.of('Ctrl+Shift+Z', '_Redo'),
  Shortcut.of('Ctrl+C / Ctrl+V', '_Copy'),
  Shortcut.of('Ctrl+E', 'Scientific exponent [Ctrl+E]'),
  Shortcut.of('Ctrl+F', 'Factorize [Ctrl+F]'),
  Shortcut.of('Ctrl+I', 'Inverse [Ctrl+I]'),
  Shortcut.of('Ctrl+P', 'Pi [Ctrl+P]'),
  Shortcut.of('Ctrl+R', 'Root [Ctrl+R]'),
  Shortcut.of('Ctrl+0…9', 'Superscript mode [Ctrl]'),
  Shortcut.of('Alt+0…9', 'Subscript mode [Alt]'),
  Shortcut.of('Ctrl+[  Ctrl+]', '⌈x⌉'),
  Shortcut.of('Alt+[  Alt+]', '⌊x⌋'),
  Shortcut.of('Ctrl+U  Ctrl+\'  Ctrl+-', 'µ  °  ⁻'),
  Shortcut.of('Alt+↑ / Alt+↓', '_History'),
  Shortcut.of('Ctrl+B / O / D / H', '_Programming'),
  Shortcut.of('Ctrl+Shift+H', '_History'),
  Shortcut.of('Ctrl+Shift+Delete', '_Clear History'),
  Shortcut.of('Ctrl+Q / Ctrl+W', '_Quit'),
  Shortcut.of('F1', '_Contents')
];

@CustomDialog
export struct HelpDialog {
  controller?: CustomDialogController;

  build() {
    Column({ space: 10 }) {
      Text(I18n.tr('MATE Calculator'))
        .fontSize(18)
        .fontWeight(FontWeight.Medium)
        .fontColor($r('app.color.display_text'))
        .width('100%')
      List({ space: 2 }) {
        ForEach(SHORTCUTS, (shortcut: Shortcut) => {
          ListItem() {
            Row({ space: 12 }) {
              Text(shortcut.keys)
                .fontSize(13)
                .fontFamily('monospace')
                .fontColor($r('app.color.accent'))
                .width('45%')
              Text(shortcut.action === 'Operators' ? $r('app.string.help_operators') : I18n.tr(shortcut.action))
                .fontSize(13)
                .fontColor($r('app.color.display_text'))
                .layoutWeight(1)
            }
            .width('100%')
            .padding({ top: 4, bottom: 4 })
          }
        }, (shortcut: Shortcut) => shortcut.keys + shortcut.action)
      }
      .height(260)
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
