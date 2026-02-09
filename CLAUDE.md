# openjtalk-native 開発ガイドライン

## プロジェクト概要

OpenJTalk のネイティブ共有ライブラリ。日本語テキストから音素変換を行う C API を提供する。

## ビルド手順

### Linux/macOS

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

## コーディング規約

- ライブラリ名: `openjtalk_native`
- API 接頭辞: `openjtalk_native_`
- エクスポートマクロ: `OPENJTALK_NATIVE_EXPORTS`
- C99 準拠
- パブリック API は `include/openjtalk_native.h` で定義

## ディレクトリ構成

- `include/` — パブリックヘッダー
- `src/` — コア実装
- `cmake/` — 依存関係の CMake ビルド設定
- `scripts/` — ビルドスクリプト
- `test/` — テスト
- `docker/` — Docker ビルド環境
- `external/` — ビルド時にダウンロードされるソース（.gitignore 対象）

## テスト

```bash
cmake -B build -DBUILD_TESTS=ON
cmake --build build
cd build && ctest --output-on-failure
```

## リリース

タグを push すると GitHub Actions が自動的にリリースを作成する。

```bash
git tag v1.0.0
git push origin v1.0.0
```
