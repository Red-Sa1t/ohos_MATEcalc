# 应用市场上架资料 — MATE 计算器

## 基本信息

| 项目 | 内容 |
|---|---|
| 应用名称 | MATE 计算器 |
| 包名 | com.neuohos.matecalc |
| 版本 | 1.28.0（versionCode 1028000） |
| 适用设备 | 鸿蒙电脑（2in1） |
| 分类建议 | 工具 / 效率办公 |
| 许可证 | GNU GPL v2 或更高版本（开源） |

## 一句话简介

专业的高精度计算器：科学、财务、程序员模式一应俱全，完整支持 PC 键盘操作。

## 应用介绍

MATE 计算器移植自 Linux 桌面上广受欢迎的开源计算器 MATE Calculator，专为鸿蒙电脑重新设计了界面，保留了原版强大的计算核心。它基于 GNU MPFR / MPC 任意精度数学库，内部以约 300 位有效数字进行运算，结果最多可显示 100 位小数。

【四种计算模式】
· 基本模式：四则运算、百分比、平方、开方，适合日常计算。
· 高级模式：三角函数与双曲函数、对数、幂与根、阶乘、绝对值、复数运算（实部、虚部、共轭、辐角）、质因数分解，内置光速、普朗克常数、阿伏加德罗常数等 9 个物理常数。
· 财务模式：提供复利期数、双倍余额递减折旧、终值、毛利定价、每期付款额、现值、周期利率、直线折旧、年数总和折旧、付款期数共 10 种财务函数，填表即可计算。
· 程序员模式：二进制、八进制、十进制、十六进制一键切换；64 位位编辑器可直接点击翻转比特；支持与、或、异或、非、移位、反码、补码，字长可在 8 到 64 位之间设置，并可插入字符编码。

【单位与货币换算】
支持角度、长度、面积、体积、质量、时间、温度等单位互换，也可以直接输入"20 km in m"这样的表达式。货币换算覆盖约 60 种货币，汇率来自国际货币基金组织（IMF）和欧洲中央银行（ECB），自动下载并在本地缓存一周。

【为电脑而生】
· 完整的键盘操作：直接键入算式，按 Enter 计算、Esc 清空；Ctrl+Z / Ctrl+Shift+Z 撤销与重做；Ctrl+数字输入上标，Alt+数字输入下标；Ctrl+P 输入 π、Ctrl+R 输入根号、Ctrl+F 因式分解等快捷键。
· 菜单栏、按键悬停提示与快捷键帮助，用法和桌面软件一致。
· 历史记录面板：点击算式可重新编辑，点击结果可插入当前算式；Alt+↑/↓ 快速浏览。
· 支持变量：把结果存为变量，在后续算式中直接引用，重启后依然保留。
· 可设置角度单位、数字格式（自动 / 定点 / 科学 / 工程）、小数位数、千位分隔符，自动跟随系统语言与数字格式。
· 支持深色模式，界面提供 80 多种语言。

【开源】
本应用是开源软件，遵循 GNU 通用公共许可证（GPL v2 或更高版本），计算核心来自 MATE 桌面项目。

## 新版本特性（1.28.0）

· 首个鸿蒙电脑版本，提供基本、高级、财务、编程四种模式。
· 全新的 ArkUI 界面，支持深色模式和完整的键盘快捷键。
· 支持单位与货币换算、历史记录、变量和偏好设置。

## 关键词

计算器, 科学计算器, 程序员计算器, 财务计算器, 高精度, 进制转换, 单位换算, 汇率换算, 复数, MATE

## 权限与隐私说明

· 网络（ohos.permission.INTERNET）：仅用于从 IMF 和 ECB 官方网站下载公开的汇率数据，不上传任何数据。
· 应用不收集个人信息，不包含广告与统计 SDK。设置、变量和汇率缓存都只保存在本机的应用沙箱内。
· 粘贴功能通过系统安全控件实现，只有在用户主动点击"粘贴"时才读取剪贴板。

## 截图

均为 2560×1440 PNG（16:9 横屏，符合市场要求：比例 16:9、不小于 1920×1080、单张小于 5 MB），由真机截图合成：

1. `screenshot_1.png`：高精度科学计算（高级模式 + 历史记录）
2. `screenshot_2.png`：程序员模式（十六进制，64 位位编辑器）
3. `screenshot_3.png`：财务计算与货币换算（每期付款额对话框）
4. `screenshot_4.png`：基本模式

原始窗口截图在 `raw/`。重新生成的命令是 `python tools/store_shots.py docs/store`（需连接设备）和 `python tools/store_compose.py docs/store`。

---

## English (optional)

**Short description:** A precise, full-featured calculator with scientific, financial and programming modes and complete keyboard support.

**Description:** MATE Calculator brings the popular open-source Linux desktop calculator to HarmonyOS PCs. It computes with arbitrary precision (GNU MPFR/MPC, about 300 significant digits) and offers basic, advanced (trigonometry, logarithms, complex numbers, factorization, physical constants), financial (10 functions such as periodic payment, present/future value and depreciation) and programming modes (binary/octal/decimal/hex, a 64-bit bit editor, bitwise operations). It also converts units and about 60 currencies using IMF and ECB exchange rates, and keeps a clickable history and persistent variables. It is fully usable from the keyboard, supports dark mode and 80+ languages, and is free software under the GNU GPL.
