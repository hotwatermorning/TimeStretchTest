# What's this

このプロジェクトは、[音楽ツール・ライブラリ・技術 Advent Calendar 2018](https://adventar.org/calendars/3353) 三日目の記事用に作成した、サンプルプロジェクトです。

SoundTouchとRubberBandという2種類のライブラリを使用して、`resource/test.wavファイルをタイムストレッチ／ピッチシフトしたオーディオファイルを作成します。

## ビルド&実行方法

```sh
cd gradle
./gradlew build_all
cd ../build_debug/Debug
./TimeStretchTest
```

これによって、`resource`ディレクトリのtest.wavを、SoundTouchとRubberBandでそれぞれタイムストレッチ／ピッチシフトしたファイルが、同じく`resource`ディレクトリに生成されます。
