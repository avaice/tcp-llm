FROM node:22-alpine

WORKDIR /app

# パッケージ依存関係をコピー
COPY package*.json ./

# 依存関係をインストール
RUN npm install

# ソースコードをコピー
COPY . .

# TypeScriptをコンパイル
RUN npm install -g tsx

# ポートを公開
EXPOSE 3000

# サーバーを起動
CMD ["npm", "start"]
