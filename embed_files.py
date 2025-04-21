from pathlib import Path
Import("env")

TEMPLATES_DIR = Path(env["PROJECT_DIR"]) / "data/templates"
OUT_HEADER = Path(env["PROJECT_DIR"]) / "src/static_files.h"

def sanitize_name(path: Path) -> str:
    # data/templates/ap_settings.html → AP_SETTINGS_HTML
    return path.stem.upper() + "_HTML"

def generate_static_files_header():
    lines = []
    lines.append('#pragma once\n')
    for file in TEMPLATES_DIR.glob("*.html"):
        name = sanitize_name(file)
        content = file.read_text(encoding="utf-8")
        lines.append(f'const char {name}[] PROGMEM = R"rawliteral(\n{content}\n)rawliteral";\n')
    OUT_HEADER.write_text("".join(lines), encoding="utf-8")
    print(f"[EMBED] Generated: {OUT_HEADER}")

generate_static_files_header()
