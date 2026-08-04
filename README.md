# Tsurido

[![Github license](https://img.shields.io/github/license/hotstaff/tsurido)](https://github.com/hotstaff/tsurido/)

Forked from [hotstaff/tsurido](https://github.com/hotstaff/tsurido/tree/master/sketch/tsurido-m5stickc)

## Hardware

- [M5 StickS3](https://github.com/hotstaff/tsurido/tree/master/sketch/tsurido-m5stickc)

M5StickS3対応や省電力化のため、一部ソースコードを改変・削減しています。

また、M5StickS3ではユーザーLEDが無いため、スピーカーを用いてアタリを通知します。

## Software

- [VSCode](https://code.visualstudio.com)
- [PlatformIO](https://platformio.org)


## 動作モード

動作モードは以下の２つがあります。切り替えはAボタンで行います。写真はM5StickCのバージョンを流用しています

- リアルタイムプロットモード

    - ![リアルタイムプロットモードの画面](https://github.com/hotstaff/tsurido/blob/master/sketch/tsurido-m5stickc/m5stickc-m1.png)

- 簡易表示モード（プロットなしなのでやや省電力です）

    - ![簡易表示モード](https://github.com/hotstaff/tsurido/blob/master/sketch/tsurido-m5stickc/m5stickc-m2.png)


## 省電力モード（センサーモード）

Bボタンを押すことで画面が消灯しBLEとセンサーのみが動いている省電力モードに落ちることができます。本体の電池で約X時間〜X時間XX分程度の動作が可能です。（計測中）
省電力モード時の場合もスピーカーは出力されます。
