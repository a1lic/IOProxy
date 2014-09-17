# I/O Proxy
Windowsの標準入出力をUTF-8⇄UTF-16の相互変換を行いつつ代行するソフトウェアです。

* コンソールのウィンドウから入力された文字列をUnicodeで受け取り、それをUTF-8に変換して子プロセスの標準入力に渡します。
* 子プロセスの標準出力または標準エラー出力を受け取るときは、UTF-8で読み出した後にUnicode文字列に変換した後、コンソールに出力します。

このソフトウェアはコマンドプロンプトまたはコマンドスクリプト(バッチファイル)から次のようにして起動して使用します。

`IOProxy.exe <Path to process image file> [Arguments ...]`

## 使用例

Javaをコンソールアプリケーションとして起動して、対話形式のアプリケーションを動かす、等があります。

### Java製のコンソールアプリケーション

JavaをI/O Proxy経由で使用する場合、UTF-8として文字列を扱うためにJavaの起動時の引数に以下を書き加えて起動する必要があります。

`IOProxy.exe java.exe -Dfile.encoding=UTF-8 -jar hoge.jar [arguments ...]`

### CraftBukkit

CraftBukkitの場合は、jlineを無効にしないと正常に動作しない可能性があります。
そもそもCraftBukkitはjline側でUTF-16による入出力に対応していますが、`-nojline`を使用しないと動作しない環境ではUTF-16による入出力が出来ません。
その場合は次の通りになります。

`IOProxy.exe java.exe -Dfile.encoding=UTF-8 -jar craftbukkit.jar -nojline`
