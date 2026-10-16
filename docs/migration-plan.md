# MATE 计算器 → 鸿蒙 PC 迁移：调研与任务规划

调研日期：2026-10-16

## 1. 调研结论

### 1.1 上游项目（mate-calc 1.28.0）

- 源码：`upstream/mate-calc`（GitHub mate-desktop/mate-calc，commit `2b63722`），GPLv2+/GPLv3+
- 语言 / 依赖：C + GTK3 + GLib/GObject/GIO + libxml2 + **MPFR/MPC（GMP）**，构建系统 meson/autotools
- 总代码量约 2.4 万行，按职责分为三层：

| 层 | 文件 | 行数 | GLib/GTK 耦合 | 迁移方式 |
|---|---|---|---|---|
| 数学内核 | `mp.c` `mp-binary.c` `mp-convert.c` `mp-trigonometric.c` `financial.c` | ~2.2k | 低（g_free/g_strdup 等） | **保留 C**，用 NDK 编译 |
| 表达式解析 | `lexer.c` `prelexer.c` `parser.c` `parserfunc.c` `mp-equation.c` | ~3.4k | 低（GString、utf8 函数） | **保留 C** |
| 格式化 / 单位 / 变量 | `mp-serializer.c` `unit*.c` `currency*.c` `math-variables.c` | ~2.2k | 中（GObject 属性、GList、GHashTable） | 保留 C，GObject 改为普通 struct |
| 汇率下载 | `currency-manager.c` 中 GIO + libxml2 部分 | ~300 | 高 | **改用 ArkTS**（`@kit.NetworkKit` 下载 ECB/IMF 数据，再交给 C 层） |
| UI 与状态 | `math-equation.c` `math-buttons.c` `math-window.c` `math-display.c` `math-preferences.c` `math-converter.c` `math-history*.c` `math-variable-popup.c` 和 4 个 `.ui` 文件 | ~6.5k + 6.6k UI XML | 极高（GTK） | **全部用 ArkUI 重写** |
| 测试 | `test-mp.c` `test-mp-equation.c` | ~1k | 低 | 移植为 native 单元测试，作为功能一致性的基准 |

功能清单（需全部对齐）：
- 4 种模式：基础（23 键）、高级（49 键）、财务（55 键）、程序员（48 键，含进制/位编辑/字长）
- 显示区：表达式编辑、上标/下标输入、状态提示、复数
- 历史记录面板、变量弹窗（存取 `x=` 变量）、单位 / 货币换算器
- 偏好设置：精度、数字格式（自动/定点/科学/工程）、角度单位、字长、千位分隔符、尾零、刷新汇率
- 键盘操作：约 116 处快捷键 / 按键映射，PC 场景下很重要
- 国际化：137 种语言的 `.po`

### 1.2 本机鸿蒙开发环境

| 项目 | 状态 |
|---|---|
| DevEco Studio | 6.1.1.300，位于 `D:\Tools\DevEco Studio` |
| SDK | HarmonyOS 6.1.1，API 24 |
| NDK | 自带 clang/lld/cmake/ninja；sysroot 提供 `aarch64-linux-ohos`（真机 PC）和 `x86_64-linux-ohos`（模拟器） |
| devecocli | 1.3.0，可用 |
| Node | v20.18.0 |
| 交叉编译辅助 | WSL Ubuntu、MSYS2 make/m4、Docker 均可用 |
| `DEVECO_HOME` | **未设置**，需要设为 `D:\Tools\DevEco Studio`（skills 脚本依赖它） |
| 设备 | 无已连接设备；现有模拟器只有 `Pura 90`（phone）；**2in1（PC）镜像未下载** |
| 签名 | 未生成，需 `devecocli auth login` 后执行 `signature generate` |

### 1.3 关键技术决策

**方案：C 内核 + N-API 桥接 + ArkUI 重写界面**（推荐）

- 原因：mate-calc 的核心价值在于 1000 位精度的 MPFR/MPC 复数运算和一套完整的表达式文法。用 ArkTS 重写这部分，精度和行为都很难对齐。内核代码与 GTK 无关，GLib 用得也少，可以原样保留。
- GMP / MPFR / MPC：用 OHOS NDK 交叉编译成静态库（arm64-v8a + x86_64）。可参考 OpenHarmony 三方库 `tpc_c_cplusplus`（lycium）中现成的 gmp/mpfr/mpc 构建脚本，在 WSL 或 MSYS2 中执行。
- GLib：不移植整套 glib，改写一个约 300 行的 `glib-compat` 小垫片（GString、GList、strdup/printf、utf8 函数、gettext 宏）。GObject 类型改写成普通 C 结构体。
- 状态层：`math-equation.c` 同时承担状态机、撤销/重做、光标插入和 GTK TextBuffer，在 ArkTS 中重写为 `CalcEquation` 状态模型，通过 N-API 调用 C 的 `mp_equation_parse` / `mp_serializer`。
- 许可证：保持 GPL，在应用的"关于"页注明上游来源。

备选方案（不推荐）：纯 ArkTS 重写 + 第三方大数库。实现更简单，但会失去任意精度复数运算，财务/程序员模式的结果也会和原版不一致。

## 2. 目标工程结构

```
mate-migration/
├── upstream/mate-calc/          # 上游源码（只读参考）
├── third_party/                 # gmp/mpfr/mpc 构建脚本与产物 (arm64-v8a, x86_64)
├── MateCalc/                    # 鸿蒙工程（deveco-create-project 生成）
│   └── entry/src/main/
│       ├── cpp/
│       │   ├── core/            # 从上游拷贝并做最小修改的 C 内核
│       │   ├── compat/          # glib-compat 垫片
│       │   ├── napi_calc.cpp    # N-API 导出：evaluate/format/convert/financial/bitops
│       │   ├── tests/           # test-mp / test-mp-equation 移植
│       │   └── CMakeLists.txt
│       ├── ets/
│       │   ├── model/           # CalcEquation、Preferences、History、Variables、Currency
│       │   ├── components/      # Display、ButtonPanel×4、HistoryPanel、Converter、VariablePopup
│       │   ├── pages/Index.ets  # 主窗口（菜单栏 + 显示区 + 按键区）
│       │   └── pages/PreferencesDialog / About
│       └── resources/           # 字符串资源（zh_CN/en_US，可由 .po 脚本转换）、图标
└── docs/
```

## 3. 任务规划

| 阶段 | 任务 | 产出 / 验收标准 | 依赖 |
|---|---|---|---|
| **P0 环境准备** | 设置 `DEVECO_HOME`；**不下载模拟器镜像，改用鸿蒙 PC 真机**；真机接入后登录并生成调试签名 | `devecocli device list` 能看到真机 | 真机目前暂时用不了：P0 的设备部分和所有"在设备上验证"的步骤（P4 设备调用、P9 截图、P10 UI 自动化）推迟到真机接入后再做；在此之前只做构建通过、WSL 下的 native 测试和代码层面的验证 |
| **P1 工程骨架** | 用 deveco-create-project 创建 `MateCalc`（`com.example.matecalc`，API 24）；配置 2in1 deviceType、窗口尺寸和 native 模块 | 空工程能构建 | P0（构建本身不需要设备） |
| **P2 三方数学库** | 交叉编译 GMP 6.3 / MPFR 4.2 / MPC 1.3，目标 arm64-v8a + x86_64 | 每个 ABI 得到 `.a` 和头文件，能被 CMake 链接 | — |
| **P3 C 内核移植** | 拷贝内核文件；编写 glib-compat；把 mp-serializer/unit/currency/variables 的 GObject 改成普通 struct；去掉 GIO/libxml2 | 在 WSL 中用 gcc 编译通过，`test-mp` 和 `test-mp-equation` 全部通过（与上游对照） | P2 |
| **P4 N-API 桥接** | 导出 `evaluate(expr, opts)`、`format(num, fmt)`、`convertUnit`、`convertCurrency`、`setCurrencyRates`、financial 系列、`bitToggle/shift/ones/twos`、`setVariable`；在 ArkTS 侧补充 `.d.ts` | 用 NDK 构建出 `libmatecalc.so`，在设备上通过 ArkTS 调用 `1+2×3` 和 `sin(30°)` 结果正确 | P1、P3 |
| **P5 状态模型** | ArkTS 版 `CalcEquation`：插入/删除/光标、上标/下标模式、ans、撤销/重做、错误状态、程序员模式进制显示 | 用 ArkTS 单元测试（hypium）覆盖主要操作 | P4 |
| **P6 UI：主窗口 + 基础/高级模式** | 菜单栏（计算器/模式/帮助）、显示区、按键面板、PC 键盘映射（对应上游约 116 个快捷键） | 两种模式下所有按键可用，键盘输入与原版一致 | P5 |
| **P7 UI：财务 + 程序员模式** | 财务对话框（Ctrm/Ddb/Fv/Gpm/Pmt/Pv/Rate/Sln/Syd/Term）；程序员模式的进制切换、位编辑面板、字长 | 与上游对照测试结果一致 | P6 |
| **P8 辅助功能** | 历史面板、变量弹窗、单位/货币换算器（ArkTS 下载 ECB/IMF 汇率并缓存到沙箱）、偏好设置（Preferences 持久化）、关于页 | 每个功能逐项验收 | P6 |
| **P9 PC 适配与本地化** | 窗口最小/默认尺寸、自由窗口缩放、深色模式、鼠标悬停态、右键复制粘贴；`.po` → `string.json` 转换脚本（首批 zh_CN、en_US） | 在 2in1 模拟器上截图验收 | P6–P8 |
| **P10 测试与交付** | 运行 native 测试和 ArkTS 测试；用 devecocli ui 自动点击主要流程；运行时崩溃排查（arkts-runtime-fix）；撰写迁移报告 | HAP 可安装运行，并附验收清单 | 全部 |

可以并行的部分：P2/P3（C 层）与 P1/P5 的 UI 原型互不依赖。

## 4. 风险与应对

| 风险 | 应对 |
|---|---|
| GMP 在 OHOS 上交叉编译失败（汇编优化、configure 探测出错） | 使用 `--disable-assembly`；必要时改用 lycium 脚本或 mini-gmp |
| 真机暂时不可用，无法在设备上验证 | 先完成构建和 native 测试（WSL 中 gcc 对照上游），设备验证集中放到真机接入之后；真机以 arm64-v8a 为主 ABI |
| 汇率接口需要网络权限 / 访问失败 | 申请 `ohos.permission.INTERNET`；下载失败时回退到内置的离线汇率 |
| GPL 许可证与应用分发 | 保持 GPL，在应用内附带许可证和源码地址 |
| ArkTS 严格语法（不支持 any、解构等） | 严格遵循 arkts-grammar-standards，编译错误按 arkts-error-fixes 处理 |

## 5. 进展记录

### 2026-10-16：P1–P4 完成（均为构建和主机测试层面，尚未在设备上验证）

| 阶段 | 结果 |
|---|---|
| P1 工程骨架 | `MateCalc/`（`com.example.matecalc`，API 24，deviceTypes=`2in1`），空工程构建通过 |
| P2 三方数学库 | `third_party/build-mp.sh`：在 MSYS2 下用 NDK clang 交叉编译 GMP 6.3.0 / MPFR 4.2.1 / MPC 1.3.1（`--disable-assembly`），产物位于 `third_party/prebuilt/{arm64-v8a,x86_64}` |
| P3 C 内核 | 内核源码在 `entry/src/main/cpp/core`，GLib 垫片在 `cpp/compat`（`glib.h`、`glib-object.h`、`glib-compat.c`）；在 WSL 中用 gcc 跑测试：上游 `test-mp` 63/63、`test-mp-equation` 511/511、移植测试 `test-port` 18/18 全部通过 |
| P4 引擎 + N-API | `cpp/engine/calc-engine.c`：`math-equation.c` 的无 GTK 移植版（文本缓冲、ans 区间、撤销/重做、千位分隔符重排、历史、变量、换算、财务、位操作），`test-engine` 71/71 通过；`cpp/napi_calc.cpp` 导出 38 个接口，求解和因式分解在 worker 线程上执行并返回 Promise；类型声明在 `cpp/types/libmatecalc/Index.d.ts`；HAP 构建通过，包含两种 ABI 的 so |

主机测试命令（WSL）：`make -C MateCalc/entry/src/main/cpp/tests DEPS=$HOME/hmdeps/root test`（依赖通过 `apt-get download` 获取 libgmp/libmpfr/libmpc 的 dev 包，解压到 `~/hmdeps/root`）。

### 相对上游的行为修改（均已在代码注释中标明）

1. `financial.c` 的 DDB：上游判断条件写成了 `len >= 0`，导致所有合法期数都会报错；改为 `len < 0`。
2. 财务函数出错时：上游先设置状态再设置数字，而设置数字时会清空状态，错误提示因此丢失；改为先设置数字、后设置状态。
3. 千位分隔符重排：只删除夹在两个数字之间的"多余分隔符"，避免在分隔符是空格的 locale 下吃掉因式分解结果（`2^3 × 3`）里的空格。
4. 汇率：GIO 下载和 libxml2 解析改为由 ArkTS 下载，C 层解析文本（`currency_manager_load_rates`）；行为与上游相同（IMF 为主，ECB 补充）。
5. 变量文件：存放路径由 ArkTS 传入应用沙箱路径。
6. 翻译：C 层返回英文 msgid，由 ArkTS 资源负责翻译。

### 2026-10-16：P5–P8 完成（构建通过，尚未在设备上验证）

ArkTS 代码位于 `MateCalc/entry/src/main/ets/`：

| 文件 | 内容 |
|---|---|
| `model/CalcModel.ets` | `@Observed` 的 UI 状态，封装原生引擎：显示字符（ans 加粗、选区、光标）、模式/进制、偏好设置、位面板、历史、换算器、汇率 |
| `model/ButtonSpecs.ets` | 四种模式的按键网格，按上游 `.ui` 文件里的坐标逐一转写；插入文本和提示取自 `button_data[]`；f(x)/const 菜单；10 个财务对话框的字段定义 |
| `model/SettingsStore.ets` | Preferences 持久化，键名与 `org.mate.calc` GSettings schema 一致 |
| `model/CurrencyService.ets` | 下载 IMF/ECB 汇率并缓存到 cacheDir，超过 7 天自动刷新（与上游策略相同） |
| `components/` | `CalcDisplay`（Span 渲染、闪烁光标、点击定位）、`CalcKey`、`ButtonPanel`（网格、三角函数子网格、下拉菜单）、`BitPanel`、`HistoryPanel`、`ConverterBar`、`VariablePopup` |
| `dialogs/` | 偏好设置、财务对话框、插入字符码、关于（显示 MPFR/MPC 版本）、键盘快捷键帮助 |
| `pages/Index.ets` | 菜单栏（计算器/模式/查看/帮助，带快捷键标注）、窗口布局、完整的键盘处理（对应 `math-display.c` 和 `math-window.c`） |
| `common/I18n.ets` | 由 `tools/po2json.py` 把上游 84 种语言的 `.po` 转成 `rawfile/i18n/*.json`，按系统 locale 加载并去掉助记符 |
| `common/WindowHolder.ets` | 按模式调整 PC 窗口大小（上游窗口大小随模式变化且不可缩放） |

其他改动：应用图标（`tools/make_icons.py` 重新绘制的分层图标）、浅色/深色主题色、`ohos.permission.INTERNET` 权限、PC 窗口最小尺寸、版本号 1.28.0。

与上游的差异：
- 换算器把上游带分类层级的单位下拉框拆成"分类 + 源单位 + 目标单位"三个 Select（ArkUI 的 Select 不支持树形选项）。
- 帮助：上游打开 yelp 帮助文档，这里改为显示快捷键列表对话框。
- 粘贴：鸿蒙普通应用读取剪贴板必须经过 `PasteButton` 安全控件，所以菜单里的"粘贴"是 PasteButton；Ctrl+V 会先尝试直接读取，读取失败时在状态栏提示使用菜单。

### 真机接入后的验证清单

1. 签名：`devecocli auth login` → `devecocli signature generate`，然后安装运行
2. 四种模式的按键逐一点击；Enter/Esc/退格/方向键/Shift 选择；Ctrl/Alt 输入上下标；Ctrl+E/F/I/P/R
3. 点击显示区文字定位光标；ans 加粗显示；千位分隔符和小数点跟随系统 locale
4. 程序员模式：切换进制、点击位面板、移位菜单、字符码对话框
5. 财务对话框计算结果；变量弹窗的存储、插入和删除；历史面板；Alt+↑/↓ 浏览历史
6. 换算器：单位换算，以及联网下载汇率后的货币换算
7. 偏好设置的持久化；窗口按模式调整大小；深色模式；中文/英文界面
8. Ctrl+V 和 PasteButton 粘贴、Ctrl+C 复制
9. 用 arkts-runtime-fix 排查运行期崩溃；hilog 中没有异常

### 2026-10-16：真机验证（P9/P10）

设备：HUAWEI MateBook Pro（2in1，HarmonyOS API 23）。为兼容该设备，`compatibleSdkVersion` 降为 6.1.0(23)；签名用 `devecocli signature generate` 生成。

**自动化测试**：`tools/devtest.py` 通过 `hdc uitest` 抓取布局、模拟点击和按键，并读取显示区内容；场景脚本在 `tests/device/`（基础、键盘、高级、编程四个模式）。从清空应用数据的状态开始执行，**58 项断言全部通过**；faultlog 和 hilog 中没有崩溃，也没有 JS 异常。主机测试 665 项全部通过。截图保存在 `docs/screenshots/`。

**手动验证通过的功能**：f(x)/常量/移位下拉菜单；变量的存储、插入和重启后保留；位面板点击；字符码对话框；财务对话框（Sln=100、Ddb=400，Enter 提交）；偏好设置及其持久化；千位分隔符跟随中文 locale；历史面板（点击条目、Alt+↑ 浏览）；复制、Ctrl+V 粘贴和 PasteButton 粘贴；关于/帮助对话框；深色模式；窗口按模式调整大小；货币换算（`$100 = 元668.69`，汇率文件从电脑下载后推送到设备）。

**真机上发现并已修复的问题**：

| # | 问题 | 原因 / 修复 |
|---|---|---|
| 1 | 切换模式后按键布局不变 | `@Builder` 的参数按值传递，模式改变时不会重新执行 → 改为在 builder 内部直接读取 `this.panel()` |
| 2 | 下拉标记 ▾ 显示成小圆点 | 系统字体缺少这个字形 → 改用 `Polygon` 绘制 |
| 3 | 单位名称、错误信息等 C 层文本只有英文 | C 层的 `_()` 原先直接返回原文 → 新增 `setTranslations`，把上游翻译表注入 C 层 |
| 4 | 变量弹窗一点输入框就关闭 | `bindMenu` 的菜单在内部控件获得焦点时会自动关闭 → 改用 `bindPopup` |
| 5 | **≥10 或 <1 的变量重启后丢失（上游 bug）** | `mp_set_from_string` 不能解析序列化后写入的 `4.2×10¹` → 补上上标指数解析，并新增 2 项回归测试 |
| 6 | 对话框内容左右被裁掉 | 系统默认对话框宽度小于内容宽度 → 在 `CustomDialogController` 上设置 `width`，内容改为 100% |
| 7 | "计算器"菜单过宽，粘贴项显示为黑条 | PasteButton 设成了 100% 宽度和透明背景 → 改为固定宽度和不透明背景色 |
| 8 | 窗口较矮时"关于"对话框底部被截断；"License/Operators"未翻译 | 压缩对话框高度；这几个词在上游词条里没有，改用应用字符串资源 |

**环境说明**（已解决，见下文）：设备当时连接的校园 Wi-Fi 需要先在门户网页认证（HTTP 请求被重定向到 202.118.1.87，HTTPS 握手失败），所以联网下载汇率暂时无法在设备上验证。应用对下载失败的处理正常：不会崩溃，换算器只是没有结果；接入正常网络后会自动下载。

**联网汇率下载（2026-10-17，用户登录 Wi-Fi 后）**：清空应用数据后进入财务模式，应用自己从 IMF（约 10 秒）和 ECB（不到 1 秒）下载了 `rms_five.xls` 和 `eurofxref-daily.xml`，并写入模块缓存目录，过程中没有报错。换算栏显示 `$100 = 元668.69`，表达式 `100 EUR in JPY = 17797.746…`。重启应用后没有再下载，直接读取缓存（缓存 7 天内有效，与上游策略一致）。至此，验证清单上的所有项目都已在真机上通过。
