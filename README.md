# openjtalk-native

**[English](#english)**

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

### Android

Docker を使用してクロスコンパイルします:

```bash
docker build -f docker/Dockerfile.android -t openjtalk-android .
docker run --rm -v "${PWD}:/workspace" -w /workspace openjtalk-android bash -c "
  ./scripts/fetch_dependencies.sh
  ./scripts/build_dependencies_android.sh
  cmake -B build \
    -DCMAKE_TOOLCHAIN_FILE=\$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-21 \
    -DCMAKE_BUILD_TYPE=Release
  cmake --build build -j\$(nproc)
"
```

### iOS

```bash
./scripts/fetch_dependencies.sh
./scripts/build_dependencies_ios.sh
cmake -B build_ios \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
  -DCMAKE_BUILD_TYPE=Release \
  -DIOS=ON
cmake --build build_ios
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

### スレッドセーフティ

- `openjtalk_native_create()` で返されるハンドルはそれぞれ独立しています
- 異なるハンドルは異なるスレッドから同時に使用できます
- 同一ハンドルを複数スレッドから同時に使用してはいけません
- `openjtalk_native_get_version()` と `openjtalk_native_get_error_string()` は任意のスレッドから安全に呼び出せます

## ディレクトリ構成

```
openjtalk-native/
├── include/           パブリックヘッダー (openjtalk_native.h)
├── src/               コア実装
├── test/              テストコード
├── scripts/           ビルドスクリプト
├── docker/            Docker ビルド環境
├── .github/workflows/ CI/CD ワークフロー
└── external/          ビルド時にダウンロードされるソース（gitignore 対象）
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

---

<a id="english"></a>

# openjtalk-native (English)

**[日本語](#openjtalk-native)**

A cross-platform native shared library for OpenJTalk. Provides a C API for converting Japanese text to phonemes.

## Supported Platforms

| Platform | Architecture | Output |
|:--|:--|:--|
| Windows | x64 | `openjtalk_native.dll` |
| Linux | x86_64 | `libopenjtalk_native.so` |
| macOS | arm64 + x86_64 | `libopenjtalk_native.dylib` |
| Android | arm64-v8a, armeabi-v7a, x86, x86_64 | `libopenjtalk_native.so` |
| iOS | arm64 | `libopenjtalk_native.a` |

## Prebuilt Binaries

Download prebuilt binaries for each platform from the [Releases](https://github.com/ayutaz/openjtalk-native/releases) page.

Each release includes a SHA256 checksum file (`SHA256SUMS.txt`). Verify integrity after downloading:

```bash
sha256sum -c SHA256SUMS.txt
```

## Dictionary Files

OpenJTalk requires a MeCab dictionary (UTF-8 version).

1. Download `open_jtalk_dic_utf_8-1.11.tar.gz` from the [Open JTalk download page](https://open-jtalk.sourceforge.net/)
2. Extract to any directory
3. Pass the dictionary directory path to `openjtalk_native_create()`

The dictionary directory must contain `sys.dic`, `matrix.bin`, `char.bin`, and `unk.dic`.

## Building

### Prerequisites

- CMake 3.10+
- C99-compatible compiler
- Linux/macOS: autotools (autoconf, automake, libtool)
- Windows: MSVC or MinGW

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

### Android

Cross-compile using Docker:

```bash
docker build -f docker/Dockerfile.android -t openjtalk-android .
docker run --rm -v "${PWD}:/workspace" -w /workspace openjtalk-android bash -c "
  ./scripts/fetch_dependencies.sh
  ./scripts/build_dependencies_android.sh
  cmake -B build \
    -DCMAKE_TOOLCHAIN_FILE=\$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-21 \
    -DCMAKE_BUILD_TYPE=Release
  cmake --build build -j\$(nproc)
"
```

### iOS

```bash
./scripts/fetch_dependencies.sh
./scripts/build_dependencies_ios.sh
cmake -B build_ios \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
  -DCMAKE_BUILD_TYPE=Release \
  -DIOS=ON
cmake --build build_ios
```

## API

### Basic Usage

```c
#include "openjtalk_native.h"

// Create an instance (specify dictionary path)
void* handle = openjtalk_native_create("/path/to/open_jtalk_dic_utf_8-1.11");
if (!handle) {
    // Error: dictionary not found or out of memory
    return;
}

// Phoneme conversion
OpenJTalkNativePhonemeResult* result = openjtalk_native_phonemize(handle, "こんにちは");
if (result) {
    printf("Phonemes: %s\n", result->phonemes);       // e.g., "pau k o N n i ch i w a pau"
    printf("Count: %d\n", result->phoneme_count);

    // Caller is responsible for freeing the result
    openjtalk_native_free_result(result);
}

// Destroy instance
openjtalk_native_destroy(handle);
```

### Phonemization with Prosody

```c
OpenJTalkNativeProsodyResult* prosody = openjtalk_native_phonemize_with_prosody(handle, "東京は晴れです");
if (prosody) {
    for (int i = 0; i < prosody->phoneme_count; i++) {
        // A1: Relative position from accent nucleus
        // A2: Position in accent phrase (1-based)
        // A3: Total morae in accent phrase
        printf("A1=%d A2=%d A3=%d\n",
            prosody->prosody_a1[i], prosody->prosody_a2[i], prosody->prosody_a3[i]);
    }
    openjtalk_native_free_prosody_result(prosody);
}
```

### Options

```c
// Available option keys:
//   "speech_rate" — Speech rate multiplier (0.0 < rate <= 10.0, default: 1.0)
//   "pitch"       — Pitch shift in semitones (-20.0 <= pitch <= 20.0, default: 0.0)
//   "volume"      — Volume multiplier (0.0 <= volume <= 2.0, default: 1.0)
openjtalk_native_set_option(handle, "speech_rate", "1.5");

const char* val = openjtalk_native_get_option(handle, "speech_rate");
// Note: returned pointer references an internal buffer, overwritten on next get_option call
```

### Error Handling

```c
OpenJTalkNativePhonemeResult* result = openjtalk_native_phonemize(handle, text);
if (!result) {
    int err = openjtalk_native_get_last_error(handle);
    printf("Error: %s\n", openjtalk_native_get_error_string(err));
}
```

### Thread Safety

- Each handle returned by `openjtalk_native_create()` is independent
- Different handles can be used concurrently from different threads
- A single handle must NOT be used from multiple threads simultaneously
- `openjtalk_native_get_version()` and `openjtalk_native_get_error_string()` are safe to call from any thread

## Directory Structure

```
openjtalk-native/
├── include/           Public header (openjtalk_native.h)
├── src/               Core implementation
├── test/              Tests
├── scripts/           Build scripts
├── docker/            Docker build environments
├── .github/workflows/ CI/CD workflows
└── external/          Sources downloaded at build time (gitignored)
```

## Troubleshooting

### Dictionary Not Found

If `openjtalk_native_create()` returns `NULL`, verify the dictionary path is correct. The path must be a UTF-8 encoded directory path.

### OpenJTalk Headers Not Found During Build

Run `fetch_dependencies.sh` (or `.ps1`) and `build_dependencies.sh` first. Header files are installed to `external/install/include/`.

### Input Text Too Long

Input text is limited to 4096 bytes. Inputs exceeding this limit return `OPENJTALK_NATIVE_ERROR_INVALID_INPUT`. Split the text into smaller chunks.

## License

BSD-3-Clause License - See [LICENSE](LICENSE).

Subject to the licenses of OpenJTalk and HTS Engine API.
