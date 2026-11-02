# AI Robot

Personal embedded engineering project: an AI-enabled robot with smart audio and motion capabilities.

> Status: Planning. Product vision, features, hardware, and architecture are not finalized.

## Project goals

1. **Product:** Build a working, reliable system with verifiable behavior.
2. **Documentation:** Keep design decisions, setup instructions, interfaces and verification evidence reproducible.
3. **Engineering Mastery:** Understand, explain, modify, test and debug the core engineering concepts used.

## Getting started

1. Define the product vision in [docs/project/vision.md](docs/project/vision.md).
2. Break the vision into milestones in [docs/project/roadmap.md](docs/project/roadmap.md).
3. Add features to [docs/project/backlog.md](docs/project/backlog.md).
4. Keep a short note per work session under `docs/sessions/YYYY/`.
5. Track essential knowledge in [docs/knowledge/index.md](docs/knowledge/index.md).

## Working rhythm

- **Each session:** Choose one small goal -> work -> check what was learned -> leave a short session note.
- **At feature completion:** Verify Product, Documentation, and Engineering Mastery before closing.
- **Weekly:** Review progress, blockers, essential knowledge and next week's focus.
- Keep records lightweight; link to source, commits and evidence instead of duplicating logs.

## Repository layout

```text
ai-robot/
├── README.md
├── docs/
│   ├── project/             # Vision, roadmap, backlog
│   ├── architecture/        # System-level designs (when needed)
│   ├── features/            # Feature requirements, designs, verification
│   ├── knowledge/           # Knowledge register and topic notes
│   ├── sessions/            # Daily work notes
│   ├── weekly-reviews/      # Weekly progress reviews
│   ├── decisions/           # Architecture Decision Records (ADRs)
│   └── templates/           # Lightweight note templates
├── firmware/                # MCU firmware (when architecture is selected)
├── software/                # Higher-level software (when needed)
├── hardware/                # Schematics / hardware designs (when needed)
├── tests/                   # Integration and test utilities (when needed)
└── .agents/                 # AI-agent workflows and reusable skills (when needed)
```

## Ground rules

- **One source of truth:** Track feature status in the backlog; use links rather than duplicate content.
- **Knowledge verification:** Core knowledge is a requirement for feature closure.
- **Document as you build:** Update design/test evidence when behavior changes, not only at week-end.
- **AI as collaborator:** The engineer owns design decisions, code review and verification.
- **No proprietary company code or documents in this personal repository.**

## Git first steps

```bash
git init
git add .
git commit -m "chore: initialize AI robot project structure"
```

The empty directories are tracked using `.gitkeep` files. Replace/remove them when real files are added.
