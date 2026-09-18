# Contributing to SensEdu

Pick up an existing [issue](https://github.com/ShiegeChan/SensEdu/issues) or create a new one. Then create a new branch from `dev` following the naming style below. After your feature is finished, submit a PR and request a review from the team.

This file covers the git conventions only. The full contribution guide lives on the website:

* [Contributing](https://sensedu-shield.com/contributing/) - how to report bugs, suggest features and submit a project
* [Library](https://sensedu-shield.com/contributing/library/) - code style for `libraries/SensEdu/src/`
* [Projects](https://sensedu-shield.com/contributing/projects/) - sketch style for `projects/` and `libraries/SensEdu/examples/`
* [Documentation](https://sensedu-shield.com/contributing/docs/) - writing wiki pages and hosting them locally

## Branch Naming

Follow the format:
```
<type>/<issue-number>/<short-name>
```

Try to avoid creating a branch without a linked issue. If you must, follow the format:
```
<type>/<descriptive-name>
```

### Types

* **pcb**: Hardware design
* **lib**: STM32 library
* **proj**: SensEdu projects
* **docs**: Website or README documentation
* **deploy**: Website deployment

### Good Examples

```
proj/7/record-audio
lib/13/dma-integration
pcb/34/tweak-rx-schematics
docs/10/web-emg-page
```

## Commit Messages

Follow the format:

```
<type>: <subject>

<optional body>
```

* Subject line: maximum 50 characters, use imperative ("Add" not "Added")
* No period at the end of subject line
* During PR review, reference the PR number in subject line

### Types

* **feat**: New features/additions
* **fix**: Bug fixes and other broken behaviour
* **refactor**: Code or project restructuring
* **docs**: Website or README documentation
* **deploy**: Website deployment

### Good Examples

```
# New lib feature:
feat: Add ADC3 support
```
```
# PCB design:
feat: Draw power supply schematic
```
```
# Docs editing:
docs: Finish EMG implementation section

Document hardware setup
Explain RC values for amplifier circuit
Add data acquisition code snippet
```
```
# Bug fix:
fix: Rewrite ADC polling

Fix improper ADC initialization in CFGR1 register
```
```
# PR review:
refactor: Rename lib example (#90)

Rename `adc_Record` to `ADC_Record`
```

## Pull Requests

Keep PRs small, focusing on one feature or fix per PR. The PR title follows the same `<type>: <Descriptive Name>` format as a commit subject. Before requesting a review, make sure that:

* The code follows the [style guidelines](https://sensedu-shield.com/contributing/#styling-guidelines)
* The code compiles and new additions are tested on hardware
* Library changes do not break any of the existing examples in `libraries/SensEdu/examples/`
* New functionality is documented on the [website](https://sensedu-shield.com), see the [documentation guide](https://sensedu-shield.com/contributing/docs/)
