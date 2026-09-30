---
name: ui-ux-pro-max
description: "Use when designing, building, reviewing, or fixing web, mobile, or desktop interfaces, including UI structure, responsive layout, accessibility, interaction patterns, typography, color systems, charts, animation, design systems, and stack-specific frontend implementation."
---

# UI/UX Pro Max

Use this skill for visual design decisions and user-experience quality control. It provides searchable guidance for styles, product patterns, palettes, typography, UX, icons, animation, charts, and frontend stacks.

## When to Apply

Apply this skill when a task changes how an interface looks, feels, moves, or is interacted with:

- New pages, dashboards, components, or design systems
- Responsive layout, typography, color, spacing, or visual hierarchy
- Accessibility, forms, feedback, navigation, or interaction behavior
- Animation, motion, charts, icons, or perceived performance
- UI review, polish, refactoring, or consistency fixes

Skip it for pure backend logic, API/database design, infrastructure, or non-visual scripts.

## Priority Rules

Follow these priorities in order:

1. **Accessibility:** contrast of at least 4.5:1, meaningful alt text, keyboard navigation, visible focus, and accessible labels.
2. **Touch and interaction:** controls should be at least 44x44px, have adequate spacing, provide loading/state feedback, and not depend on hover alone.
3. **Performance:** prefer WebP/AVIF, lazy loading, reserved layout space, and motion that does not cause layout thrashing.
4. **Style selection:** match the visual language to the product and keep icon, component, and interaction styles consistent. Do not use emoji as UI icons.
5. **Responsive layout:** use mobile-first constraints, stable dimensions, consistent breakpoints, and no horizontal scrolling.
6. **Typography and color:** use a readable 16px base, approximately 1.5 body line height, semantic color tokens, and sufficient contrast. Avoid raw color values scattered through components.
7. **Animation:** use motion to communicate meaning, with context-appropriate timing and reduced-motion support. Avoid animating width or height when transform or opacity works.
8. **Forms and feedback:** use visible labels, local errors, helper text, progressive disclosure, and clear success/loading states.
9. **Navigation:** keep hierarchy predictable, preserve back behavior, and support deep links where applicable.
10. **Charts and data:** provide legends and tooltips, use accessible colors, and never rely on color alone.

## Workflow

### 1. Analyze the Request

Identify:

- Product type: SaaS, dashboard, tool, e-commerce, portfolio, entertainment, or other
- Audience and usage context
- Style keywords: minimal, playful, vibrant, immersive, content-first, dark mode, and so on
- Target stack from the repository; inspect `package.json`, `pubspec.yaml`, `Package.swift`, project files, or other framework markers. Never assume a stack.

### 2. Establish a Design System

For a new page or project, generate a coherent design direction before implementation. Define:

- Visual style and product rationale
- Color tokens and semantic states
- Type scale and font pairing
- Spacing, radius, elevation, and layout constraints
- Component states and interaction behavior
- Motion level and reduced-motion behavior
- Anti-patterns to avoid

Persist a project-wide master design system only when the repository has the necessary search data and the user wants it. Use page-specific overrides only when they genuinely differ from the master system. Read an existing master before changing it, and never overwrite one without explicit authorization.

### 3. Search Guidance

Use the skill's local search data when available. Build queries with one dominant intent, 2-5 meaningful terms, and one useful constraint.

Typical domains:

- `product`: product-type patterns
- `style`: visual styles
- `color`: palettes and accessible pairs
- `typography`: font pairings
- `google-fonts`: individual font choices
- `chart`: chart types and data presentation
- `ux`: accessibility, forms, layout, navigation, and interaction
- `icons`: icon selection and labeling
- `gsap`: animation presets
- `react`: React performance and rendering guidance
- `web`: web/native interface guidance

For a known implementation stack, use its stack guidance separately. Supported examples include `react`, `nextjs`, `vue`, `svelte`, `astro`, `angular`, `laravel`, `swiftui`, `react-native`, `flutter`, `jetpack-compose`, `html-tailwind`, `shadcn`, `threejs`, `javafx`, `wpf`, `winui`, `avalonia`, `uno`, and `uwp`.

If a search returns no results, retry once with a narrower query or explicit domain/stack. If it still returns nothing, label any advice as general fallback guidance rather than presenting it as a database match. Never fabricate search results.

For accessibility work, search for the observable outcome first, then the component and stack. Examples:

- `error summary validation` in `ux`
- `focus not obscured` in `ux`
- `decorative icon aria hidden` in `icons`
- `icon button accessible label` in `icons`
- `chip badge overflow nowrap` for the detected stack

### 4. Implement

Use the repository's existing framework, components, and design conventions. Keep the implementation focused and complete the expected states:

- Loading, empty, error, success, disabled, hover, focus, pressed, and selected states where relevant
- Keyboard and touch interaction
- Responsive behavior across mobile and desktop
- Accessible names, roles, relationships, and announcements
- Stable dimensions for boards, grids, toolbars, charts, and repeated items
- Reduced-motion behavior for meaningful animation

Prefer established icon libraries over manually drawn SVG icons. Use familiar symbols for icon-only actions and provide accessible labels and tooltips for unfamiliar icons. Do not use rounded text containers when a familiar symbol or control is more appropriate.

### 5. Review Before Delivery

Check the finished UI for:

- Contrast, focus visibility, keyboard access, and screen-reader labeling
- 44px touch targets and sufficient spacing
- No horizontal overflow or clipped text at mobile and desktop widths
- No overlapping content, layout shift, or unstable controls
- Text that fits its parent and remains readable at normal zoom
- Consistent tokens, type scale, spacing, radii, and icon treatment
- Loading, error, empty, and disabled states
- `prefers-reduced-motion` support
- Charts with legends, tooltips, and non-color distinctions
- Images with correct content, alt text, dimensions, and lazy loading where appropriate

For Three.js interfaces, verify that the scene is nonblank, correctly framed, responsive, interactive, and free of unintended overlap at desktop and mobile sizes.

## Design Direction

Avoid interchangeable or generic layouts. Choose a clear visual direction suited to the domain, with purposeful typography and a restrained but distinctive palette. Do not default to purple-on-white, dark-mode-only, flat single-color backgrounds, decorative blobs, excessive cards, or marketing-style composition for operational tools.

For dashboards and internal tools, prioritize scanning, comparison, predictable navigation, and repeated actions. For games and immersive tools, allow more expressive visuals and motion while preserving accessibility and control clarity.

Use full-width bands or unframed layouts for page sections. Reserve cards for repeated items, modals, and genuinely framed tools; do not nest cards inside cards.

## Search Tool Integration

If this skill is bundled with a local search script, invoke it using its absolute path rather than assuming a working directory:

```powershell
python <absolute-skill-directory>\scripts\search.py "<query>" --domain <domain>
```

For a complete system:

```powershell
python <absolute-skill-directory>\scripts\search.py "<product> <industry> <keywords>" --design-system -p "<Project Name>"
```

Use the detected project root as the output directory when persisting generated design-system files. Do not persist unverified output.

## Completion Standard

Do not consider a UI task complete until the implementation has been checked at relevant viewport sizes and the interaction states above have been considered. Report any unavailable search data, untested viewport, missing asset, or unresolved accessibility issue explicitly.
