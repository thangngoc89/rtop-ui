const webpack = require("webpack");
const HtmlWebpackPlugin = require("html-webpack-plugin");
const path = require("path");
const postcssPresetEnv = require("postcss-preset-env");

const outputDir = path.join(__dirname, "build/");

const isProd = process.env.NODE_ENV === "production";

const base = {
  entry: ["react-hot-loader/patch", "./entry.js"],
  mode: isProd ? "production" : "development",
  devServer: {
    hot: true,
    contentBase: path.join(__dirname, "public"),
    host: "0.0.0.0",
    port: 3000,
    historyApiFallback: true,
    disableHostCheck: true,
    proxy: {
      "/graphql": "http://localhost:8081/v1alpha1",
    },
  },
  output: {
    path: outputDir,
    publicPath: "/",
    filename: "[name].js",
    globalObject: "this",
  },
  node: {
    fs: "empty",
    child_process: "empty",
  },
  plugins: [
    new HtmlWebpackPlugin({
      template: path.join(__dirname, "src", "index.html"),
    }),
  ],
  module: {
    rules: [
      {
        test: file =>
          file.endsWith(".worker.js") || file.endsWith("Worker_Index.bs.js"),
        use: { loader: "worker-loader" },
      },
      {
        test: /\.css$/,
        use: [
          "style-loader",
          { loader: "css-loader" },
          {
            loader: "postcss-loader",
            options: {
              ident: "postcss",
              plugins: () => [
                postcssPresetEnv({
                  stage: 4,
                  features: {
                    "nesting-rules": true,
                  },
                }),
              ],
            },
          },
        ],
      },
    ],
  },
};

if (!isProd) {
  base.plugins = [...base.plugins, new webpack.HotModuleReplacementPlugin()];
}

module.exports = base;
