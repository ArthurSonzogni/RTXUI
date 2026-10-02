# Coding Agents

RTXUI's templates resemble HTML, Vue and browser CSS closely enough that a
language model writes them from habit, borrowing syntax RTXUI does not have.
Three things keep an agent on track.

## Give It the Manual

- [`llms.txt`](https://arthursonzogni.github.io/RTXUI/llms.txt) indexes the
  documentation.
- [`llms-full.txt`](https://arthursonzogni.github.io/RTXUI/llms-full.txt) is
  the whole manual as one Markdown file, opening with the rules models most
  often get wrong.

## Install the Skill

The RTXUI skill holds the supported syntax and the steps to check a change.
In Claude Code:

```text
/plugin marketplace add ArthurSonzogni/RTXUI
/plugin install rtxui@rtxui
```

Other agents that read Agent Skills can use the file directly:
[`tools/claude-plugin/skills/rtxui/SKILL.md`](https://github.com/ArthurSonzogni/RTXUI/blob/main/tools/claude-plugin/skills/rtxui/SKILL.md).

## Let It Check Its Work

An agent should not trust an interface it has only read. Have it run the
application with [diagnostics](/guide/diagnostics) made fatal and the screen
printed as text ([headless rendering](/guide/headless)):

```bash
printf '\t\r' | RTXUI_STRICT=1 RTXUI_HEADLESS=80x24 ./my_app
```

`RTXUI_STRICT=1` stops on the first construct RTXUI would ignore and says
what to write instead. `RTXUI_HEADLESS` shows what the interface looks like
after the piped input, so the agent can compare it with what was asked.
