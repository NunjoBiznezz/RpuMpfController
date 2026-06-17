# Claude behavior for this project

Do not make any file changes unless the user gives an explicit directive to do so
(e.g. "change X to Y", "add X", "delete X", "implement X", "write X").

Questions, analysis, and exploration requests ("could we...", "would it be possible...",
"does X work like Y", "verify that...", "how does...") should produce only a text answer —
no edits, no new files. When in doubt, answer first and ask before touching anything.

## Workflow
Changes do not require per-change spec approval. All changes must conform to:
- The porting and customization guidelines in `README.md`
- The goal-level specs in `specs/original/` and `specs/house-rules/`

Specs describe *what the system should do and why*, not implementation task lists.
If a requested change conflicts with an existing spec or README guideline, flag the
conflict before proceeding.
