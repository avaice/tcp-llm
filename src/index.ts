import * as net from "node:net";
import { config } from "dotenv";
import OpenAI from "openai";
import type { ChatCompletionMessageParam } from "openai/resources/chat/completions";
import { XMLBuilder } from "fast-xml-parser";

// 環境変数の読み込み
config();

// 環境変数からの設定取得
const PORT = Number.parseInt(process.env.PORT || "3000", 10);
const HOST = process.env.HOST || "0.0.0.0"; // Dockerコンテナ内では0.0.0.0にバインドして外部からアクセス可能にする
const MAX_HISTORY = 10; // 保持する会話履歴の最大数

// 利用可能なモデルの定義
const AVAILABLE_MODELS = [
  "gpt-4.1-2025-04-14",
  "gpt-4.1-nano-2025-04-14",
  "o4-mini-2025-04-16",
];

const DEFAULT_MODEL = "gpt-4.1-nano-2025-04-14";

// OpenAI APIクライアントの初期化
const openai = new OpenAI({
  apiKey: process.env.OPENAI_API_KEY,
});

// クライアントごとの会話履歴を保持するMap
// キー: クライアントの一意の識別子（IPアドレス:ポート）
// 値: 会話履歴の配列
const conversationHistories = new Map<string, ChatCompletionMessageParam[]>();

// クライアントごとの選択モデルを保持するMap
// キー: クライアントの一意の識別子（IPアドレス:ポート）
// 値: 選択されたモデル名
const clientModels = new Map<string, string>();

// クライアントIDの生成
function getClientId(socket: net.Socket): string {
  return `${socket.remoteAddress}:${socket.remotePort}`;
}

// 会話履歴の初期化
function initializeConversationHistory(clientId: string): void {
  conversationHistories.set(clientId, [
    { role: "system", content: "あなたは役立つアシスタントです。" },
  ]);
  // デフォルトモデルを設定
  clientModels.set(clientId, DEFAULT_MODEL);
}

// クライアントのモデルを取得
function getClientModel(clientId: string): string {
  return clientModels.get(clientId) || DEFAULT_MODEL;
}

// クライアントのモデルを設定
function setClientModel(clientId: string, model: string): boolean {
  // 指定されたモデルが利用可能なモデルリストに含まれているか確認
  if (AVAILABLE_MODELS.includes(model)) {
    clientModels.set(clientId, model);
    return true;
  }
  return false;
}

// 利用可能なモデル一覧を取得
function getAvailableModels(): string {
  return AVAILABLE_MODELS.join(", ");
}

// 会話履歴の取得
function getConversationHistory(
  clientId: string
): ChatCompletionMessageParam[] {
  // 会話履歴が存在しない場合は初期化
  if (!conversationHistories.has(clientId)) {
    initializeConversationHistory(clientId);
  }
  return conversationHistories.get(clientId) || [];
}

// 会話履歴の更新
function updateConversationHistory(
  clientId: string,
  role: "user" | "assistant",
  content: string
): void {
  const history = getConversationHistory(clientId);

  // 新しいメッセージを追加
  history.push({ role, content });

  // 履歴が最大数を超えた場合、古いものから削除（システムメッセージは保持）
  if (history.length > MAX_HISTORY + 1) {
    // システムメッセージを保持するため、インデックス1から削除
    history.splice(1, history.length - (MAX_HISTORY + 1));
  }

  // 更新された履歴を保存
  conversationHistories.set(clientId, history);
}

// 会話履歴のクリア
function clearConversationHistory(clientId: string): void {
  conversationHistories.delete(clientId);
}

// レスポンス型の定義
interface ResponseData {
  model: string;
  content: string;
}

// XMLビルダーの設定
const xmlBuilder = new XMLBuilder({
  format: true,
  ignoreAttributes: false,
});

// メッセージ処理関数
async function processMessage(
  clientId: string,
  message: string
): Promise<ResponseData> {
  try {
    // ユーザーメッセージを履歴に追加
    updateConversationHistory(clientId, "user", message);

    // 現在の会話履歴を取得
    const history = getConversationHistory(clientId);

    // クライアントの選択モデルを取得
    const model = getClientModel(clientId);

    // OpenAI APIにリクエスト送信（会話履歴を含む）
    const completion = await openai.chat.completions.create({
      model: model,
      messages: history,
    });

    // アシスタントの応答を取得
    const responseContent =
      completion.choices[0]?.message?.content ||
      "レスポンスがありませんでした。";

    // アシスタントの応答を履歴に追加
    updateConversationHistory(clientId, "assistant", responseContent);

    // モデル情報を含むレスポンスを返す
    return {
      model: model,
      content: responseContent,
    };
  } catch (error) {
    console.error("OpenAI API エラー:", error);
    // エラー時もモデル情報を含める
    return {
      model: getClientModel(clientId),
      content: `エラーが発生しました: ${
        error instanceof Error ? error.message : String(error)
      }`,
    };
  }
}

// TCPサーバーの作成
const server = net.createServer((socket) => {
  const clientId = getClientId(socket);
  console.log(`クライアント接続: ${clientId}`);

  // 新しいクライアント接続時に会話履歴を初期化
  initializeConversationHistory(clientId);

  // データ受信時の処理
  let buffer = "";
  socket.on("data", async (data) => {
    // データをUTF-8文字列として解釈
    const chunk = data.toString("utf-8");
    buffer += chunk;

    // メッセージの終端を検出（この例では改行文字を終端とする）
    if (chunk.includes("\n")) {
      const messages = buffer.split("\n");
      buffer = messages.pop() || ""; // 最後の不完全なメッセージを保持

      for (const message of messages) {
        if (message.trim()) {
          console.log(`受信メッセージ (${clientId}): ${message}`);

          // 特殊コマンドの処理
          const trimmedMessage = message.trim().toLowerCase();

          // 会話履歴クリアコマンド
          if (trimmedMessage === "/clear") {
            // 会話履歴をクリア
            clearConversationHistory(clientId);
            initializeConversationHistory(clientId);

            // XMLレスポンスを送信
            const xmlData = xmlBuilder.build({
              response: {
                type: "command",
                command: "clear",
                message: "会話履歴をクリアしました。",
              },
            });
            socket.write(`${xmlData}\n`);
            continue;
          }

          // モデル一覧表示コマンド
          if (trimmedMessage === "/models") {
            const currentModel = getClientModel(clientId);

            // XMLレスポンスを送信
            const xmlData = xmlBuilder.build({
              response: {
                type: "command",
                command: "models",
                current_model: currentModel,
                available_models: AVAILABLE_MODELS,
                message:
                  "モデルを変更するには '/model モデル名' と入力してください。",
              },
            });
            socket.write(`${xmlData}\n`);
            continue;
          }

          // モデル変更コマンド
          if (trimmedMessage.startsWith("/model ")) {
            const modelName = trimmedMessage.substring(7).trim();
            let success = false;
            let message = "";

            if (setClientModel(clientId, modelName)) {
              success = true;
              message = `モデルを '${modelName}' に変更しました。`;
            } else {
              message = `エラー: '${modelName}' は利用できないモデルです。利用可能なモデル: ${getAvailableModels()}`;
            }

            // XMLレスポンスを送信
            const xmlData = xmlBuilder.build({
              response: {
                type: "command",
                command: "model_change",
                success: success,
                model: modelName,
                message: message,
              },
            });
            socket.write(`${xmlData}\n`);
            continue;
          }

          // メッセージを処理してレスポンスを取得（クライアントIDを渡す）
          const responseData = await processMessage(clientId, message);

          // レスポンスをXML形式でクライアントに送信
          const xmlData = xmlBuilder.build({
            response: {
              model: responseData.model,
              content: responseData.content,
            },
          });
          socket.write(`${xmlData}\n`);
        }
      }
    }
  });

  // クライアント切断時の処理
  socket.on("end", () => {
    console.log(`クライアント切断: ${clientId}`);
    // クライアント切断時に会話履歴を削除（メモリリーク防止）
    clearConversationHistory(clientId);
  });

  // エラー発生時の処理
  socket.on("error", (err) => {
    console.error(`ソケットエラー (${clientId}):`, err);
  });
});

// サーバー起動
server.listen(PORT, HOST, () => {
  console.log(`TCP/IPサーバーが起動しました - ${HOST}:${PORT}`);
});

// サーバーエラー処理
server.on("error", (err) => {
  console.error("サーバーエラー:", err);
});
