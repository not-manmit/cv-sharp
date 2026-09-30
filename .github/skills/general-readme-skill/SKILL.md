---
name: general-readme-skill
description: "Use when generating, improving, updating, or reviewing README.md and project documentation. Trigger on /readme, generate readme, write readme, create project documentation, update README, or equivalent requests."
version: 1.0
tags: documentation, readme, auto-generate, project-docs
---

# General README Skill

Generate professional, human-readable README files from verified project content. Never invent features, commands, dependencies, endpoints, screenshots, contributors, licenses, or deployment steps.

## Workflow

Follow this order: **Configure -> Scan -> Generate -> Beautify -> Validate**.

## 1. Configure

Use these defaults unless the user specifies otherwise:

- Tone: Professional
- Badge style: Flat
- Primary language: English
- Secondary languages: none
- Output: repository root `README.md`

Tone options:

- **Energetic:** direct and confident; feature emojis are allowed only when appropriate.
- **Minimal:** terse and code-first; remove redundant explanation.
- **Professional:** neutral, structured, and formal.

Badge options: `flat`, `flat-square`, or `for-the-badge`. Do not add badges for technologies that are not detected in project files.

If the user gives explicit configuration, follow it. Do not block generation just because optional style settings were omitted.

## 2. Scan the Project

Read static project files only. Do not execute project commands, modify project files, delete files, install packages, or inspect secrets.

Ignore generated and dependency directories such as `node_modules`, `.git`, `build`, `dist`, `__pycache__`, `.venv`, `.next`, `coverage`, and logs.

Inspect the repository root and relevant package roots for:

- Manifests: `package.json`, `pyproject.toml`, `requirements.txt`, `go.mod`, `Cargo.toml`, `Gemfile`, `pom.xml`, `build.gradle`
- Build files: `Makefile`, `CMakeLists.txt`, Docker files, CI workflow files
- Configuration: `.env.example`, `*.config.*`, `*.yml`, `*.yaml`
- API definitions and route files
- Root `LICENSE`, `LICENSE.md`, `CONTRIBUTING.md`, `AUTHORS`, or `CONTRIBUTORS`
- Existing `README.md`

Detection precedence:

1. Manifest declarations
2. Dependency declarations
3. File-extension majority
4. Filename heuristics

If multiple package roots conflict, ask which root to document. If clarification is not available, use the root manifest that most clearly represents the requested application and state the assumption.

Detect when supported:

- Primary language and framework
- Build and test commands from manifest scripts
- Frontend/backend split or other architecture
- REST, GraphQL, gRPC, or WebSocket interfaces
- Database and ORM dependencies/configuration
- Container and CI/CD setup
- Application, library, CLI, or static-site project type
- License and static contributor information

Never expose secrets. Mask values from `.env`, credentials, tokens, private URLs, and connection strings. Do not list contributor emails.

If no license exists, do not create a license section as though one exists. Add this recommendation only when appropriate: `No LICENSE file detected. Add a LICENSE to clarify project licensing.`

## 3. Generate Content

Use only facts found during the scan. Omit sections with no verified project data. Do not write `N/A`, `Coming soon`, placeholder commands, or generic filler.

Keep this section order for generated content:

1. Hero
2. Features / Why
3. Quick Start
4. Usage
5. Architecture
6. Configuration
7. API
8. Directory Structure
9. Tech Stack
10. Deployment
11. Contributing
12. License

Preserve existing manual content. Treat content between `<!-- MANUAL-START -->` and `<!-- MANUAL-END -->` as protected. Also preserve top-level sections that are not explicitly marked `<!-- AUTO-GENERATED -->`. Regenerate only marked or clearly generated content, and do not delete user-authored sections to force ordering.

### Hero

Use the real project name and a one-line description. Badges must link to valid targets and represent detected technologies or actions. Centered HTML is acceptable for a polished open-source README, but keep the markup maintainable.

### Features / Why

List no more than six differentiating capabilities, each grounded in the implementation. Prefer concrete behavior over marketing language.

### Quick Start

Provide no more than four copy-paste steps. Include prerequisites, install commands, and the actual development/build command discovered in the repository. Do not invent environment variables or services.

### Usage

Show real commands, routes, or code examples from the project. Keep examples short and runnable. If no public usage surface is discoverable, omit the section.

### Architecture

Include a Mermaid diagram when the repository has enough verified structure to support one. Choose a diagram that matches the project, such as frontend/backend flow, pipeline, or module relationships. Do not create a diagram that implies unverified services.

### Configuration

Include a table only for configuration that is actually detected. Explain safe example values without reproducing secrets.

### API

Include an endpoint table only when routes or API handlers are present. Use verified HTTP methods, paths, request shapes, and response behavior.

### Directory Structure

Show a concise annotated tree with a maximum depth of three. Include only meaningful, verified paths.

### Tech Stack

Group detected technologies by layer, such as frontend, backend, algorithms, data, tooling, and testing. Do not list a technology merely because it is common for the language.

### Deployment

Include Docker, CI, hosting, or production instructions only when corresponding files or scripts exist.

### Contributing

Use the standard fork, branch, change, test, and pull-request workflow only when a contribution section is useful. Do not claim a project-specific convention unless it was detected in repository documentation.

### License

State the detected license in one line. If none exists, use the recommendation described in the scan rules.

## 4. Beautify

After generating content, improve readability without changing facts:

- Use a centered hero only when it improves the project presentation.
- Keep Markdown tables, lists, fenced code blocks, and Mermaid diagrams maintainable.
- Use accessible badge alt text and meaningful link labels.
- Keep headings, spacing, and code examples consistent.
- Do not add decorative HTML, badges, screenshots, or diagrams with no verified purpose.

If the user requests confirmation before beautification, present the proposed transformations first. Otherwise apply only low-risk formatting improvements.

## 5. Validate

Before finishing:

- Confirm all commands and paths exist in scanned files.
- Confirm headings follow the generated section order unless protected manual content requires otherwise.
- Confirm no secrets, private metadata, emails, or fabricated facts were included.
- Confirm links and anchors are syntactically valid.
- Confirm the README uses UTF-8 and consistent line endings.
- Confirm the project root and package root are clearly identified.
- Mention missing license, unavailable contributor history, incomplete scans, or unverified areas explicitly.

## Output

Write the primary README to the selected project root. For requested secondary languages, use `README-<language>.md` and add a language switcher to each generated file. End with a concise summary of what was documented and any assumptions or missing project data.
