# Driftstead Agent Guide

## Commands

- Build editor: `powershell -ExecutionPolicy Bypass -File Tools/BuildEditor.ps1`
- Generate assets: `powershell -ExecutionPolicy Bypass -File Tools/GenerateAssets.ps1`
- Run tests: `powershell -ExecutionPolicy Bypass -File Tools/RunTests.ps1`
- Run game: `powershell -ExecutionPolicy Bypass -File Tools/RunGame.ps1`
- Package Win64: `powershell -ExecutionPolicy Bypass -File Tools/PackageWin64.ps1`

## Structure and boundaries

- Runtime rules, state, inventory, upgrades, facilities, quests, saves, and tests belong in C++.
- Blueprint assets are thin visual/configuration subclasses. Editor Python creates repeatable content.
- Content lives below `Content/Driftstead`; asset prefixes follow `BP_`, `WBP_`, `M_`, `MI_`, `DT_`, `DA_`, `E_`, `S_`, and `L_`.

## Prohibitions

- Never edit `.uasset` or `.umap` as text.
- Never put core logic in a Level Blueprint.
- Never use `GetAllActorsOfClass` in Tick or perform per-frame global scans.
- Never delete user-created assets from automation scripts.
- Close only an empty editor or an editor running this project before compiling C++.

## Definition of done

The editor target builds, `L_Demo` is playable, `Driftstead.*` automation tests pass, Win64 packaging succeeds, the packaged EXE launches, and `Docs/TEST_REPORT.md` records real evidence.
