## Contributing

Pick up an existing issue or create a new one. Then create a new branch from `dev` following the naming style below. After your feature is finished, submit a PR and request a review from the team.

## Branch Naming

Follow the format:
```
<type>/<issue-number>/<short-name>
```

Try to avoid creating a branch without a linked issue. If you must, follow the following format:
```
<type>/<descriptive-name>
```

### Types

* pcb: Hardware design
* lib: STM32 library
* proj: SensEdu projects 
* docs: Website or README documentation
* deploy: Website deployment

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

* Subject line: maximum 50 characters, use imperative mood ("Add" not "Added")
* No period at the end of subject line
* During PR review, reference the PR number in subject line

### Types

* feat: New features/additions
* fix: Bug fixes and other broken behaviour
* refactor: Code or project restructuring
* docs: Website or README documentation
* deploy: Website deployment

### Good Examples

New lib feature:
```
feat: Add ADC3 support
```
PCB design:
```
feat: Draw power supply schematic
```
Docs editing:
```
docs: Add implementation section for EMG

Document hardware setup
Write about RC values for amplifier circuit
Add data acquisition code snippet
```
Bug fix:
```
fix: Rewrite ADC polling

Fix improper ADC initialization in CFGR1 register
```
PR review:
```
refactor: Rename lib example (#90)

Rename `adc_Record` to `ADC_Record` 
```