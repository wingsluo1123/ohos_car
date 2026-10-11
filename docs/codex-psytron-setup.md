# VSCode Codex 插件接入中转站（psytron / gpt-6.1-sol）配置指南

> 可复现 SOP。记录了让 VSCode 的 OpenAI 官方 Codex 插件（`openai.chatgpt` 扩展）在
> WSL2 + Remote-WSL 环境下接入 psytron.cn 中转站并使用 `gpt-6.1-sol` 模型的全过程、
> 关键坑位与验收方法。供其他 AI / 工程师在新机器上照做。
>
> 实测环境：Ubuntu 22.04 (WSL2)，2026-10-11。

## 1. 任务目标

让 VSCode Codex 插件的模型请求改走中转站（OpenAI 兼容 API），而不是 OpenAI 官方：

```json
{
  "name": "psytron",
  "provider": "psytron",
  "key": "sk-…",
  "url": "https://psytron.cn/v1"
}
```

目标模型：`gpt-6.1-sol`。
（注意：本文档**不包含**真实 API key；实际 key 见该机器的 `~/.codex/config.toml`。）

## 2. 关键事实（全部为实测结论，照做前必读）

1. **中转站只认"标准 Codex 客户端"**。裸 `curl` / 通用 HTTP 库发请求会被直接拒绝，
   报错：`我们检测到您的客户端存在异常，请使用标准 Codex 客户端请求…请首先尝试升级您的
   Codex 客户端版本`。因此：
   - **不能用 curl 验证中转站是否可用**（curl 的 4xx 不代表配置错误）；
   - 真正的端到端验证必须用**扩展内置的 codex 二进制**（其 UA/指纹受中转站信任）。
2. **`gpt-6.1-sol` 只支持 Responses 协议**（`/v1/responses`），不支持 chat completions
   （中转站返回 `protocol_not_supported`）。因此配置**必须** `wire_api = "responses"`。
3. **API key 可以不走环境变量**。codex 的 `model_providers` 支持 `http_headers`
   （静态请求头），可把 `Authorization: Bearer <KEY>` 直接写进 `~/.codex/config.toml`。
   不用标准 `env_key` 方案的原因——WSL2 + Remote-WSL 环境下环境变量很难注入到
   vscode-server 进程：
   - WSL 不读 `/etc/environment`（那是 PAM 机制，WSL 没有 PAM）；
   - `~/.bashrc` 只对交互式 shell 生效（vscode-server 是 WSL 直接拉起的非交互会话）；
   - Windows 侧环境变量需在 Windows 端 `setx`，而本机 WSL interop 未启用（`cmd.exe`
     不存在），WSL 内无法设置 Windows 环境变量。
   - 用 `http_headers` 内嵌后：终端 codex CLI 与 VSCode 插件读同一份配置，零环境变量依赖。
4. **扩展以 `app-server` 常驻模式运行 codex**：
   `codex -c features.code_mode_host=true app-server --analytics-default-enabled`。
   `config.toml` 只在该进程**启动时**读取 → **改完配置必须杀掉旧 app-server**（或
   Reload Window），否则一直跑旧配置。
5. **codex 运行时由扩展自带**，全局并没有安装 codex（`which codex` 为空是正常的）：
   - 本机 VSCode：`~/.vscode/extensions/openai.chatgpt-<ver>-<platform>/bin/linux-x86_64/codex`
   - Remote/WSL：`~/.vscode-server/extensions/openai.chatgpt-<ver>-<platform>/bin/linux-x86_64/codex`

## 3. 复现步骤（SOP）

### Step 0：验证 key 与模型（GET /models 不受客户端指纹限制，可安全用 curl）

```bash
curl -sS https://psytron.cn/v1/models -H "Authorization: Bearer <KEY>" | python3 -m json.tool
```

确认列表中有所需模型（如 `gpt-6.1-sol`）及其 `supported_endpoint_types`。

### Step 1：确定协议要求

```bash
curl -sS https://psytron.cn/v1/chat/completions \
  -H "Authorization: Bearer <KEY>" -H "Content-Type: application/json" \
  -d '{"model":"gpt-6.1-sol","messages":[{"role":"user","content":"hi"}],"max_tokens":16}'
```

- 返回 `protocol_not_supported` → 必须 `wire_api = "responses"`。
- `/v1/responses` 用 curl 测大概率被"客户端异常"拒绝，**属预期**，不代表不可用。

### Step 2：定位扩展内置 codex 二进制

```bash
ls -d ~/.vscode-server/extensions/openai.chatgpt-* ~/.vscode/extensions/openai.chatgpt-* 2>/dev/null
BIN=$(find ~/.vscode-server/extensions ~/.vscode/extensions \
        -path '*openai.chatgpt*/bin/*/codex' -type f 2>/dev/null | head -1)
"$BIN" --version   # 期望输出 codex-cli x.y.z
```

### Step 3：写配置

写入 `~/.codex/config.toml`（**若文件已存在，先读后合并**，保留用户已有配置）：

```toml
model = "gpt-6.1-sol"
model_provider = "psytron"

[model_providers.psytron]
name = "psytron"
base_url = "https://psytron.cn/v1"
wire_api = "responses"
http_headers = { Authorization = "Bearer <KEY>" }
```

```bash
chmod 600 ~/.codex/config.toml   # 目录 ~/.codex 默认已是 700
```

字段说明：
- `wire_api = "responses"`：该模型只支持 Responses 协议（事实 2）。
- `http_headers`：内嵌 key，绕开 WSL 环境变量难题（事实 3）。
  若目标环境不存在客户端指纹限制、且环境变量可正常注入（如纯 Linux 桌面），
  可改用标准方案：`env_key = "PSYTRON_API_KEY"` + 在 shell 配置里 `export`。

### Step 4：端到端验证（必须用 codex 二进制，curl 无效）

```bash
mkdir -p /tmp/codex-test && cd /tmp/codex-test
"$BIN" exec --strict-config --skip-git-repo-check "Reply with exactly: OK"
```

验收标准（三项全满足才算通过）：
1. 启动横幅显示 `model: gpt-6.1-sol`、`provider: psytron`；
2. 最终回复包含 `OK`（请求真实走通了中转站）；
3. `--strict-config` 未报错（所有配置字段被当前版本识别）。

### Step 5：让 VSCode 扩展生效

```bash
pkill -f 'codex.*app-server'    # 只杀扩展的 codex app-server 子进程
```

- 扩展下次打开 Codex 面板会自动拉起新的 app-server，读取新配置，用户无需额外操作。
- ⚠️ **不要**杀 vscode-server 本体（`code-server` / `out/server-main.js` 进程）：
  remote 场景下 AI 助手（如 AtomCode）可能就跑在 vscode-server 的 ptyHost 子进程树里，
  整棵杀了会断开当前会话。codex app-server 挂在 agentHost 下，单独杀掉是安全的。
- 若 Codex 面板显示离线：`Ctrl+Shift+P → Developer: Reload Window` 即可恢复。

## 4. 排错表

| 现象 | 原因 | 处理 |
|---|---|---|
| 面板模型不是 gpt-6.1-sol | UI 里的模型选择状态覆盖配置默认值 | 模型下拉框手动输入/选择 `gpt-6.1-sol` |
| 面板离线 / 连不上 | 旧 app-server 进程未重启，还在用旧配置 | `pkill -f 'codex.*app-server'` 或 Reload Window |
| `客户端存在异常，请使用标准 Codex 客户端` | 请求不是 codex 二进制发出的 | 放弃 curl 验证，用 Step 4 的内置二进制验证 |
| `protocol_not_supported` | `wire_api` 不对 | 改为 `wire_api = "responses"` |
| 配置字段报错 / 行为异常 | codex 版本差异 | 加 `--strict-config` 跑 Step 4 定位不受支持的字段 |
| `which codex` 找不到命令 | 正常：codex 由扩展自带 | 按 Step 2 定位二进制，或另装 codex CLI（同样读 `~/.codex/config.toml`） |

## 5. 安全注意

- key 明文存于 `~/.codex/config.toml`（文件 600、目录 700，仅属主可读写）。
- **不要把真实 key 写进仓库文件 / 文档 / git 提交**（本文档用 `<KEY>` 占位）。
- 换 key：改 `config.toml` 里 `http_headers` 一行 → 执行 Step 5 重启 app-server。

## 6. 本次实测记录（2026-10-11）

| 项 | 值 |
|---|---|
| 扩展 | `openai.chatgpt-26.1007.21434-linux-x64`（Remote-WSL，Windows 端 VSCode 连接） |
| 内置 codex | `codex-cli 0.162.0-alpha.17.2` |
| 中转站模型列表 | `gpt-6.1-sol`、`gpt-6-sol`、`gpt-5.6-sol`、`gpt-6-astra`、`gpt-image-*`、`claude-*` 等 |
| 协议 | 仅 Responses（chat completions 被拒：`protocol_not_supported`） |
| 验证结果 | `codex exec --strict-config` 走中转站成功，模型回复 `OK` |
| 最终配置 | `~/.codex/config.toml`（实际 key 在该文件内，权限 600） |
