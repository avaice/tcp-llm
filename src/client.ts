import * as net from "node:net";
import * as readline from "node:readline";

// 接続設定
const PORT = Number.parseInt(process.env.PORT || "3000", 10);
const HOST = process.env.HOST || "127.0.0.1";

// ソケット作成
const client = new net.Socket();

// 標準入力の設定
const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout,
});

// 使用可能なコマンドの表示
function showHelp(): void {
  console.log("\n=== 使用可能なコマンド ===");
  console.log("/help   - このヘルプメッセージを表示");
  console.log("/clear  - 会話履歴をクリア");
  console.log("/models - 利用可能なモデル一覧と現在のモデルを表示");
  console.log("/model モデル名 - 使用するモデルを変更");
  console.log("exit    - クライアントを終了");
  console.log("========================\n");
}

// サーバーに接続
client.connect(PORT, HOST, () => {
  console.log(`サーバー(${HOST}:${PORT})に接続しました`);
  console.log(
    "メッセージを入力してください（コマンド一覧は '/help'、終了するには 'exit'）:"
  );

  // ユーザー入力の処理
  rl.on("line", (input) => {
    // 終了コマンド
    if (input.toLowerCase() === "exit") {
      console.log("接続を終了します...");
      client.end();
      rl.close();
      return;
    }

    // ヘルプコマンド
    if (input.toLowerCase() === "/help") {
      showHelp();
      return;
    }

    // サーバーにメッセージ送信
    client.write(`${input}\n`);
  });
});

// レスポンス型の定義
interface ResponseData {
  model: string;
  content: string;
}

// サーバーからのデータ受信
client.on("data", (data) => {
  const responseText = data.toString().trim();

  try {
    // JSONレスポンスの解析を試みる
    const responseData = JSON.parse(responseText) as ResponseData;

    // モデル情報とレスポンス内容を表示
    console.log("\n=== AIからの応答 ===");
    console.log(`[モデル: ${responseData.model}]`);
    console.log(responseData.content);
  } catch (error) {
    // JSON解析に失敗した場合（コマンドレスポンスなど）は、そのまま表示
    console.log("\n=== サーバーからの応答 ===");
    console.log(responseText);
  }

  console.log(
    "\n次のメッセージを入力してください（コマンド一覧は '/help'、終了するには 'exit'）:"
  );
});

// 接続終了時の処理
client.on("close", () => {
  console.log("サーバーとの接続が閉じられました");
  process.exit(0);
});

// エラー処理
client.on("error", (err) => {
  console.error("エラーが発生しました:", err.message);
  process.exit(1);
});
