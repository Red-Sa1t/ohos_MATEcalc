/*
 * ArkTS declarations for the MATE Calculator native engine (libmatecalc.so).
 * Text offsets are ArkTS string indices (UTF-16 code units).
 */

export interface EngineState {
  display: string;
  selectionStart: number;
  selectionEnd: number;
  /** -1 when the display holds no answer span */
  ansStart: number;
  ansEnd: number;
  /** Untranslated status message (mate-calc msgid) or empty */
  status: string;
  /** 0 normal, 1 superscript, 2 subscript */
  numberMode: number;
  isResult: boolean;
  inSolve: boolean;
  revision: number;
}

export interface EngineSettings {
  accuracy?: number;
  showThousandsSeparators?: boolean;
  showTrailingZeroes?: boolean;
  /** 0 automatic, 1 fixed, 2 scientific, 3 engineering */
  numberFormat?: number;
  base?: number;
  wordSize?: number;
  /** 0 radians, 1 degrees, 2 gradians */
  angleUnits?: number;
  radix?: string;
  thousandsSeparator?: string;
}

export interface BitsInfo {
  enabled: boolean;
  /** 64 characters, index 0 is the most significant bit */
  bits: string;
  label: string;
}

export interface VariableInfo {
  name: string;
  value: string;
}

export interface HistoryInfo {
  equation: string;
  answer: string;
}

export interface UnitInfo {
  name: string;
  displayName: string;
}

export interface UnitCategoryInfo {
  name: string;
  displayName: string;
  units: UnitInfo[];
}

export interface LibraryVersions {
  mpfr: string;
  mpc: string;
}

export interface ConversionInfo {
  ok: boolean;
  source: string;
  target: string;
}

export const init: (variablesFile: string) => EngineState;
export const getState: () => EngineState;

export const insert: (text: string) => EngineState;
export const insertDigit: (digit: number) => EngineState;
export const insertNumericPoint: () => EngineState;
export const insertExponent: () => EngineState;
export const insertSubtract: () => EngineState;
export const insertCharacterCode: (text: string) => EngineState;
export const replace: (start: number, end: number, text: string) => EngineState;
export const setSelection: (start: number, end: number) => EngineState;
export const backspace: () => EngineState;
export const deleteChar: () => EngineState;
export const clear: () => EngineState;
export const undo: () => EngineState;
export const redo: () => EngineState;
export const copy: () => string;
export const setNumberMode: (mode: number) => EngineState;
export const setStatus: (status: string) => EngineState;

export const solve: () => Promise<EngineState>;
export const factorize: () => Promise<EngineState>;

export const setSettings: (settings: EngineSettings) => EngineState;

export const shift: (count: number) => EngineState;
export const toggleBit: (bit: number) => EngineState;
export const getBits: () => BitsInfo;

export const store: (name: string) => EngineState;
export const recall: (name: string) => EngineState;
export const getVariables: () => VariableInfo[];
export const deleteVariable: (name: string) => void;
export const getAnswer: () => string;

export const getHistory: () => HistoryInfo[];
export const historyInsertAnswer: (index: number) => EngineState;
export const historySetEquation: (index: number) => EngineState;
export const clearHistory: () => void;

export const getUnitCategories: () => UnitCategoryInfo[];
export const convert: (category: string, sourceUnit: string, targetUnit: string) => ConversionInfo;
export const applyConversion: (category: string, sourceUnit: string, targetUnit: string) => EngineState;
export const loadCurrencyRates: (imfData: string, ecbData: string) => EngineState;
export const hasCurrencyRates: () => boolean;

/** function: 0 Ctrm, 1 Ddb, 2 Fv, 3 Gpm, 4 Pmt, 5 Pv, 6 Rate, 7 Sln, 8 Syd, 9 Term */
export const financial: (func: number, args: string[]) => EngineState;

export const getLibraryVersions: () => LibraryVersions;

/** Install translations (msgid or "contextmsgid" -> translation) for the C core */
export const setTranslations: (keys: string[], values: string[]) => void;
