# AI Robot

Personal embedded engineering project: an AI-enabled robot with smart audio and motion capabilities.

> Status: Planning. Product vision, features, hardware, and architecture are not finalized.

M0 provides a local C++17 build foundation. Audio, AI, Motion, and Orchestrator
remain unimplemented; product planning continues in the existing documents.

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

## Build and test (M0)

Use a Linux terminal (WSL Ubuntu on Windows) with a C++17 compiler, CMake 3.16+
and a build tool such as Make. On Ubuntu, install the prerequisites with:

```bash
sudo apt-get update
sudo apt-get install build-essential cmake
```

From the repository root:

```bash
bash tools/scripts/build.sh
./build/ai_robot
bash tools/scripts/test.sh
```

The scripts accept no arguments, work from any current directory, and use
`build/` for a Debug build. The test script configures and builds before running
CTest, and fails if no tests are registered. The equivalent manual commands are:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure --no-tests=error
```

The single `ai_robot_smoke` test runs the executable and requires a successful
exit within five seconds. The executable prints `AI Robot: M0 foundation ready.`
No test framework or external library is needed. Keep generated files in an out-of-source build directory.

Future subsystem code belongs in the empty `software/src/audio/`, `ai/`,
`motion/`, and `orchestrator/` directories. M0 adds no subsystem interfaces or
manager classes. For Raspberry Pi builds, see the
[ARM cross-compilation guide](docs/project/arm-cross-compilation.md) for toolchain
setup, ELF inspection, and deployment dry runs.

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
├── CMakeLists.txt           # C++17 executable and CTest smoke test
├── software/
│   └── src/
│       ├── main.cpp         # Minimal executable entry point
│       ├── audio/           # Future Audio subsystem
│       ├── ai/              # Future AI subsystem
│       ├── motion/          # Future Motion subsystem
│       └── orchestrator/    # Future integration, when needed
├── hardware/                # Schematics / hardware designs (when needed)
├── tests/                   # Integration and test utilities (when needed)
├── tools/scripts/           # Build/test scripts; future deployment tooling
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
