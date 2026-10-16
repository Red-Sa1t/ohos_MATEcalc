/*
 * Button layouts for the four calculator modes, transcribed from mate-calc's
 * buttons-basic.ui, buttons-advanced.ui, buttons-financial.ui and
 * buttons-programming.ui (grid positions), with the inserted text and tooltips
 * from math-buttons.c (button_data[]).
 */

export enum ButtonMode {
  BASIC = 0,
  ADVANCED = 1,
  FINANCIAL = 2,
  PROGRAMMING = 3
}

/* Visual role of a key (math-buttons.c ButtonClass) */
export enum KeyKind {
  NUMBER = 0,
  OPERATOR = 1,
  FUNCTION = 2,
  GROUP = 3,
  ACTION = 4,
  MEMORY = 5,
  RESULT = 6
}

export class ButtonSpec {
  id: string = '';
  label: string = '';
  row: number = 0;
  col: number = 0;
  colSpan: number = 1;
  rowSpan: number = 1;
  kind: KeyKind = KeyKind.FUNCTION;
  italic: boolean = false;
  /* Untranslated tooltip msgid */
  tip: string = '';
  /* Text inserted by plain insert buttons, '' for special actions */
  insert: string = '';
  /* Shows a ▾ drop-down marker */
  dropDown: boolean = false;

  static of(id: string, label: string, row: number, col: number, kind: KeyKind): ButtonSpec {
    const spec: ButtonSpec = new ButtonSpec();
    spec.id = id;
    spec.label = label;
    spec.row = row;
    spec.col = col;
    spec.kind = kind;
    const data: ButtonData | undefined = BUTTON_DATA[id];
    if (data !== undefined) {
      spec.insert = data.insert;
      spec.tip = data.tip;
    }
    return spec;
  }

  span(cols: number): ButtonSpec {
    this.colSpan = cols;
    return this;
  }

  slanted(): ButtonSpec {
    this.italic = true;
    return this;
  }

  menu(): ButtonSpec {
    this.dropDown = true;
    return this;
  }
}

export class PanelSpec {
  rows: number = 0;
  cols: number = 0;
  keys: ButtonSpec[] = [];

  static of(rows: number, cols: number, keys: ButtonSpec[]): PanelSpec {
    const panel: PanelSpec = new PanelSpec();
    panel.rows = rows;
    panel.cols = cols;
    panel.keys = keys;
    return panel;
  }
}

class ButtonData {
  insert: string = '';
  tip: string = '';

  static of(insert: string, tip: string): ButtonData {
    const data: ButtonData = new ButtonData();
    data.insert = insert;
    data.tip = tip;
    return data;
  }
}

/* math-buttons.c button_data[] */
const BUTTON_DATA: Record<string, ButtonData> = {
  'pi': ButtonData.of('π', 'Pi [Ctrl+P]'),
  'eulers_number': ButtonData.of('e', 'Euler’s Number'),
  'imaginary': ButtonData.of('i', 'Imaginary unit'),
  'numeric_point': ButtonData.of('', 'Decimal point'),
  'subscript': ButtonData.of('', 'Subscript mode [Alt]'),
  'superscript': ButtonData.of('', 'Superscript mode [Ctrl]'),
  'exponential': ButtonData.of('', 'Scientific exponent [Ctrl+E]'),
  'add': ButtonData.of('+', 'Add [+]'),
  'subtract': ButtonData.of('', 'Subtract [-]'),
  'multiply': ButtonData.of('×', 'Multiply [*]'),
  'divide': ButtonData.of('÷', 'Divide [/]'),
  'modulus_divide': ButtonData.of(' mod ', 'Modulus divide'),
  'function': ButtonData.of('', 'Additional Functions'),
  'const': ButtonData.of('', 'Additional constants'),
  'x_pow_y': ButtonData.of('^', 'Exponent [^ or **]'),
  'x_squared': ButtonData.of('²', 'Square [Ctrl+2]'),
  'percentage': ButtonData.of('%', 'Percentage [%]'),
  'factorial': ButtonData.of('!', 'Factorial [!]'),
  'abs': ButtonData.of('|', 'Absolute value [|]'),
  'arg': ButtonData.of('Arg ', 'Complex argument'),
  'conjugate': ButtonData.of('conj ', 'Complex conjugate'),
  'root': ButtonData.of('√', 'Root [Ctrl+R]'),
  'square_root': ButtonData.of('√', 'Square root [Ctrl+R]'),
  'logarithm': ButtonData.of('log ', 'Logarithm'),
  'natural_logarithm': ButtonData.of('ln ', 'Natural Logarithm'),
  'sine': ButtonData.of('sin ', 'Sine'),
  'cosine': ButtonData.of('cos ', 'Cosine'),
  'tangent': ButtonData.of('tan ', 'Tangent'),
  'hyperbolic_sine': ButtonData.of('sinh ', 'Hyperbolic Sine'),
  'hyperbolic_cosine': ButtonData.of('cosh ', 'Hyperbolic Cosine'),
  'hyperbolic_tangent': ButtonData.of('tanh ', 'Hyperbolic Tangent'),
  'inverse_sine': ButtonData.of('asin ', 'Inverse Sine'),
  'inverse_cosine': ButtonData.of('acos ', 'Inverse Cosine'),
  'inverse_tangent': ButtonData.of('atan ', 'Inverse Tangent'),
  'inverse': ButtonData.of('⁻¹', 'Inverse [Ctrl+I]'),
  'and': ButtonData.of('∧', 'Boolean AND'),
  'or': ButtonData.of('∨', 'Boolean OR'),
  'xor': ButtonData.of('⊻', 'Boolean Exclusive OR'),
  'not': ButtonData.of('¬', 'Boolean NOT'),
  'integer_portion': ButtonData.of('int ', 'Integer Component'),
  'fractional_portion': ButtonData.of('frac ', 'Fractional Component'),
  'real_portion': ButtonData.of('Re ', 'Real Component'),
  'imaginary_portion': ButtonData.of('Im ', 'Imaginary Component'),
  'ones_complement': ButtonData.of('ones ', 'Ones\' Complement'),
  'twos_complement': ButtonData.of('twos ', 'Two\'s Complement'),
  'start_group': ButtonData.of('(', 'Start Group [(]'),
  'end_group': ButtonData.of(')', 'End Group [)]'),
  'memory': ButtonData.of('', 'Memory'),
  'character': ButtonData.of('', 'Insert Character Code'),
  'result': ButtonData.of('', 'Calculate Result'),
  'factor': ButtonData.of('', 'Factorize [Ctrl+F]'),
  'clear': ButtonData.of('', 'Clear Display [Escape]'),
  'undo': ButtonData.of('', 'Undo [Ctrl+Z]'),
  'shift_left': ButtonData.of('', 'Shift Left [<<]'),
  'shift_right': ButtonData.of('', 'Shift Right [>>]'),
  'finc_compounding_term': ButtonData.of('', 'Compounding Term'),
  'finc_double_declining_depreciation': ButtonData.of('', 'Double Declining Depreciation'),
  'finc_future_value': ButtonData.of('', 'Future Value'),
  'finc_term': ButtonData.of('', 'Financial Term'),
  'finc_sum_of_the_years_digits_depreciation': ButtonData.of('', 'Sum of the Years Digits Depreciation'),
  'finc_straight_line_depreciation': ButtonData.of('', 'Straight Line Depreciation'),
  'finc_periodic_interest_rate': ButtonData.of('', 'Periodic Interest Rate'),
  'finc_present_value': ButtonData.of('', 'Present Value'),
  'finc_periodic_payment': ButtonData.of('', 'Periodic Payment'),
  'finc_gross_profit_margin': ButtonData.of('', 'Gross Profit Margin')
};

function digit(value: number, row: number, col: number): ButtonSpec {
  const labels: string = '0123456789ABCDEF';
  const spec: ButtonSpec = ButtonSpec.of(`digit_${value}`, labels.charAt(value), row, col, KeyKind.NUMBER);
  spec.tip = '';
  return spec;
}

function key(id: string, label: string, row: number, col: number, kind: KeyKind): ButtonSpec {
  return ButtonSpec.of(id, label, row, col, kind);
}

/* buttons-basic.ui: 4 rows x 6 columns */
export const BASIC_PANEL: PanelSpec = PanelSpec.of(4, 6, [
  digit(7, 0, 0), digit(8, 0, 1), digit(9, 0, 2), key('divide', '÷', 0, 3, KeyKind.OPERATOR),
  key('undo', 'Undo', 0, 4, KeyKind.GROUP), key('clear', 'Clear', 0, 5, KeyKind.GROUP),
  digit(4, 1, 0), digit(5, 1, 1), digit(6, 1, 2), key('multiply', '×', 1, 3, KeyKind.OPERATOR),
  key('start_group', '(', 1, 4, KeyKind.GROUP), key('end_group', ')', 1, 5, KeyKind.GROUP),
  digit(1, 2, 0), digit(2, 2, 1), digit(3, 2, 2), key('subtract', '−', 2, 3, KeyKind.OPERATOR),
  key('x_squared', 'x²', 2, 4, KeyKind.FUNCTION).slanted(), key('square_root', '√', 2, 5, KeyKind.FUNCTION),
  digit(0, 3, 0), key('numeric_point', '.', 3, 1, KeyKind.NUMBER), key('percentage', '%', 3, 2, KeyKind.NUMBER),
  key('add', '+', 3, 3, KeyKind.OPERATOR), key('result', '=', 3, 4, KeyKind.RESULT).span(2)
]);

/* buttons-advanced.ui: 5 rows x 11 columns (trigonometric block in columns 6-9) */
export const ADVANCED_PANEL: PanelSpec = PanelSpec.of(5, 11, [
  key('subscript', '↓n', 0, 0, KeyKind.NUMBER), key('superscript', '↑n', 0, 1, KeyKind.NUMBER),
  key('exponential', '×10ʸ', 0, 2, KeyKind.NUMBER), key('modulus_divide', 'mod', 0, 3, KeyKind.OPERATOR),
  key('undo', 'Undo', 0, 4, KeyKind.GROUP), key('clear', 'Clear', 0, 5, KeyKind.GROUP),
  digit(7, 1, 0), digit(8, 1, 1), digit(9, 1, 2), key('divide', '÷', 1, 3, KeyKind.OPERATOR),
  key('start_group', '(', 1, 4, KeyKind.GROUP), key('end_group', ')', 1, 5, KeyKind.GROUP),
  digit(4, 2, 0), digit(5, 2, 1), digit(6, 2, 2), key('multiply', '×', 2, 3, KeyKind.OPERATOR),
  key('memory', 'x', 2, 4, KeyKind.MEMORY).span(2).menu(),
  digit(1, 3, 0), digit(2, 3, 1), digit(3, 3, 2), key('subtract', '−', 3, 3, KeyKind.OPERATOR),
  key('pi', 'π', 3, 4, KeyKind.NUMBER), key('eulers_number', 'e', 3, 5, KeyKind.NUMBER).slanted(),
  key('x_pow_y', 'xʸ', 3, 6, KeyKind.FUNCTION).slanted(), key('root', '√', 3, 7, KeyKind.FUNCTION),
  key('logarithm', 'log', 3, 8, KeyKind.FUNCTION), key('natural_logarithm', 'ln', 3, 9, KeyKind.FUNCTION),
  digit(0, 4, 0), key('numeric_point', '.', 4, 1, KeyKind.NUMBER), key('imaginary', 'i', 4, 2, KeyKind.NUMBER).slanted(),
  key('add', '+', 4, 3, KeyKind.OPERATOR), key('result', '=', 4, 4, KeyKind.RESULT),
  key('factor', 'a×b', 4, 5, KeyKind.ACTION), key('real_portion', 'Re', 4, 6, KeyKind.FUNCTION),
  key('imaginary_portion', 'Im', 4, 7, KeyKind.FUNCTION), key('conjugate', 'conj', 4, 8, KeyKind.FUNCTION),
  key('arg', 'Arg', 4, 9, KeyKind.FUNCTION),
  key('function', 'f(x)', 0, 10, KeyKind.FUNCTION).menu(), key('inverse', 'x⁻¹', 1, 10, KeyKind.FUNCTION).slanted(),
  key('factorial', 'x!', 2, 10, KeyKind.FUNCTION).slanted(), key('abs', '|x|', 3, 10, KeyKind.FUNCTION).slanted(),
  key('const', 'const', 4, 10, KeyKind.FUNCTION).menu()
]);

/* Trigonometric sub-grid of buttons-advanced.ui (table1, 3 x 3, at column 6 row 0 spanning 4 x 3) */
export const ADVANCED_TRIG_PANEL: PanelSpec = PanelSpec.of(3, 3, [
  key('cosine', 'cos', 0, 0, KeyKind.FUNCTION), key('sine', 'sin', 0, 1, KeyKind.FUNCTION),
  key('tangent', 'tan', 0, 2, KeyKind.FUNCTION),
  key('hyperbolic_cosine', 'cosh', 1, 0, KeyKind.FUNCTION), key('hyperbolic_sine', 'sinh', 1, 1, KeyKind.FUNCTION),
  key('hyperbolic_tangent', 'tanh', 1, 2, KeyKind.FUNCTION),
  key('inverse_cosine', 'acos', 2, 0, KeyKind.FUNCTION), key('inverse_sine', 'asin', 2, 1, KeyKind.FUNCTION),
  key('inverse_tangent', 'atan', 2, 2, KeyKind.FUNCTION)
]);

/* buttons-financial.ui: 4 rows x 10 columns */
export const FINANCIAL_PANEL: PanelSpec = PanelSpec.of(4, 10, [
  digit(7, 0, 0), digit(8, 0, 1), digit(9, 0, 2), key('divide', '÷', 0, 3, KeyKind.OPERATOR),
  key('undo', 'Undo', 0, 4, KeyKind.GROUP), key('clear', 'Clear', 0, 5, KeyKind.GROUP),
  key('x_pow_y', 'xʸ', 0, 6, KeyKind.FUNCTION).slanted(),
  key('finc_compounding_term', 'Ctrm', 0, 7, KeyKind.FUNCTION),
  key('finc_double_declining_depreciation', 'Ddb', 0, 8, KeyKind.FUNCTION),
  key('finc_future_value', 'Fv', 0, 9, KeyKind.FUNCTION),
  digit(4, 1, 0), digit(5, 1, 1), digit(6, 1, 2), key('multiply', '×', 1, 3, KeyKind.OPERATOR),
  key('start_group', '(', 1, 4, KeyKind.GROUP), key('end_group', ')', 1, 5, KeyKind.GROUP),
  key('root', '√', 1, 6, KeyKind.FUNCTION),
  key('finc_gross_profit_margin', 'Gpm', 1, 7, KeyKind.FUNCTION),
  key('finc_periodic_payment', 'Pmt', 1, 8, KeyKind.FUNCTION),
  key('finc_present_value', 'Pv', 1, 9, KeyKind.FUNCTION),
  digit(1, 2, 0), digit(2, 2, 1), digit(3, 2, 2), key('subtract', '−', 2, 3, KeyKind.OPERATOR),
  key('memory', 'x', 2, 4, KeyKind.MEMORY).span(2).menu(),
  key('logarithm', 'log', 2, 6, KeyKind.FUNCTION),
  key('finc_periodic_interest_rate', 'Rate', 2, 7, KeyKind.FUNCTION),
  key('finc_straight_line_depreciation', 'Sln', 2, 8, KeyKind.FUNCTION),
  key('finc_sum_of_the_years_digits_depreciation', 'Syd', 2, 9, KeyKind.FUNCTION),
  digit(0, 3, 0), key('numeric_point', '.', 3, 1, KeyKind.NUMBER), key('percentage', '%', 3, 2, KeyKind.NUMBER),
  key('add', '+', 3, 3, KeyKind.OPERATOR), key('result', '=', 3, 4, KeyKind.RESULT).span(2),
  key('finc_term', 'Term', 3, 8, KeyKind.FUNCTION)
]);

/* buttons-programming.ui advanced_panel: 5 rows x 10 columns */
export const PROGRAMMING_PANEL: PanelSpec = PanelSpec.of(5, 10, [
  key('subscript', '↓n', 0, 0, KeyKind.NUMBER), key('superscript', '↑n', 0, 1, KeyKind.NUMBER),
  key('numeric_point', '.', 0, 2, KeyKind.NUMBER), key('memory', 'x', 0, 3, KeyKind.MEMORY).menu(),
  key('start_group', '(', 0, 4, KeyKind.GROUP), key('end_group', ')', 0, 5, KeyKind.GROUP),
  key('shift_left', '<', 0, 6, KeyKind.ACTION).menu(), key('shift_right', '>', 0, 7, KeyKind.ACTION).menu(),
  digit(12, 1, 0), digit(13, 1, 1), digit(14, 1, 2), digit(15, 1, 3),
  key('divide', '÷', 1, 4, KeyKind.OPERATOR), key('modulus_divide', 'mod', 1, 5, KeyKind.OPERATOR),
  key('ones_complement', 'ones', 1, 6, KeyKind.FUNCTION), key('twos_complement', 'twos', 1, 7, KeyKind.FUNCTION),
  key('character', 'á', 1, 8, KeyKind.MEMORY), key('abs', '|x|', 1, 9, KeyKind.FUNCTION).slanted(),
  digit(8, 2, 0), digit(9, 2, 1), digit(10, 2, 2), digit(11, 2, 3),
  key('multiply', '×', 2, 4, KeyKind.OPERATOR), key('and', 'AND', 2, 5, KeyKind.OPERATOR),
  key('not', 'NOT', 2, 6, KeyKind.FUNCTION), key('root', '√', 2, 7, KeyKind.FUNCTION),
  key('x_pow_y', 'xʸ', 2, 8, KeyKind.FUNCTION).slanted(), key('inverse', 'x⁻¹', 2, 9, KeyKind.FUNCTION).slanted(),
  digit(4, 3, 0), digit(5, 3, 1), digit(6, 3, 2), digit(7, 3, 3),
  key('subtract', '−', 3, 4, KeyKind.OPERATOR), key('or', 'OR', 3, 5, KeyKind.OPERATOR),
  key('clear', 'Clear', 3, 6, KeyKind.GROUP), key('logarithm', 'log', 3, 7, KeyKind.FUNCTION),
  key('natural_logarithm', 'ln', 3, 8, KeyKind.FUNCTION), key('integer_portion', 'int', 3, 9, KeyKind.FUNCTION),
  digit(0, 4, 0), digit(1, 4, 1), digit(2, 4, 2), digit(3, 4, 3),
  key('add', '+', 4, 4, KeyKind.OPERATOR), key('xor', 'XOR', 4, 5, KeyKind.OPERATOR),
  key('result', '=', 4, 6, KeyKind.RESULT), key('factor', 'fact', 4, 7, KeyKind.ACTION),
  key('factorial', 'x!', 4, 8, KeyKind.FUNCTION).slanted(), key('fractional_portion', 'frac', 4, 9, KeyKind.FUNCTION)
]);

/* f(x) menu (math-buttons.c function_cb) */
export class MenuEntry {
  name: string = '';
  insert: string = '';
  detail: string = '';

  static of(name: string, insert: string, detail: string): MenuEntry {
    const entry: MenuEntry = new MenuEntry();
    entry.name = name;
    entry.insert = insert;
    entry.detail = detail;
    return entry;
  }
}

export const FUNCTION_MENU: MenuEntry[] = [
  MenuEntry.of('Integer Component', 'int ', ''),
  MenuEntry.of('Fractional Component', 'frac ', ''),
  MenuEntry.of('Round', 'round ', ''),
  MenuEntry.of('Floor', 'floor ', ''),
  MenuEntry.of('Ceiling', 'ceil ', ''),
  MenuEntry.of('Sign', 'sgn ', '')
];

/* const menu (math-buttons.c const_cb) */
export const CONSTANT_MENU: MenuEntry[] = [
  MenuEntry.of('Velocity of Light', 'c₀', '299,792,458 m/s'),
  MenuEntry.of('Magnetic constant', 'μ₀', '1.2566370614×10⁻⁶ N/A²'),
  MenuEntry.of('Electric constant', 'ε₀', '8.85418782×10⁻¹² s⁴A²/m³kg'),
  MenuEntry.of('Newtonian constant of gravitation', 'G', '6.67408×10⁻¹¹ m³/(s²kg)'),
  MenuEntry.of('Planck constant', 'h', '6.62607004×10⁻³⁴ m²kg/s'),
  MenuEntry.of('Elementary charge', 'ｅ', '1.6021766208(98)×10⁻¹⁹ C'),
  MenuEntry.of('Electron mass', 'mₑ', '9.10938356×10⁻³¹ kg'),
  MenuEntry.of('Proton mass', 'mₚ', '1.672621898(21)×10⁻²⁷ kg'),
  MenuEntry.of('Avogadro constant', 'Nₐ', '6.02214086×10²³ mol⁻¹')
];

/* Financial dialogs (buttons-financial.ui, field order of finc_dialog_fields[]) */
export class FinancialDialogSpec {
  func: number = 0;
  title: string = '';
  description: string = '';
  fields: string[] = [];

  static of(func: number, title: string, description: string, fields: string[]): FinancialDialogSpec {
    const spec: FinancialDialogSpec = new FinancialDialogSpec();
    spec.func = func;
    spec.title = title;
    spec.description = description;
    spec.fields = fields;
    return spec;
  }
}

export const FINANCIAL_DIALOGS: Record<string, FinancialDialogSpec> = {
  'finc_compounding_term': FinancialDialogSpec.of(0, 'Compounding Term',
    'Calculates the number of compounding periods necessary to increase an investment of present value to a future value, at a fixed interest rate per compounding period.',
    ['Periodic Interest _Rate:', '_Future Value:', 'Present _Value:']),
  'finc_double_declining_depreciation': FinancialDialogSpec.of(1, 'Double-Declining Depreciation',
    'Calculates the depreciation allowance on an asset for a specified period of time, using the double-declining balance method.',
    ['C_ost:', '_Life:', '_Period:']),
  'finc_future_value': FinancialDialogSpec.of(2, 'Future Value',
    'Calculates the future value of an investment based on a series of equal payments at a periodic interest rate over the number of payment periods in the term.',
    ['_Periodic Payment:', 'Periodic Interest _Rate:', '_Number of Periods:']),
  'finc_gross_profit_margin': FinancialDialogSpec.of(3, 'Gross Profit Margin',
    'Calculates the resale price of a product, based on the product cost and the wanted gross profit margin.',
    ['C_ost:', '_Margin:']),
  'finc_periodic_payment': FinancialDialogSpec.of(4, 'Periodic Payment',
    'Calculates the amount of the periodic payment of a loan, where payments are made at the end of each payment period. ',
    ['_Principal:', 'Periodic Interest _Rate:', '_Term:']),
  'finc_present_value': FinancialDialogSpec.of(5, 'Present Value',
    'Calculates the present value of an investment based on a series of equal payments discounted at a periodic interest rate over the number of payment periods in the term. ',
    ['_Periodic Payment:', 'Periodic Interest _Rate:', '_Number of Periods:']),
  'finc_periodic_interest_rate': FinancialDialogSpec.of(6, 'Periodic Interest Rate',
    'Calculates the periodic interest necessary to increase an investment to a future value, over the number of compounding periods. ',
    ['_Future Value:', 'Present _Value:', '_Term:']),
  'finc_straight_line_depreciation': FinancialDialogSpec.of(7, 'Straight-Line Depreciation',
    'Calculates the straight-line depreciation of an asset for one period. The straight-line method of depreciation divides the depreciable cost evenly over the useful life of an asset. The useful life is the number of periods, typically years, over which an asset is depreciated. ',
    ['_Cost:', '_Salvage:', '_Life:']),
  'finc_sum_of_the_years_digits_depreciation': FinancialDialogSpec.of(8, 'Sum-of-the-Years\'-Digits Depreciation',
    'Calculates the depreciation allowance on an asset for a specified period of time, using the Sum-of-the-Years\'-Digits method. This method of depreciation accelerates the rate of depreciation, so that more depreciation expense occurs in earlier periods than in later ones. The useful life is the number of periods, typically years, over which an asset is depreciated. ',
    ['_Cost:', '_Salvage:', '_Life:', '_Period:']),
  'finc_term': FinancialDialogSpec.of(9, 'Payment Period',
    'Calculates the number of payment periods that are necessary during the term of an ordinary annuity, to accumulate a future value, at a periodic interest rate.',
    ['_Periodic Payment:', 'Future _Value:', 'Periodic Interest _Rate:'])
};

export function panelForMode(mode: number): PanelSpec {
  if (mode === ButtonMode.ADVANCED) {
    return ADVANCED_PANEL;
  } else if (mode === ButtonMode.FINANCIAL) {
    return FINANCIAL_PANEL;
  } else if (mode === ButtonMode.PROGRAMMING) {
    return PROGRAMMING_PANEL;
  }
  return BASIC_PANEL;
}
