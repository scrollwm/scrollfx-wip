# Echo CLI - Technical Specification

**Version:** 0.1.0  
**Base:** Gemini CLI 0.9.0+  
**License:** Apache 2.0  
**Upstream Sync:** Weekly (Tuesday 23:59 UTC)

## Overview

Echo CLI is a fork of Google's Gemini CLI with three additive architectural patches:

1. **Multi-Provider Architecture** - Abstract LLM backend with support for OpenAI, Anthropic, Gemini, and OpenAI-compatible local models
2. **Nvim RPC Integration** - Direct Neovim buffer manipulation via msgpack-RPC protocol
3. **Native MCP Server** - Built-in Model Context Protocol server implementation

All three patches are **100% additive** with zero modifications to existing Gemini CLI files. This enables:
- Clean upstream synchronization
- Independent feature stability
- Modular testing and deployment

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         Echo CLI Core                            │
│                    (Gemini CLI 0.9.0 base)                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌────────────────────┐  ┌──────────────┐  ┌─────────────────┐ │
│  │  Provider Layer    │  │  Nvim RPC    │  │  Native MCP     │ │
│  │  (Patch 1)         │  │  (Patch 2)   │  │  (Patch 3)      │ │
│  └────────┬───────────┘  └──────┬───────┘  └────────┬────────┘ │
│           │                     │                    │           │
│           ├─────────────────────┴────────────────────┤           │
│           │         Tool Registry + PTY Layer        │           │
│           └──────────────────────────────────────────┘           │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
         │                      │                      │
         ▼                      ▼                      ▼
   [LLM Backends]        [Neovim Instance]      [MCP Clients]
   OpenAI, Anthropic     (via Unix socket)      Claude Desktop
   Local (Ollama/LMStudio)                      MCP Inspector
```

## Patch 1: Multi-Provider Architecture

### Technical Design

**Pattern:** Adapter + Strategy Pattern  
**LOC:** ~8,000 lines  
**Files:** 29 source files  
**Integration Point:** `ContentGenerator` interface

#### Component Hierarchy

```
ContentGenerator (Gemini CLI interface)
    ↓
ProviderContentGenerator (implements ContentGenerator)
    ↓
GeminiCompatibleWrapper (format translation boundary)
    ↓
IProvider (universal provider interface)
    ↓
    ├─ OpenAIProvider
    ├─ AnthropicProvider
    ├─ GeminiProvider
    └─ [Any OpenAI-compatible provider]
```

#### Key Components

##### GeminiCompatibleWrapper (765 LOC)
**Purpose:** Single format translation layer  
**Input:** Gemini CLI `Content[]` + `FunctionDeclaration[]`  
**Output:** Universal `IMessage[]` + `ITool[]`

Translation logic:
- Gemini `Content` → `IMessage` (role, parts, metadata)
- `FunctionDeclaration` → `ITool` (JSON schema normalization)
- Tool calls: Native format → Universal format → Provider-specific format

##### ProviderManager (145 LOC)
**Purpose:** Runtime provider selection and lifecycle management

```typescript
interface IProviderManager {
  registerProvider(provider: IProvider): void;
  setActiveProvider(name: string): void;
  getActiveProvider(): IProvider;
  listProviders(): string[];
}
```

##### IProvider Interface
**Contract:**
```typescript
interface IProvider {
  readonly name: string;
  readonly supportsStreaming: boolean;
  readonly supportsTools: boolean;
  
  generateContent(params: GenerateContentParams): Promise<GenerateContentResult>;
  streamContent(params: GenerateContentParams): AsyncIterableIterator<ContentChunk>;
  countTokens(messages: IMessage[]): Promise<number>;
}
```

##### ToolFormatter (306 LOC)
**Supported Formats:**
1. OpenAI function calling
2. Anthropic tool use
3. Gemini function declarations
4. DeepSeek text format
5. Qwen text format
6. Hermes function calling
7. Gemma text format
8. Llama text format

**Format Detection:** Automatic based on model identifier  
**Fallback:** Text-based tool calling with regex parsing

### Configuration

```bash
# Provider selection
export GEMINI_PROVIDER=openai|anthropic|gemini

# OpenAI (includes GPT-4, GPT-4 Turbo, GPT-4.1)
export OPENAI_API_KEY="sk-..."
export OPENAI_BASE_URL="https://api.openai.com/v1"  # Optional

# Anthropic (Claude 4 Opus, Sonnet)
export ANTHROPIC_API_KEY="sk-ant-..."

# Gemini (default)
export GOOGLE_API_KEY="..."

# Local LLMs (Ollama, LMStudio, etc.)
export GEMINI_PROVIDER=openai
export OPENAI_API_KEY="ollama"  # Dummy key
export OPENAI_BASE_URL="http://localhost:11434/v1"
```

### Dependencies

```json
{
  "@anthropic-ai/sdk": "^0.55.1",
  "openai": "^5.10.1",
  "@dqbd/tiktoken": "^1.0.21"
}
```

**Bundle Size:** ~4MB (lazy loaded)  
**Runtime Overhead:** <50ms provider initialization

### Integration Points

**Modified Files:** 2 (via patches)
- `packages/core/src/index.ts` - Export provider types
- `packages/core/src/content/ContentGenerator.ts` - Add `userPromptId` parameter

**New Files:** 29
- `packages/core/src/providers/` (18 files)
- `packages/core/src/parsers/` (2 files)
- `packages/core/src/tools/` (2 files, ToolFormatter additions)

## Patch 2: Nvim RPC Integration

### Technical Design

**Pattern:** RPC Client + Buffer Abstraction  
**LOC:** ~1,200 lines  
**Files:** 6 source files  
**Integration Point:** Gemini CLI tool registry

#### Protocol Stack

```
NvimBufferTool (Gemini CLI tool interface)
    ↓
BufferOperations (high-level buffer API)
    ↓
RpcClient (msgpack-RPC protocol)
    ↓
Unix Socket (/tmp/nvim.sock)
    ↓
Neovim Instance (msgpack-RPC server)
```

#### Key Components

##### RpcClient (400 LOC)
**Protocol:** Neovim msgpack-RPC  
**Transport:** Unix domain socket

Message types:
```typescript
enum RpcMessageType {
  REQUEST = 0,    // [0, msgid, method, params]
  RESPONSE = 1,   // [1, msgid, error, result]
  NOTIFICATION = 2 // [2, method, params]
}
```

**Features:**
- Request/response correlation via message ID
- Automatic reconnection
- Timeout handling (configurable)
- Event-driven architecture (EventEmitter)

##### BufferOperations (350 LOC)
**High-level API:**
```typescript
class BufferOperations {
  async readBuffer(path: string): Promise<string>
  async writeBuffer(path: string, content: string): Promise<void>
  async replaceLines(path: string, start: number, end: number, lines: string[]): Promise<void>
  async getBufferInfo(path: string): Promise<BufferInfo>
  async saveBuffer(path: string): Promise<void>
}
```

**Buffer Resolution:**
1. Search existing buffers by normalized path
2. Create new buffer if not found
3. Load file content via `:edit` command
4. Return buffer handle (integer)

##### DiffApplication (300 LOC)
**Supported Formats:**

1. **Unified Diff** (standard `diff -u` output)
   ```diff
   @@ -1,3 +1,3 @@
    function hello() {
   -  console.log("old");
   +  console.log("new");
    }
   ```

2. **Semantic Diff** (AI-friendly format)
   ```
   function hello() {
     console.log("old");
   }
   <<<<<<<<<<
   function hello() {
     console.log("new");
   }
   ```

**Application Strategy:**
- Parse diff into hunks/blocks
- Find matching lines in buffer (fuzzy matching with trim)
- Apply changes via `nvim_buf_set_lines`
- Track line offset for subsequent changes
- Mark buffer as modified

##### NvimBufferTool (150 LOC)
**Tool Schema:**
```typescript
{
  name: 'nvim_buffer',
  description: 'Manipulate nvim buffers via RPC for direct code editing',
  parameters: {
    action: 'read' | 'write' | 'apply_diff' | 'get_info',
    path: string,
    content?: string,
    diff_format?: 'unified' | 'semantic'
  }
}
```

**Execution Flow:**
1. Ensure RPC connection
2. Route action to appropriate handler
3. Execute buffer operation
4. Return formatted result to LLM
5. Include diagnostic data for debugging

### Configuration

```bash
# Socket path (auto-detected if exists)
export NVIM_SOCKET=/tmp/nvim.sock

# Timeouts (optional)
export NVIM_CONNECTION_TIMEOUT=5000  # ms
export NVIM_EXECUTION_TIMEOUT=30000  # ms
```

### Starting Neovim with RPC

**Option 1: Command-line**
```bash
nvim --listen /tmp/nvim.sock
```

**Option 2: init.lua**
```lua
vim.fn.serverstart('/tmp/nvim.sock')
```

**Option 3: Auto-start**
```lua
local socket_path = '/tmp/nvim.sock'
if vim.fn.filereadable(socket_path) == 0 then
  vim.fn.serverstart(socket_path)
end
```

### Dependencies

```json
{
  "@msgpack/msgpack": "^3.0.0"
}
```

**Bundle Size:** ~100KB  
**Runtime Overhead:** <10ms per RPC call

### Integration Points

**Modified Files:** 1
- `packages/cli/src/index.ts` - Register NvimBufferTool in tool registry

**New Files:** 6
- `packages/cli/src/tools/nvim-rpc/` (all tool implementation)

### Security Considerations

**Socket Access:** Tool has full control over Neovim instance
- Can execute arbitrary vim commands
- Can read/write any buffer
- Can access files with nvim's permissions

**Recommendations:**
1. Use socket in protected directory (`~/.config/nvim/`)
2. Set restrictive permissions: `chmod 600 /tmp/nvim.sock`
3. Never expose socket over network
4. Use SSH tunneling for remote access

## Patch 3: Native MCP Server

### Technical Design

**Pattern:** Direct Inheritance (Backlog.md pattern)  
**LOC:** ~3,500 lines  
**Files:** 18 source files + 4 patches  
**Integration Point:** New CLI command + core abstraction

#### Architecture Layers

```
┌─────────────────────────────────────────────┐
│          MCP Clients                         │
│  (Claude Desktop, MCP Inspector, etc.)       │
└────────────────┬────────────────────────────┘
                 │
                 │ stdio / SSE
                 │
┌────────────────▼────────────────────────────┐
│        EchoMcpServer                         │
│    (extends EchoCore)                        │
│  ┌──────────────────────────────────────┐   │
│  │  MCP Protocol Handler                │   │
│  │  - initialize                        │   │
│  │  - tools/list, tools/call           │   │
│  │  - resources/list, resources/read    │   │
│  └──────────────┬───────────────────────┘   │
└─────────────────┼───────────────────────────┘
                  │
┌─────────────────▼───────────────────────────┐
│           EchoCore                           │
│  ┌──────────────────────────────────────┐   │
│  │  ToolRegistry (wrapper)              │   │
│  │  SecurityPolicy                      │   │
│  │  ValidationEngine                    │   │
│  └──────────────┬───────────────────────┘   │
└─────────────────┼───────────────────────────┘
                  │
┌─────────────────▼───────────────────────────┐
│      Gemini CLI Tool System                  │
│  (file ops, shell, git, web, etc.)          │
└─────────────────────────────────────────────┘
```

#### Key Components

##### EchoCore (500 LOC)
**Purpose:** Core business logic abstraction

```typescript
class EchoCore {
  protected toolRegistry: ToolRegistry;
  protected securityPolicy: SecurityPolicy;
  protected validationEngine: ValidationEngine;
  
  async executeTool(
    toolName: string,
    args: Record<string, any>,
    context: ExecutionContext
  ): Promise<ToolResult>
  
  async listTools(): Promise<ToolDefinition[]>
  async validateInput(tool: string, args: any): Promise<ValidationResult>
  async checkPermission(tool: string, args: any): Promise<PolicyDecision>
}
```

**Key Design Decision:** No modifications to existing Gemini CLI tools. All tools accessed via wrapper.

##### EchoMcpServer (400 LOC)
**Inheritance:** `extends EchoCore`  
**Pattern:** Direct inheritance (Backlog.md native server pattern)

```typescript
class EchoMcpServer extends EchoCore {
  private server: Server;  // @modelcontextprotocol/sdk
  
  async start(transport: 'stdio' | 'sse'): Promise<void>
  
  // MCP Protocol Handlers
  private handleInitialize()
  private handleToolsList()
  private handleToolsCall()
  private handleResourcesList()
  private handleResourcesRead()
}
```

**Transports:**
- **stdio:** Standard MCP transport for local clients (Claude Desktop)
- **SSE:** Server-Sent Events for HTTP bridges (MCP Context Forge)

**Zero Translation:** Direct tool calls, no format conversion between MCP and Gemini CLI

##### ToolRegistry (200 LOC)
**Purpose:** Wrapper around Gemini CLI tool system

```typescript
class ToolRegistry {
  private tools: Map<string, ToolDefinition>;
  
  async listTools(): Promise<ToolDefinition[]>
  async getTool(name: string): Promise<ToolDefinition | undefined>
  async executeTool(name: string, args: any): Promise<any>
  async getToolSchema(name: string): Promise<JSONSchema>
}
```

**Tool Discovery:** Scans Gemini CLI tool registry at startup  
**Schema Generation:** Converts tool definitions to JSON Schema for MCP

##### SecurityPolicy (150 LOC)
**Modes:**
- `interactive`: Prompts user before dangerous operations
- `autonomous`: Full automation, no prompts

**Policies:**
```typescript
interface PolicyDecision {
  allowed: boolean;
  requiresConfirmation: boolean;
  reason?: string;
}
```

**Dangerous Operations:**
- File writes outside project directory
- Shell command execution
- Network requests
- Git operations (push, force operations)

##### ValidationEngine (200 LOC)
**Input Validation:** JSON Schema-based validation

```typescript
interface ValidationResult {
  valid: boolean;
  errors?: ValidationError[];
}

class ValidationEngine {
  async validate(schema: JSONSchema, data: any): Promise<ValidationResult>
  async sanitize(data: any): any
}
```

**Validation Rules:**
- Type checking
- Required fields
- Value constraints (min/max, patterns)
- Path traversal prevention
- Command injection prevention

##### Resource System
**URI Scheme:** `echo://workflow/*`

**Available Resources:**
1. `echo://workflow/overview` - Server capabilities and workflow guide
2. `echo://workflow/security` - Security policies and modes
3. `echo://workflow/tools-reference` - Complete tool documentation
4. `echo://workflow/best-practices` - Usage guidelines for agents

**Format:** Markdown documentation  
**Purpose:** Agent guidance and self-documentation

### CLI Command

**Binary:** `echo-mcp-server` (or `echo mcp`)

**Usage:**
```bash
# Stdio transport (default)
echo mcp start

# SSE transport
echo mcp start --transport=sse --port=3000

# Security mode
echo mcp start --mode=interactive
echo mcp start --mode=autonomous

# Custom project root
echo mcp start --project-root=/path/to/project
```

**Process Management:**
- Graceful shutdown on SIGINT/SIGTERM
- Automatic reconnection handling
- Health check endpoint (SSE only)

### Configuration

**Environment Variables:**
```bash
# Security mode
export ECHO_MCP_MODE=interactive|autonomous

# Project root (defaults to cwd)
export ECHO_PROJECT_ROOT=/path/to/project

# Logging
export DEBUG=echo:mcp:*
```

**Claude Desktop Config:**
```json
{
  "mcpServers": {
    "echo-cli": {
      "command": "echo",
      "args": ["mcp", "start"],
      "env": {
        "ECHO_MCP_MODE": "interactive"
      }
    }
  }
}
```

**MCP Context Forge Config:**
```yaml
servers:
  - name: echo-cli
    transport: sse
    url: http://localhost:3000/sse
```

### Dependencies

```json
{
  "@modelcontextprotocol/sdk": "^1.11.0"
}
```

**Note:** Already present in Gemini CLI 0.9.0, no additional installation needed

**Bundle Size:** ~500KB  
**Runtime Overhead:** <20ms per MCP request

### Integration Points

**Modified Files:** 3 (via patches)
- `packages/core/src/index.ts` - Export EchoCore and EchoMcpServer
- `packages/cli/src/index.ts` - Register MCP command
- `packages/cli/package.json` - Add echo-mcp-server binary

**New Files:** 18
- `packages/core/src/core/` (5 files - EchoCore abstraction)
- `packages/core/src/mcp/` (13 files - MCP server implementation)

### MCP Protocol Compliance

**Implemented Methods:**
- `initialize` - Server initialization with capabilities
- `tools/list` - List all available tools
- `tools/call` - Execute tool with arguments
- `resources/list` - List available resources
- `resources/read` - Read resource content

**Capabilities:**
```typescript
{
  tools: true,
  resources: true,
  prompts: false,  // Not implemented
  sampling: false  // Not implemented
}
```

**Tool Discovery:** Dynamic based on Gemini CLI tool registry  
**Schema Format:** JSON Schema draft-07

## Build System

### Directory Structure

```
echo-cli/
├── packages/
│   ├── core/
│   │   ├── src/
│   │   │   ├── providers/      # Patch 1
│   │   │   ├── parsers/        # Patch 1
│   │   │   ├── core/           # Patch 3
│   │   │   ├── mcp/            # Patch 3
│   │   │   └── [existing]
│   │   └── package.json
│   └── cli/
│       ├── src/
│       │   ├── tools/
│       │   │   └── nvim-rpc/   # Patch 2
│       │   ├── commands/
│       │   │   └── mcp.ts      # Patch 3
│       │   └── [existing]
│       └── package.json
├── scripts/
│   ├── apply-all-echo-patches.sh
│   └── update-homebrew-formula.sh
├── .github/
│   └── workflows/
│       └── weekly-echo-release.yml
└── [existing gemini-cli files]
```

### Build Process

**Standard Build:**
```bash
npm install
npm run build
```

**Output:**
- `packages/core/dist/` - Compiled TypeScript
- `packages/cli/dist/` - CLI application

**Build Time:** ~30-60 seconds

### Patching Process

**Automated:**
```bash
./scripts/apply-all-echo-patches.sh gemini-cli-upstream echo-cli-build
```

**Steps:**
1. Copy upstream Gemini CLI
2. Apply provider extraction (29 files + patches)
3. Apply MCP native integration (18 files + patches)
4. Apply Nvim RPC integration (6 files + registration)
5. Install dependencies
6. Build packages

**Total Time:** 2-5 minutes

## Testing

### Unit Tests

**Provider Layer:**
```bash
cd packages/core
npm test -- providers
```

**Test Coverage:**
- Provider initialization
- Message format conversion
- Tool format conversion
- Token counting
- Streaming

**Nvim RPC:**
```bash
# Start test nvim instance
nvim --listen /tmp/nvim-test.sock

# Run tests
NVIM_TEST_SOCKET=/tmp/nvim-test.sock npm test -- nvim-rpc
```

**Test Coverage:**
- RPC connection
- Buffer operations (read/write)
- Diff application (unified/semantic)
- Error handling
- Timeout behavior

**MCP Native:**
```bash
npm test -- mcp
```

**Test Coverage:**
- MCP protocol handlers
- Tool discovery
- Resource system
- Security policies
- Validation engine

### Integration Tests

**End-to-End Test:**
```bash
# Test all three patches together
./test/e2e/full-integration.sh
```

**Scenarios:**
1. OpenAI provider + Nvim RPC + MCP server
2. Local LLM + tool execution + security policy
3. Provider switching + buffer operations + resource access

### Manual Testing

**Provider Test:**
```bash
export GEMINI_PROVIDER=openai
export OPENAI_BASE_URL=http://localhost:11434/v1
export OPENAI_API_KEY=ollama
echo --prompt "List files in current directory"
```

**Nvim RPC Test:**
```bash
nvim --listen /tmp/nvim.sock test.txt &
export NVIM_SOCKET=/tmp/nvim.sock
echo
# In CLI: "Read test.txt and add a comment at the top"
```

**MCP Test:**
```bash
echo mcp start &
npx @modelcontextprotocol/inspector
# Connect via stdio: echo mcp start
# Test: tools/list, tools/call, resources/list
```

## Distribution

### GitHub Releases

**Format:** `v{gemini-version}-echo.{YYYYMMDD}`  
**Example:** `v0.9.0-echo.20251023`

**Assets:**
- Source tarball (`.tar.gz`)
- npm package (`.tgz`)
- Release notes (auto-generated)

**Automation:** GitHub Actions, Tuesday 23:59 UTC

### Homebrew

**Tap:** `mecattaf/echo`

**Installation:**
```bash
brew tap mecattaf/echo
brew install echo-cli
```

**Binaries Installed:**
- `gemini` - Main CLI (compatibility)
- `echo` - Main CLI (branded)
- `echo-mcp` - MCP server standalone

**Dependencies:** `node@20`

### Manual Installation

**Linux / macOS:**
```bash
curl -fsSL https://raw.githubusercontent.com/mecattaf/echo-cli/main/install.sh | bash
```

**Installation Location:** `~/.echo-cli`  
**Binaries:** `~/.local/bin/`

### npm Package

**Package Name:** `@mecattaf/echo-cli` (planned)

**Installation:**
```bash
npm install -g @mecattaf/echo-cli
```

## Upstream Synchronization

### Weekly Process

**Schedule:** Every Tuesday 23:59 UTC (after Gemini CLI preview release)

**Automation Steps:**
1. Clone latest Gemini CLI nightly
2. Apply all three patches via unified script
3. Install dependencies
4. Run build
5. Run tests
6. Create GitHub release (if build succeeds)
7. Update Homebrew formula
8. Create issue (if build fails)

**Failure Handling:**
- Build failures create GitHub issue with logs
- Manual intervention required to fix patches
- Re-run workflow after fixes

### Patch Maintenance

**When Gemini CLI Changes Break Patches:**

1. **Provider Layer:** Update `GeminiCompatibleWrapper` if `ContentGenerator` interface changes
2. **Nvim RPC:** Likely unaffected (independent tool)
3. **MCP Native:** Update `EchoCore` if tool registry API changes

**Testing After Sync:**
```bash
# Automated in GitHub Actions
npm run build
npm test
```

## Performance Characteristics

### Startup Time

**Cold Start:**
- Gemini CLI base: ~200ms
- + Provider layer: +50ms
- + Nvim RPC: +10ms (if socket exists)
- + MCP server: +20ms (if started)
- **Total:** ~280ms

**Warm Start (cached):** ~150ms

### Runtime Overhead

**Provider Layer:**
- Message conversion: <5ms per request
- Tool format conversion: <10ms per tool
- Token counting: <50ms per request

**Nvim RPC:**
- RPC call: <10ms per operation
- Buffer read: <20ms (typical file)
- Diff application: <100ms (typical diff)

**MCP Native:**
- MCP request: <20ms per request
- Tool execution: Variable (depends on tool)
- Resource read: <5ms (cached)

### Memory Usage

**Base:** ~50MB (Gemini CLI)  
**+ Provider layer:** +20MB (SDK lazy loaded)  
**+ Nvim RPC:** +5MB  
**+ MCP server:** +10MB  
**Total:** ~85MB

### Network Usage

**Provider Layer:**
- OpenAI API: ~1-10KB per request (depends on context)
- Anthropic API: Similar to OpenAI
- Local LLM: LAN traffic only

**Nvim RPC:** No network (Unix socket)  
**MCP Native:** No network (stdio) or localhost only (SSE)

## Compatibility Matrix

| Feature | Gemini CLI | PTY Layer | Provider Layer | Nvim RPC | MCP Native |
|---------|-----------|-----------|----------------|----------|------------|
| File operations | ✅ | ✅ | ✅ | ✅ | ✅ |
| Shell execution | ✅ | ✅ | ✅ | N/A | ✅ |
| Tool calling | ✅ | ✅ | ✅ | N/A | ✅ |
| Streaming | ✅ | ✅ | ✅ | N/A | ✅ |
| Conversation history | ✅ | N/A | ✅ | N/A | N/A |
| MCP client mode | ✅ | N/A | ✅ | N/A | N/A |
| MCP server mode | ❌ | N/A | N/A | N/A | ✅ |

**Independence:** All three patches operate independently. Failure in one doesn't affect others.

## Security Model

### Threat Model

**Attack Surfaces:**
1. LLM prompt injection → Tool execution
2. Nvim RPC socket → Arbitrary vim commands
3. MCP clients → File system access

### Mitigations

**Provider Layer:**
- Input sanitization before tool execution
- Tool call validation against schema
- Rate limiting (provider-dependent)

**Nvim RPC:**
- Socket permission checks (600 recommended)
- Path validation (prevent traversal)
- No network exposure (Unix socket only)

**MCP Native:**
- Security policy enforcement (interactive/autonomous)
- Validation engine (JSON Schema)
- Resource access controls
- Dangerous operation flagging

### Audit Logging

**Enabled by:**
```bash
export DEBUG=echo:*
```

**Logged Events:**
- Provider selection/switching
- Tool executions
- Nvim RPC operations
- MCP requests
- Security policy decisions

## Development

### Prerequisites

- Node.js >= 20
- npm >= 10
- TypeScript >= 5.3.3
- Git

### Setup

```bash
git clone https://github.com/mecattaf/echo-cli.git
cd echo-cli
npm install
npm run build
```

### Development Workflow

**File Watching:**
```bash
cd packages/core
npm run build -- --watch
```

**Testing Single Patch:**
```bash
# Test provider layer only
npm test -- providers

# Test with specific provider
GEMINI_PROVIDER=openai npm test

# Test Nvim RPC
NVIM_TEST_SOCKET=/tmp/nvim-test.sock npm test -- nvim-rpc

# Test MCP
npm test -- mcp
```

### Adding New Providers

1. Implement `IProvider` interface
2. Add to `ProviderManager` registration
3. Update `ToolFormatter` if custom tool format needed
4. Add tests
5. Update documentation

**Example:**
```typescript
class CustomProvider implements IProvider {
  readonly name = 'custom';
  readonly supportsStreaming = true;
  readonly supportsTools = false;
  
  async generateContent(params: GenerateContentParams): Promise<GenerateContentResult> {
    // Implementation
  }
  
  // ... other methods
}
```

### Debugging

**Provider Layer:**
```bash
DEBUG=echo:provider:* echo --prompt "test"
```

**Nvim RPC:**
```bash
DEBUG=echo:nvim:* echo
```

**MCP Native:**
```bash
DEBUG=echo:mcp:* echo mcp start
```

## Known Limitations

### Provider Layer
- Token counting accuracy varies by provider
- Some providers don't support all tool formats
- Local LLM tool support depends on model capabilities

### Nvim RPC
- Requires Neovim 0.5+ (msgpack-RPC support)
- Single socket = single nvim instance
- No support for vim (only neovim)
- Diff application can fail with significant whitespace differences

### MCP Native
- No prompts support (MCP capability)
- No sampling support (MCP capability)
- Limited to tools available in Gemini CLI
- Security policy requires manual configuration

## Roadmap

### Short-term (v0.2.x)
- [ ] Additional providers (Cohere, Mistral)
- [ ] Nvim RPC connection pooling
- [ ] MCP prompts support
- [ ] Enhanced security policies

### Medium-term (v0.3.x)
- [ ] Docker distribution
- [ ] MCP sampling support
- [ ] Performance optimizations
- [ ] Comprehensive test suite

### Long-term (v1.0.x)
- [ ] Plugin system
- [ ] Web UI for configuration
- [ ] Cloud deployment options
- [ ] Enterprise features

## Support

### Issue Tracking
**Repository:** https://github.com/mecattaf/echo-cli  
**Issues:** https://github.com/mecattaf/echo-cli/issues

### Documentation
- `/docs/providers.md` - Provider configuration guide
- `/docs/nvim-rpc.md` - Nvim RPC detailed documentation
- `/docs/mcp-native.md` - MCP server configuration guide
- `/docs/troubleshooting.md` - Common issues and solutions

### Community
- GitHub Discussions: https://github.com/mecattaf/echo-cli/discussions
- Discord: (TBD)

## License

Apache License 2.0

Same as Gemini CLI and LLxprt Code.

## Contributing

See `CONTRIBUTING.md` for guidelines.

**Key Points:**
- All patches must remain 100% additive
- Maintain independence between patches
- Add tests for new features
- Update documentation
- Follow existing code style

## Acknowledgments

- **Gemini CLI:** Base framework and tool system
- **LLxprt Code:** Provider architecture inspiration
- **Neovim:** RPC protocol and buffer manipulation
- **Anthropic MCP:** Protocol specification and SDK

---

**Echo CLI** - Multi-provider LLM CLI with Neovim integration and native MCP support.
