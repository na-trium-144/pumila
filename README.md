# pumila

## ビルド
* C++17が使えるコンパイラが必要です
* Eigen3がインストールされていればそれを使い、なければFetchContentで自動的にダウンロードします
* GUI(`pumila::Window`)を使うにはSDL2とSDL2_TTFをインストールする必要があります
    * インストールされてない場合GUI関係の機能が無効化されますがビルドは可能です
* pybind11とroboto-font(GUIの表示に必要)がFetchContentでダウンロードされます
* ubuntuの場合
```sh
sudo apt install libeigen3-dev libsdl2-dev libsdl2-ttf-dev
```
* macの場合
```sh
brew install eigen3 sdl2 sdl2_ttf
```
* ビルド
```sh
cmake -Bbuild
cmake --build build
```
