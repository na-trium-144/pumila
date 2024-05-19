# pumila

ニューラルネットワークにぷよぷよを学習させたい

<del>5〜8連鎖が打てます</del>

![pumila11.gif](pumila11.gif)

pumila13以前はC++(Eigen)で実装していたのをpumila14からPyTorchに移行しました

## ビルド、環境構築

* C++17が使えるコンパイラが必要です
    * Windows,Linux,MacOSでビルドできるはず
* 依存ライブラリとして [pybind11](https://github.com/pybind/pybind11), [BS::thread_pool](https://github.com/bshoshany/thread-pool), [googletest](https://github.com/google/googletest.git) とroboto-font(GUIの表示に必要)がFetchContentでダウンロードされます
* ビルド
```sh
mkdir build
cd build
cmake ..
make
```

* Python3.9以上が必要です
* jupyter, tqdm, matplotlib, torch, (psutil, pygame) をインストールしてください
    * poetryが使えれば `poetry install` でok
    * GPU使う場合は別途GPUが有効なtorchをインストールする必要がある

<details><summary>pumila13以前</summary>

* Eigen3がインストールされていればそれを使い、なければFetchContentで自動的にダウンロードします
    * ubuntu: `sudo apt install libeigen3-dev`
    * mac: `brew install eigen3`
* GUI(`pumila::Window`)を使うにはSDL2とSDL2_TTFをインストールする必要があります(linux, macのみ)
    * インストールされてない場合GUI関係の機能が無効化されますがビルドは可能です
    * ubuntu: `sudo apt install libsdl2-dev libsdl2-ttf-dev`
    * mac: `brew install sdl2 sdl2_ttf`
    * windowsの場合はFetchContentで自動的にダウンロードされます
* [pybind11](https://github.com/pybind/pybind11), [BS::thread_pool](https://github.com/bshoshany/thread-pool), [cli11](https://github.com/CLIUtils/CLI11) とroboto-font(GUIの表示に必要)がFetchContentでダウンロードされます
* IntelCPUの場合cmake時に`-DPUMILA_MKL_DYNAMIC=ON`または`-DPUMILA_MKL_STATIC=ON`オプションを追加するとIntel MKLを使用し計算が速くなります
    * windows: [ここ](https://www.intel.com/content/www/us/en/developer/tools/oneapi/onemkl-download.html)からoneMKLをダウンロードしインストール (古いCPUはサポートされてないっぽい)
        * インストール場所がデフォルト (`C:\Program Files (x86)\Intel\oneAPI`) でない場合は `-DPUMILA_MKL_ROOT=Path\To\oneAPI`
    * ubuntu: `sudo apt install libmkl-dev` (古めのCPUでも動く)
* MacOSの場合cmake時に`-DPUMILA_ACCELERATE=ON`オプションを追加するとAccelerateを使用し計算が速くなります

</details>

## 使い方

[Releases](https://github.com/na-trium-144/pumila/releases)に適当に学習後のモデルを置いているのでこれをダウンロードしてbuildディレクトリの中に置いてください

### pumila-core

* C++でぷよぷよのゲームシステムを実装したものです
* pythonからアクセスできるようbindingを作ります (pypumila)
* pumila-test に簡単なテストを書いています (buildディレクトリの中で `ctest` で実行できます)

### pumila

* Pythonで書いたpypumilaのラッパー+ニューラルネット周りの処理を書いたライブラリです
* pygameを使ってぷよぷよの盤面を可視化しユーザーが操作することもできるシミュレータがあります (pumila.Window クラス)

<details><summary>pumila13以前</summary>

buildディレクトリの中で`./pumila-sim`を実行するとシミュレータが起動します
引数にAIのモデル(pumila3, pumila5)または player を最大2つまで指定してください

```sh
# 例 player vs AI
./pumila-sim player pumila5
```
playerの操作は A / D キーで横移動、N / M キーで回転、S キーで高速落下、(Wキーで瞬間落下) です

</details>

### notebook

* notebook ディレクトリにあるノートブックで学習させています
