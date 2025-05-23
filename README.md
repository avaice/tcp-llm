# TCP/IP OpenAI API サーバー

TCP/IP 通信を使って OpenAI API にアクセスできるサーバーです。クライアントからテキストメッセージを送信すると、OpenAI API を使って応答を生成し、結果をクライアントに返します。

## 機能

- TCP/IP ソケット通信によるサーバー・クライアント間の通信
- OpenAI API を使った自然言語処理
- シンプルなコマンドラインインターフェース
- **直近 10 個の会話履歴を保持**し、文脈を考慮した応答
- **複数クライアントの同時接続**をサポート（各クライアントごとに独立した会話履歴を管理）
- 会話履歴のクリア機能
- **複数の言語モデルを選択可能**（gpt-3.5-turbo, gpt-4.1-2025-04-14, gpt-4.1-nano-2025-04-14, o4-mini-2025-04-16）
- **レスポンスに使用モデル情報を表示**（どのモデルが応答を生成したかが一目でわかる）

## 必要条件

- Node.js 18.0.0 以上
- OpenAI API キー

## セットアップ

### 通常のセットアップ

1. リポジトリをクローン

```bash
git clone https://github.com/yourusername/tcp-llm.git
cd tcp-llm
```

2. 依存パッケージをインストール

```bash
npm install
```

3. `.env`ファイルを設定

```
# OpenAI API設定
OPENAI_API_KEY=your_openai_api_key_here

# サーバー設定
PORT=3000
HOST=127.0.0.1
```

### Docker を使用したセットアップ

1. リポジトリをクローン

```bash
git clone https://github.com/yourusername/tcp-llm.git
cd tcp-llm
```

2. `.env`ファイルを設定

```
# OpenAI API設定
OPENAI_API_KEY=your_openai_api_key_here
```

3. Docker コンテナをビルドして起動

```bash
# デフォルトポート(3000)で起動
docker-compose up -d

# 別のポートを指定して起動（例：4000番ポート）
PORT=4000 docker-compose up -d
```

これにより、サーバーがバックグラウンドで起動します。ログを確認するには：

```bash
docker-compose logs -f
```

4. コンテナを停止するには

```bash
docker-compose down
```

5. ポートの指定方法

ポートは以下の方法で指定できます：

- 環境変数で直接指定: `PORT=4000 docker-compose up -d`
- `.env`ファイルに追加:
  ```
  # サーバー設定
  PORT=4000
  ```

## 使い方

### サーバーの起動

```bash
npm start
```

サーバーが起動すると、以下のメッセージが表示されます：

```
TCP/IPサーバーが起動しました - 127.0.0.1:3000
```

### クライアントの実行(デバッグ用)

別のターミナルウィンドウで以下のコマンドを実行します：

```bash
npm run client
```

クライアントが接続されると、メッセージの入力を求められます：

```
サーバー(127.0.0.1:3000)に接続しました
メッセージを入力してください（コマンド一覧は '/help'、終了するには 'exit'）:
```

### 使用可能なコマンド

クライアントでは以下のコマンドが使用できます：

- `/help` - 使用可能なコマンド一覧を表示
- `/clear` - 現在の会話履歴をクリア（新しい会話を開始）
- `/models` - 利用可能なモデル一覧と現在選択中のモデルを表示
- `/model モデル名` - 使用するモデルを変更（例: `/model gpt-4.1-2025-04-14`）
- `exit` - クライアントを終了

### 利用可能なモデル

以下のモデルが利用可能です：

- `gpt-3.5-turbo` - デフォルトモデル
- `gpt-4.1-2025-04-14` - GPT-4.1 標準モデル
- `gpt-4.1-nano-2025-04-14` - GPT-4.1 軽量モデル
- `o4-mini-2025-04-16` - O4 Mini モデル

### クライアントの終了

クライアントを終了するには、`exit`と入力して Enter キーを押します。

## カスタマイズ

### 一般的なカスタマイズ

- `.env`ファイルでポート番号やホスト名を変更できます
- `src/index.ts`の`processMessage`関数で OpenAI API のパラメータを調整できます
- `MAX_HISTORY`定数を変更して、保持する会話履歴の数を調整できます（デフォルトは 10）
- `AVAILABLE_MODELS`配列を編集して、利用可能なモデルを追加・削除できます
- 必要に応じて、メッセージのフォーマットやプロトコルをカスタマイズできます

### Docker 関連のカスタマイズ

- `docker-compose.yml`ファイルを編集して、ポートマッピングやボリュームマウントを変更できます
- `Dockerfile`を編集して、ビルドプロセスをカスタマイズできます
- 本番環境では、`docker-compose.prod.yml`を作成して、本番用の設定を追加することをお勧めします

## 注意事項

- このサーバーは開発・テスト目的で作成されています
- 本番環境で使用する場合は、セキュリティ対策を追加してください
- OpenAI API の利用料金が発生する可能性があります
- 会話履歴はメモリ上に保持されるため、サーバー再起動時にはすべての履歴が失われます
