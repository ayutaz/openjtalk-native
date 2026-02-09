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

```c
#include "openjtalk_native.h"

// インスタンス作成
void* handle = openjtalk_native_create("/path/to/dictionary");

// 音素変換
OpenJTalkNativePhonemeResult* result = openjtalk_native_phonemize(handle, "こんにちは");
printf("Phonemes: %s\n", result->phonemes);

// 結果の解放
openjtalk_native_free_result(result);

// インスタンス破棄
openjtalk_native_destroy(handle);
```

## ライセンス

Modified BSD License - [LICENSE](LICENSE) を参照してください。

OpenJTalk および HTS Engine API のライセンスに準じます。
