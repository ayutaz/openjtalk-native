# openjtalk-native

クロスプラットフォームの OpenJTalk ネイティブ共有ライブラリ。日本語テキストから音素への変換機能を C API として提供します。

## サポートプラットフォーム

| プラットフォーム | アーキテクチャ | 出力 |
|:--|:--|:--|
| Windows | x64 | `openjtalk_native.dll` |
| Linux | x86_64 | `libopenjtalk_native.so` |
| macOS | arm64 + x86_64 | `libopenjtalk_native.dylib` |
| Android | arm64-v8a, armeabi-v7a, x86, x86_64 | `libopenjtalk_native.so` |
| iOS | arm64 | `libopenjtalk_native.a` |

## プリビルドバイナリ

[Releases](https://github.com/ayutaz/openjtalk-native/releases) ページからプラットフォーム別のプリビルドバイナリをダウンロードできます。

各リリースには SHA256 チェックサムファイル (`SHA256SUMS.txt`) が含まれています。ダウンロード後に整合性を確認してください:

```bash
sha256sum -c SHA256SUMS.txt
```

## 辞書ファイル

OpenJTalk の動作には MeCab 辞書（UTF-8 版）が必要です。

1. [Open JTalk のダウンロードページ](https://open-jtalk.sourceforge.net/) から `open_jtalk_dic_utf_8-1.11.tar.gz` を取得
2. 任意のディレクトリに展開
3. `openjtalk_native_create()` に辞書ディレクトリのパスを指定

辞書ディレクトリには `sys.dic`, `matrix.bin`, `char.bin`, `unk.dic` が含まれている必要があります。

## ビルド

### 前提条件

- CMake 3.10 以上
- C99 対応コンパイラ
- Linux/macOS: autotools (autoconf, automake, libtool)
- Windows: MSVC または MinGW

### Linux / macOS

```bash
./scripts/fetch_dependencies.sh
./scripts/build_dependencies.sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build -j$(nproc)
cd build && ctest --output-on-failure
```

### Windows

```powershell
.\scripts\fetch_dependencies.ps1
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## API

### 基本的な使い方

```c
#include "openjtalk_native.h"

// インスタンス作成（辞書パスを指定）
void* handle = openjtalk_native_create("/path/to/open_jtalk_dic_utf_8-1.11");
if (!handle) {
    // エラー: 辞書が見つからない、またはメモリ不足
    return;
}

// 音素変換
OpenJTalkNativePhonemeResult* result = openjtalk_native_phonemize(handle, "こんにちは");
if (result) {
    printf("Phonemes: %s\n", result->phonemes);       // e.g., "pau k o N n i ch i w a pau"
    printf("Count: %d\n", result->phoneme_count);

    // 結果の解放（呼び出し元の責任）
    openjtalk_native_free_result(result);
}

// インスタンス破棄
openjtalk_native_destroy(handle);
```

### プロソディ付き音素変換

```c
OpenJTalkNativeProsodyResult* prosody = openjtalk_native_phonemize_with_prosody(handle, "東京は晴れです");
if (prosody) {
    for (int i = 0; i < prosody->phoneme_count; i++) {
        // A1: アクセント核からの相対位置
        // A2: アクセント句内の位置（1始まり）
        // A3: アクセント句のモーラ数
        printf("A1=%d A2=%d A3=%d\n",
            prosody->prosody_a1[i], prosody->prosody_a2[i], prosody->prosody_a3[i]);
    }
    openjtalk_native_free_prosody_result(prosody);
}
```

### オプション設定

```c
// 利用可能なオプションキー:
//   "speech_rate" — 話速 (0.0 < rate <= 10.0, デフォルト: 1.0)
//   "pitch"       — ピッチ (-20.0 <= pitch <= 20.0, デフォルト: 0.0)
//   "volume"      — 音量 (0.0 <= volume <= 2.0, デフォルト: 1.0)
openjtalk_native_set_option(handle, "speech_rate", "1.5");

const char* val = openjtalk_native_get_option(handle, "speech_rate");
// 注意: 戻り値はインスタンス内部バッファを指すため、次の get_option 呼び出しで上書きされます
```

### エラーハンドリング

```c
OpenJTalkNativePhonemeResult* result = openjtalk_native_phonemize(handle, text);
if (!result) {
    int err = openjtalk_native_get_last_error(handle);
    printf("Error: %s\n", openjtalk_native_get_error_string(err));
}
```

## トラブルシューティング

### 辞書が見つからない

`openjtalk_native_create()` が `NULL` を返す場合、辞書パスが正しいか確認してください。パスには UTF-8 でエンコードされたディレクトリパスを指定します。

### ビルド時に OpenJTalk ヘッダーが見つからない

`fetch_dependencies.sh` (または `.ps1`) と `build_dependencies.sh` を先に実行してください。ヘッダーファイルは `external/install/include/` にインストールされます。

### 入力テキストが長すぎる

入力テキストは最大 4096 バイトに制限されています。これを超える入力は `OPENJTALK_NATIVE_ERROR_INVALID_INPUT` エラーを返します。テキストを分割して処理してください。

## ライセンス

BSD-3-Clause License - [LICENSE](LICENSE) を参照してください。

OpenJTalk および HTS Engine API のライセンスに準じます。
