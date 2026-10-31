"""Prepare generic C headers for the private module; no ROM data is copied."""
from pathlib import Path
import re
import shutil
import sys

engine = Path(sys.argv[1]).resolve()
sdk = Path(sys.argv[2]).resolve()
for relative in ("runner/cyc", "runner/include", "common"):
    for source in (engine / relative).rglob("*.h"):
        destination = sdk / source.relative_to(engine)
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, destination)

imports = (
    "cpu", "cyc_cpu_rts_observer", "cyc_cpu_ram_read_policy", "cpu6502_op_length",
    "cyc_trace_enabled", "cyc_trace_hash", "cyc_trace_file", "cyc_trace_cycle_hook",
    "cyc_trace_cycle", "hw_dma_stalls", "hw_frame_done", "hw_observe_line",
    "hw_observe_hit", "hw_frame_end_hit", "hw_prg_modified", "cyc_hooks_armed",
    "cyc_hook_passed", "cyc_hook_hit", "cyc_ram_code_dirty",
)
pattern = re.compile(r"(?m)^extern\s+(?=[^\n]*\b(?:" + "|".join(imports) + r")\b)")
count = 0
for header in (sdk / "runner/cyc").glob("*.h"):
    original = header.read_text(encoding="utf-8")
    updated, changed = pattern.subn("extern __declspec(dllimport) ", original)
    if changed:
        header.write_text(updated, encoding="utf-8", newline="\n")
        count += changed
if count != len(imports):
    raise RuntimeError(f"Expected {len(imports)} host data imports, found {count}; headers changed")
print(f"Generic module SDK prepared: {count} runtime data imports.")
