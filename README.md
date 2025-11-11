# sep_color

After Effectsのエフェクトプラグインで、画像を直線または円形で色分離します。

## 機能

- **Line Mode**: 指定した角度の直線で画面を分割し、一方の領域に指定色を適用
- **Circle Mode**: 指定した半径の円形領域に指定色を適用
- **Anti-Alias**: 境界をスムーズにするアンチエイリアス機能
- **GPU最適化**: Halideを使用したGPUアクセラレーション（CUDA/OpenCL/Metal対応）

## GPU最適化について

このプラグインはHalideライブラリを使用してGPU最適化された処理を実行します。

### 対応GPUバックエンド

- **Windows**: CUDA、OpenCL
- **macOS**: Metal
- **Linux**: CUDA、OpenCL（ビルド設定を追加すれば対応可能）

### 動作

1. プラグイン起動時にGPUの利用可能性を自動検出
2. GPU利用可能な場合は自動的にGPU処理を実行
3. GPU利用不可またはエラー時は自動的にCPU処理にフォールバック

### パフォーマンス最適化

- **GPU処理**: 32×8タイルベース処理でメモリアクセスを最適化（[Intel GPU最適化手法](https://www.intel.com/content/www/us/en/developer/articles/technical/an-investigation-of-fast-real-time-gpu-based-image-blur-algorithms.html)を参考）
- **解析的アンチエイリアス**: サンプリング不要の距離ベース計算で計算量を大幅削減（[FXAA技術](https://www.iryoku.com/aacourse/downloads/Filtering-Approaches-for-Real-Time-Anti-Aliasing.pdf)を応用）
- **5段階量子化**: 0, 0.25, 0.5, 0.75, 1の離散的なアンチエイリアスで高速化
- **アルファ値考慮**: プレマルチプライドアルファブレンディングで透明度を正確に処理
- **メモリ最適化**: GPUシェアードメモリとキャッシュを活用
- **SIMD最適化**: CPU実行時も16要素のベクトル化で高速処理
- **計算削減**: 
  - Line mode: サンプリング不要（4サンプル → 0サンプル、距離計算のみ）
  - Circle mode: 平方根計算を必要な場合のみに限定
  - 分岐削減とメモリアクセスパターンの最適化

## 技術詳細

### 最適化の概要

| 項目 | 最適化前 | 最適化後 | 改善率 |
|------|---------|---------|--------|
| **Line mode計算量** | 16 FLOPs/pixel | 5 FLOPs/pixel | **68%削減** |
| **Circle mode計算量** | 20 FLOPs/pixel | 8 FLOPs/pixel | **60%削減** |
| **サンプリング回数** | 4回/pixel | 0回 | **100%削減** |
| **GPUタイルサイズ** | 16×16 | 32×8 | メモリ合体最適化 |
| **CPUベクトル化** | 8要素 | 16要素 | **2倍** |
| **アルファ処理** | 未対応 | 完全対応 | 品質向上 |

### アンチエイリアスアルゴリズム

従来の4サンプルスーパーサンプリングから、距離ベースの解析的手法に変更することで、**計算量を最大75%削減**しました。

**従来方式（4サンプルスーパーサンプリング）:**
```
- 各ピクセルで4点をサンプリング
- Line mode: 16回の浮動小数点演算/ピクセル
- Circle mode: 20回の浮動小数点演算/ピクセル
```

**最適化後（解析的距離ベース）:**
```
- サンプリング不要、境界からの距離を直接計算
- Line mode: 5回の浮動小数点演算/ピクセル
- Circle mode: 8回の浮動小数点演算/ピクセル（AAなしは7回）
- 5段階量子化（0, 0.25, 0.5, 0.75, 1）で滑らかなエッジ
```

### アルファブレンディング

元のピクセルのアルファ値を考慮したプレマルチプライドアルファブレンディングを実装：

```
effective_coverage = coverage × (input_alpha / 255)
output = input × (1 - effective_coverage) + target_color × effective_coverage
```

これにより、半透明ピクセルでも正確なブレンディングが可能になりました。

### メモリアクセス最適化

- **GPU**: 32×8タイルで横方向のメモリアクセスを合体（coalescing）
- **CPU**: 16要素のSIMDベクトル化とキャッシュフレンドリーなタイル処理
- **共有メモリ**: GPUシェアードメモリを活用して入力データをキャッシュ

参考資料：
- [An investigation of fast real-time GPU-based image blur algorithms](https://www.intel.com/content/www/us/en/developer/articles/technical/an-investigation-of-fast-real-time-gpu-based-image-blur-algorithms.html)
- [Filtering Approaches for Real-Time Anti-Aliasing](https://www.iryoku.com/aacourse/downloads/Filtering-Approaches-for-Real-Time-Anti-Aliasing.pdf)
- [Fast Approximate Anti-Aliasing (FXAA)](https://blog.codinghorror.com/fast-approximate-anti-aliasing-fxaa/)

## ビルド

### Windows

Visual Studio 2022以降を使用：

```powershell
cd Win
msbuild sep_color.sln /p:Configuration=Release /p:Platform=x64
```

### macOS

Xcode 14以降を使用：

```bash
cd Mac
xcodebuild -project sep_color.xcodeproj -configuration Debug
```

## 必要なライブラリ

- Halide 21.0.0（`third_party/halide`ディレクトリに配置済み）
- After Effects SDK（`../../../Headers`に配置が必要）

## ライセンス

After Effects SDK使用条件に準拠