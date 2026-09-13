# Luna OS agent instructions

- After every change to tracked project files, run the relevant build/checks, create a focused Git commit, and push it to `origin/main`.
- Keep commits small and describe the user-visible or architectural change.
- Do not amend or rewrite published commits unless explicitly requested.
- Do not commit generated build artifacts; `build/` is ignored.
- For kernel changes, run `make` and, when practical, boot the ISO with QEMU and inspect serial output.
- For documentation-only changes, verify Markdown and repository status, then commit and push.
- If a push fails, preserve the local commit and report the exact failure rather than silently dropping work.
