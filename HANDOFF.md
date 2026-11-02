# HANDOFF — Xreader Pro

> Documento de continuidade. Estado capturado em: **2026-11-01**, HEAD `0e9e1bb`, working tree **limpa**, `master` sincronizada com `origin`.
> Repo: `https://github.com/yuri-schmaltz/xreader_pro` (fork de `linuxmint/xreader`).

---

## 1. Objetivo do projeto

Fazer do fork `xreader` um produto próprio chamado **Xreader Pro**:
GUI refinada (abas, menus, sidebar, statusbar), app **100% pt-BR**, e branding
consistente ("Xreader Pro") em todo o ponto de contato com o usuário.

## 2. O que já está pronto (commits enviados)

| Commit | Conteúdo |
|---|---|
| `a857947` | GUI do EvTabbedWindow refinada |
| `053e004` | **Fix das abas**: largura normal mostrando títulos, encolhem até mínimo conforme a barra enche (não colapsam em `...`) |
| `b592e82` | Diálogo Sobre com logo `xreader-pro`, nome "Xreader Pro", website do fork |
| `7b470ff` | Versão **1.0.0** + tag `v1.0.0` (enviada) |
| `fc44a6a` | `--version` → "Xreader Pro 1.0.0" |
| `304a112` | **Rename de packaging**: binário, projeto meson, paths, `.pc`, desktop, man, ícones, gettext → `xreader-pro` |
| `5792253` | Desktop `Name=Xreader Pro` + appdata migrado para o fork |
| `0e9e1bb` | Notas de release do appdata apontam para `yuri-schmaltz/xreader_pro` |

**Validações:** 15/15 testes (`meson test -C build`), build estrito `-Werror` limpo,
app roda pt-BR, ícone resolve, daemon D-Bus funciona, appdata XML válido (`xmllint`).

## 3. Decisões que NÃO devem ser reabertas sem combinar

- **Escopo do rename: "Packaging/branding".** Renomeado: binário, projeto meson,
  paths de instalação, `.pc`, desktop, man, ícones, appdata, domínio gettext.
  **Mantido de propósito** (encanamento interno invisível ao usuário):
  - GSettings schema `org.x.reader` (~250 refs no código — renomear perde settings)
  - D-Bus `org.x.reader.Application/Window/Daemon` + paths
  - daemon `xreaderd`, helpers `xreader-previewer`/`xreader-thumbnailer`
  - sufixo de backends `.xreader-backend`, gresource `/org/x/reader/shell`
- **Versão 1.0.0** como primeira release do Xreader Pro; `api_version=1.5` e
  `binary_version=3.0.0` são fixos/independentes (bump de versão é só cosmético).
- App em **pt-BR**; `GETTEXT_PACKAGE=xreader-pro` (o `.mo` é `xreader-pro.mo`).
- Usuário **não interpreta imagens** — qualquer screenshot deve ser descrito em texto.

## 4. Migração para system-wide — CONCLUÍDA (1 pendência de permissão)

Migração de `~/.local` → **`/usr/local`** executada. O usuário rodou
`sudo ninja -C build install` (o agente não digita senha).

### Itens concluídos
1. **Validação system-wide: OK** —
   - `which xreader-pro` → **`/usr/local/bin/xreader-pro`** (vence no PATH),
     `--version` → "Xreader Pro 1.0.0"
   - desktop `Name=Xreader Pro`, `Exec/Icon/TryExec=xreader-pro`
   - handler padrão: `xdg-mime query default application/pdf` → `xreader-pro.desktop`
   - schemas, backends (`libpdfdocument.so` etc.), libs (`libxreaderview.so.3`),
     `.pc`, man, appdata, `xreader.thumbnailer` — todos em `/usr/local`
   - D-Bus service `Exec=/usr/local/libexec/xreaderd`; `XDG_DATA_DIRS` já contém
     `/usr/local/share` → auto-ativador funciona nativamente
2. **Limpeza de `~/.local`: OK** — prefixo antigo **totalmente vazio** de `*xreader*`
   (binário, helpers, `xreaderd`, desktop, appdata, man, ícones, **todos** os
   `.mo` de todos os idiomas, D-Bus service, schemas, `share/xreader-pro/`,
   backends, libs `libx{readerview,readerdocument}*`, `.pc`, `include/`,
   `xreader.thumbnailer`). O `gschemas.compiled` de `~/.local` era exclusivo do
   xreader (dir de schemas ficou vazio → nenhum outro app afetado).
3. **Teste ponta a ponta: OK** — app **RUNNING** de `/usr/local/bin/xreader-pro`,
   abrindo PDFs com libs de `/usr/local/lib/...`, `LANGUAGE=pt_BR` no ambiente,
   daemon `org.x.reader.Daemon` reativou; `mimeinfo.cache` system-wide existe.

### ⚠️ PENDÊNCIA ÚNICA — permissão do locale (pt-BR quebrado)
`/usr/local/share/locale` instalou como **`drwx------` (root)** → o app não lê
`/usr/local/share/locale/pt_BR/LC_MESSAGES/xreader-pro.mo` e **cai em inglês**
(testado: `gettext.translation(...)` → `FileNotFoundError`).

Correção (exige sudo):

```bash
sudo chmod -R a+rX /usr/local/share/locale
```

Depois: relançar e confirmar pt-BR. As demais permissões de `/usr/local` estão
corretas (755 dirs / 644 arquivos).

## 5. Armadilhas conhecidas (já diagnosticadas)

- **Daemon stale `xreaderd`:** `pkill -x xreader-pro` **não mata** o `xreaderd`
  (outro nome de processo). O daemon fica com estado stale e a **próxima** instância
  entrega o documento e **fecha sem abrir janela**. Sintoma no log:
  `GLib-GObject-CRITICAL ... invalid unclassed pointer in cast to 'TotemScrsaver'`
  (é sintoma de teardown, não a causa). **Correção:** `pkill xreaderd` e relançar.
  Edge case de robustez do daemon upstream; **oferta pendente**: corrigir no código
  detectando cliente morto via D-Bus (o usuário ainda não decidiu).
- **Rodar SEM instalar** (build dir): precisa de
  `XREADER_BACKEND_DIR=.../build/backend GSETTINGS_SCHEMA_DIR=<dir com gschemas.compiled>`
  senão: `GLib-GIO-ERROR: Settings schema 'org.x.reader' not found`.
  Recriar os schemas com `glib-compile-schemas --targetdir=/tmp/opencode/schemas data/`
  se `/tmp/opencode` foi limpo.
- **Aviso inofensivo** `Error registering document: GDBus.Error:...ServiceUnknown`
  quando não há daemon.
- Para **matar tudo**: `pkill -x xreader-pro; pkill xreaderd`.

## 6. Notas de ambiente

- `PATH` do usuário: `~/.local/bin` aparece **antes** de `/usr/local/bin`, mas o
  prefixo antigo foi **totalmente limpo** (§4) → `which xreader-pro` já retorna
  `/usr/local/bin/xreader-pro`. Se algum dia reinstalar em `~/.local`, ele venceria.
- `XDG_DATA_DIRS` = `...:/usr/local/share:/usr/share` → **contém `/usr/local/share`**
  (e NÃO contém `~/.local/share`, por isso o D-Bus em `~/.local` era frágil).
- App de teste: `/tmp/opencode/test-docs/doc1..3.pdf` (multi-página).
  Se `/tmp/opencode` sumir, gerar PDFs novos (ex.: `python3` + reportlab ou `enscript|ps2pdf`).
- Rodar o app em background com `nohup`/`setsid` — é o preferido do usuário para
  testes visuais.

## 7. Rust (avaliação pedida — conclusão)

O usuário pediu para **avaliar tornar o app 100% Rust**. Veredito dado:
**não vale a pena reescrever.** Escala: ~96.8k linhas de `.c` (110k C+headers,
322 arquivos), GUI GTK3/GObject (418 refs), 9 backends C via `dlopen` com libs
externas sem bindings Rust maduros (poppler-glib, libspectre, ddjvuapi, libgxps,
kpathsea…). Estimativa de paridade: 12–24 meses + perda de sincronia com upstream.
**Alternativa recomendada (se o interessar):** ilha FFI incremental — features novas
em Rust ligadas via `cbindgen`, mantendo o C estável. Nenhuma ação tomada.

## 8. Comandos rápidos

```bash
# build + testes
cd ~/Documentos/xreader
ninja -C build && meson test -C build          # 15/15 esperado
meson setup build-strict --reconfigure -Dprefix=/usr/local && ninja -C build-strict   # -Werror

# rodar a partir do build (sem instalar)
GSETTINGS_SCHEMA_DIR=/tmp/opencode/schemas \
XREADER_BACKEND_DIR=$PWD/build/backend \
./build/shell/xreader-pro --tabbed /tmp/opencode/test-docs/doc1.pdf

# rodar instalado (após migração)
setsid nohup xreader-pro --tabbed doc.pdf >/tmp/opencode/xr.log 2>&1 </dev/null & disown
```

## 9. Arquivos-chave

- `meson.build` — `project('xreader-pro', version: '1.0.0')`, paths/gettext via
  `meson.project_name()`, seção `.pc`
- `shell/meson.build` — executável `'xreader-pro'`, `install_rpath`
- `shell/main.c` — i18n, `--version`, icon-name `xreader-pro`
- `shell/ev-application.c` — spawn `BINDIR/xreader-pro`, Sobre, GSettings/D-Bus
- `shell/ev-daemon.c` — `spawn_xreader` → `BINDIR/xreader-pro`, daemon `org.x.reader.Daemon`
- `shell/ev-tabbed-window.c` — GUI (menubar, abas, sidebar, statusbar)
- `data/xreader-pro.desktop.in.in`, `data/xreader-pro.appdata.xml.in`,
  `data/xreader-pro.1`, `data/icons/` — superfície de empacotamento
- `xreader-pro-document.pc.in`, `xreader-pro-view.pc.in`
- **Mantidos** (namespace preservado): `data/org.x.reader.gschema.xml`,
  `data/org.x.reader.Daemon.service.in`

## 10. Estado do app em execução

Instância de teste rodando de **`/usr/local/bin/xreader-pro`** (migração concluída).
Fechar com `pkill -x xreader-pro; pkill xreaderd` se necessário.

## 11. Próximos passos (ordem sugerida)

1. **[do usuário]** `sudo chmod -R a+rX /usr/local/share/locale` → reativa o
   **pt-BR** (§4, pendência única da migração).
2. **[agente]** Confirmar pt-BR após o chmod (relançar e checar strings da UI).
3. Commitar/push (o `HANDOFF.md` pode trazer atualizações; working tree limpa
   além disso).
4. **[opcional, pedido pendente]** Corrigir o daemon stale no código (D-Bus
   NameOwnerChanged) — melhorou a robustez; usuário ainda não aprovou.
5. **[opcional]** Entrada `<releases>` 1.0.0 no appdata (a seção ainda lista 4.7/4.8).
