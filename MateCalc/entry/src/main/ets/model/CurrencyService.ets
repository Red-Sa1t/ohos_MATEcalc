/*
 * Exchange rate download, replacing the GIO based code of currency-manager.c.
 *
 * Same sources and policy as upstream: the IMF "rms_five" table and the ECB
 * daily reference rates, cached and refreshed when older than a week. The
 * files are parsed by the native core (currency_manager_load_rates).
 */
import { http } from '@kit.NetworkKit';
import { fileIo as fs } from '@kit.CoreFileKit';
import { hilog } from '@kit.PerformanceAnalysisKit';

const DOMAIN: number = 0x0C1C;
const IMF_URL: string = 'https://www.imf.org/external/np/fin/data/rms_five.aspx?tsvflag=Y';
const ECB_URL: string = 'https://www.ecb.europa.eu/stats/eurofxref/eurofxref-daily.xml';
const MAX_AGE_SECONDS: number = 60 * 60 * 24 * 7;

export class RateFiles {
  imf: string = '';
  ecb: string = '';
}

export class CurrencyService {
  private cacheDir: string;
  private downloading: boolean = false;

  constructor(cacheDir: string) {
    this.cacheDir = cacheDir;
  }

  private get imfPath(): string {
    return `${this.cacheDir}/rms_five.xls`;
  }

  private get ecbPath(): string {
    return `${this.cacheDir}/eurofxref-daily.xml`;
  }

  /* Cached rate files, possibly empty */
  readCached(): RateFiles {
    const files: RateFiles = new RateFiles();
    files.imf = CurrencyService.readText(this.imfPath);
    files.ecb = CurrencyService.readText(this.ecbPath);
    return files;
  }

  needsUpdate(): boolean {
    return CurrencyService.isStale(this.imfPath) || CurrencyService.isStale(this.ecbPath);
  }

  /* Download stale files; resolves with the new contents or undefined if nothing changed */
  async update(): Promise<RateFiles | undefined> {
    if (this.downloading || !this.needsUpdate()) {
      return undefined;
    }
    this.downloading = true;
    let changed: boolean = false;
    try {
      if (CurrencyService.isStale(this.imfPath)) {
        hilog.info(DOMAIN, 'Currency', 'Downloading rates from the IMF...');
        changed = (await CurrencyService.download(IMF_URL, this.imfPath)) || changed;
      }
      if (CurrencyService.isStale(this.ecbPath)) {
        hilog.info(DOMAIN, 'Currency', 'Downloading rates from the ECB...');
        changed = (await CurrencyService.download(ECB_URL, this.ecbPath)) || changed;
      }
    } finally {
      this.downloading = false;
    }
    return changed ? this.readCached() : undefined;
  }

  private static isStale(path: string): boolean {
    try {
      if (!fs.accessSync(path)) {
        return true;
      }
      const stat: fs.Stat = fs.statSync(path);
      return Date.now() / 1000 - stat.mtime > MAX_AGE_SECONDS || stat.size === 0;
    } catch (err) {
      return true;
    }
  }

  private static readText(path: string): string {
    try {
      if (fs.accessSync(path)) {
        return fs.readTextSync(path);
      }
    } catch (err) {
      hilog.warn(DOMAIN, 'Currency', 'Failed to read %{public}s', path);
    }
    return '';
  }

  private static async download(url: string, path: string): Promise<boolean> {
    const request: http.HttpRequest = http.createHttp();
    try {
      const response: http.HttpResponse = await request.request(url, {
        method: http.RequestMethod.GET,
        expectDataType: http.HttpDataType.STRING,
        connectTimeout: 30000,
        readTimeout: 30000
      });
      if (response.responseCode !== 200) {
        hilog.warn(DOMAIN, 'Currency', 'Couldn\'t download %{public}s: HTTP %{public}d', url, response.responseCode);
        return false;
      }
      const text: string = response.result as string;
      const file: fs.File = fs.openSync(path, fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE | fs.OpenMode.TRUNC);
      fs.writeSync(file.fd, text);
      fs.closeSync(file);
      return true;
    } catch (err) {
      hilog.warn(DOMAIN, 'Currency', 'Couldn\'t download %{public}s: %{public}s', url, JSON.stringify(err));
      return false;
    } finally {
      request.destroy();
    }
  }
}
