/*
 * Access to the main window for PC-style sizing (the upstream window is not
 * resizable and fits the current button mode).
 */
import { window } from '@kit.ArkUI';
import { display } from '@kit.ArkUI';
import { hilog } from '@kit.PerformanceAnalysisKit';
import { ButtonMode } from '../model/ButtonSpecs';

const DOMAIN: number = 0x0C1C;

export class WindowHolder {
  static mainWindow: window.Window | undefined = undefined;

  /* Content size (vp) of each mode, plus the history panel when shown */
  static sizeFor(mode: number, showHistory: boolean): number[] {
    let width: number = 520;
    let height: number = 470;
    if (mode === ButtonMode.ADVANCED) {
      width = 900;
      height = 560;
    } else if (mode === ButtonMode.FINANCIAL) {
      width = 860;
      height = 520;
    } else if (mode === ButtonMode.PROGRAMMING) {
      width = 860;
      height = 680;
    }
    if (showHistory) {
      height += 200;
    }
    return [width, height];
  }

  static fitToMode(mode: number, showHistory: boolean): void {
    const win: window.Window | undefined = WindowHolder.mainWindow;
    if (win === undefined) {
      return;
    }
    try {
      const size: number[] = WindowHolder.sizeFor(mode, showHistory);
      const density: number = display.getDefaultDisplaySync().densityPixels;
      win.resize(Math.round(size[0] * density), Math.round(size[1] * density)).catch(() => {
        // Not supported in this window mode (e.g. full screen)
      });
    } catch (err) {
      hilog.warn(DOMAIN, 'Window', 'Resize failed: %{public}s', JSON.stringify(err));
    }
  }
}
