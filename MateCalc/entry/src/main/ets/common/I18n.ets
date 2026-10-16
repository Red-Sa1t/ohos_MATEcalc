/*
 * Translations for the MATE Calculator port.
 *
 * The upstream gettext catalogs are converted into rawfile/i18n/<lang>.json
 * (tools/po2json.py). Strings throughout the UI and the engine are the
 * original English msgids, so every upstream translation is reused as-is.
 */
import { i18n } from '@kit.LocalizationKit';
import { resourceManager } from '@kit.LocalizationKit';
import { util } from '@kit.ArkTS';
import { hilog } from '@kit.PerformanceAnalysisKit';

const DOMAIN: number = 0x0C1C;
const CONTEXT_SEPARATOR: string = '\u0004';

/* Engine messages built with a %s argument (see calc-engine.c) */
const FORMATTED_MESSAGES: string[] = [
  'Unknown variable \'%s\'',
  'Function \'%s\' is not defined',
  'Malformed expression at token \'%s\''
];

export class I18n {
  private static table: Record<string, string> = {};
  private static language: string = 'C';

  static load(manager: resourceManager.ResourceManager): void {
    let languages: string[] = [];
    try {
      languages = JSON.parse(I18n.readRawText(manager, 'i18n/languages.json')) as string[];
    } catch (err) {
      hilog.warn(DOMAIN, 'I18n', 'No translation index: %{public}s', JSON.stringify(err));
      return;
    }

    const locale: string = I18n.systemLocale();
    const language: string = I18n.pickLanguage(locale, languages);
    if (language.length === 0) {
      return;
    }
    try {
      I18n.table = JSON.parse(I18n.readRawText(manager, `i18n/${language}.json`)) as Record<string, string>;
      I18n.language = language;
    } catch (err) {
      hilog.warn(DOMAIN, 'I18n', 'Failed to load %{public}s: %{public}s', language, JSON.stringify(err));
    }
  }

  /* BCP 47 tag of the system locale, e.g. "zh-Hans-CN" */
  static systemLocale(): string {
    try {
      return i18n.System.getSystemLocaleInstance().toString();
    } catch (err) {
      return 'en-US';
    }
  }

  /* Raw catalog for the native core */
  static catalogKeys(): string[] {
    return Object.keys(I18n.table);
  }

  static catalogValues(keys: string[]): string[] {
    return keys.map((key: string) => I18n.table[key] ?? key);
  }

  static getLanguage(): string {
    return I18n.language;
  }

  /* Translate a msgid and strip GTK mnemonics ("_File", "文件(_F)") */
  static tr(msgid: string): string {
    const translated: string | undefined = I18n.table[msgid];
    return I18n.stripMnemonic(translated !== undefined ? translated : msgid);
  }

  /* Translate a msgid with a gettext context (NC_/C_) */
  static trc(context: string, msgid: string): string {
    const translated: string | undefined = I18n.table[context + CONTEXT_SEPARATOR + msgid];
    return I18n.stripMnemonic(translated !== undefined ? translated : msgid);
  }

  /* Translate a printf style msgid and substitute %s / %d in order */
  static format(msgid: string, args: string[]): string {
    let result: string = I18n.tr(msgid);
    for (let i = 0; i < args.length; i++) {
      result = result.replace(new RegExp('%[sd]'), args[i]);
    }
    return result.replace(new RegExp('%%', 'g'), '%');
  }

  /* Translate an engine status message, including formatted ones */
  static status(message: string): string {
    if (message.length === 0) {
      return '';
    }
    if (I18n.table[message] !== undefined) {
      return I18n.tr(message);
    }
    for (let i = 0; i < FORMATTED_MESSAGES.length; i++) {
      const pattern: string = FORMATTED_MESSAGES[i];
      const index: number = pattern.indexOf('%s');
      const prefix: string = pattern.substring(0, index);
      const suffix: string = pattern.substring(index + 2);
      if (message.startsWith(prefix) && message.endsWith(suffix) && message.length >= prefix.length + suffix.length) {
        const argument: string = message.substring(prefix.length, message.length - suffix.length);
        return I18n.format(pattern, [argument]);
      }
    }
    return message;
  }

  private static stripMnemonic(text: string): string {
    // "文件(_F)" style used by CJK translations, then plain "_File"
    return text.replace(new RegExp('\\(_[^)]\\)', 'g'), '').replace(new RegExp('_(?=[^_\\s])', 'g'), '');
  }

  /* Throws if the raw file is missing; callers handle it */
  private static readRawText(manager: resourceManager.ResourceManager, path: string): string {
    let bytes: Uint8Array = new Uint8Array(0);
    try {
      bytes = manager.getRawFileContentSync(path);
    } catch (err) {
      throw new Error(`Missing raw file ${path}`);
    }
    const decoder: util.TextDecoder = util.TextDecoder.create('utf-8');
    return decoder.decodeToString(bytes);
  }

  /* Map a BCP 47 system locale ("zh-Hans-CN") onto a gettext catalog name */
  private static pickLanguage(locale: string, languages: string[]): string {
    const parts: string[] = locale.split('-');
    const lang: string = parts.length > 0 ? parts[0] : '';
    let script: string = '';
    let region: string = '';
    for (let i = 1; i < parts.length; i++) {
      if (parts[i].length === 4) {
        script = parts[i];
      } else if (parts[i].length === 2 || parts[i].length === 3) {
        region = parts[i].toUpperCase();
      }
    }

    const candidates: string[] = [];
    if (lang === 'zh') {
      if (script === 'Hant' || region === 'TW' || region === 'HK' || region === 'MO') {
        candidates.push(region === 'HK' || region === 'MO' ? 'zh_HK' : 'zh_TW');
        candidates.push('zh_TW');
      } else {
        candidates.push('zh_CN');
      }
    }
    if (region.length > 0) {
      candidates.push(`${lang}_${region}`);
    }
    candidates.push(lang);

    for (let i = 0; i < candidates.length; i++) {
      if (languages.indexOf(candidates[i]) >= 0) {
        return candidates[i];
      }
    }
    return '';
  }
}

/* Short alias used by components */
export function tr(msgid: string): string {
  return I18n.tr(msgid);
}
